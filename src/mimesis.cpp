/* Mimesis -- a library for parsing and creating RFC2822 messages
   Copyright © 2017-2026 Guus Sliepen <guus@lightbts.info>

   Mimesis is free software; you can redistribute it and/or modify it under the
   terms of the GNU Lesser General Public License as published by the Free
   Software Foundation, either version 3 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
   more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "mimesis.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "base64.hpp"
#include "charset.hpp"
#include "encoded-words.hpp"
#include "parameters.hpp"
#include "quoted-printable.hpp"
#include "string-utils.hpp"

using namespace std;

namespace Mimesis {

static string random_string(size_t length) {
	static constexpr string_view characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	thread_local mt19937_64 engine = [] {
		random_device device;
		seed_seq seed{device(), device(), device(), device(), device(), device(), device(), device()};
		return mt19937_64(seed);
	}();
	uniform_int_distribution<size_t> distribution(0, characters.size() - 1);

	string result;
	ranges::generate_n(back_inserter(result), length, [&] { return characters[distribution(engine)]; });
	return result;
}

static string_view line_ending(bool crlf) {
	return crlf ? "\r\n" : "\n";
}

// Matches "type/subtype" exactly, or only the type if either side has no subtype.
static bool types_match(string_view a, string_view b) {
	a = trim(a);
	b = trim(b);
	if (a.find('/') == string_view::npos || b.find('/') == string_view::npos)
		return iequals(a.substr(0, a.find('/')), b.substr(0, b.find('/')));
	return iequals(a, b);
}

static bool has_parameters(string_view field) {
	return iequals(field, "Content-Type") || iequals(field, "Content-Disposition");
}

static bool is_content_header(string_view field) {
	return istarts_with(field, "Content-");
}

static auto has_field(string_view field) {
	return [field](const Header &header) { return iequals(header.first, field); };
}

static bool is_field_name(string_view field) {
	return !field.empty() && ranges::all_of(field, [](unsigned char c) { return c > 32 && c < 127 && c != ':'; });
}

static void check_header(string_view field, string_view value) {
	if (!is_field_name(field))
		throw invalid_argument(format("Invalid header field name \"{}\"", field));

	// Line breaks are only allowed when followed by whitespace (folding).
	for (size_t i = 0; i < value.size(); i++) {
		if (value[i] != '\r' && value[i] != '\n')
			continue;
		if (value.substr(i).starts_with("\r\n"))
			i++;
		bool is_folded = value[i] == '\n' && i + 1 < value.size() && (value[i + 1] == ' ' || value[i + 1] == '\t');
		if (!is_folded)
			throw invalid_argument(format("Invalid line break in value of header \"{}\"", field));
	}
}

// The boundary may be followed by "--" and whitespace, including a CR.
static bool is_boundary_line(string_view line, string_view boundary) {
	if (!line.starts_with("--") || !line.substr(2).starts_with(boundary))
		return false;
	line.remove_prefix(2 + boundary.size());
	if (line.starts_with("--"))
		line.remove_prefix(2);
	return ranges::all_of(line, is_space);
}

static bool is_final_boundary_line(string_view line, string_view boundary) {
	return is_boundary_line(line, boundary) && line.substr(2 + boundary.size()).starts_with("--");
}

static bool is_any_boundary_line(string_view line, const vector<string> &boundaries) {
	return ranges::any_of(boundaries, [&](const string &boundary) { return is_boundary_line(line, boundary); });
}

// Folds the header at whitespace, so lines are at most 76 characters long where possible
// (RFC 2047 section 2). Values that already contain line breaks are left alone.
static string fold_header(string_view field, string_view value, string_view ending) {
	static constexpr size_t max_line_length = 76;

	if (value.empty())
		return format("{}:", field);
	if (value.find('\n') != string_view::npos)
		return format("{}: {}", field, value);

	string result;
	string line = format("{}:", field);
	string text = format(" {}", value);

	// Each piece consists of whitespace followed by a word; a line break is inserted in front of it if needed.
	for (size_t begin = 0; begin < text.size();) {
		auto word = text.find_first_not_of(" \t", begin);
		auto end = word == string::npos ? text.size() : min(text.find_first_of(" \t", word), text.size());
		auto piece = string_view(text).substr(begin, end - begin);

		if (!line.empty() && line.size() + piece.size() > max_line_length) {
			result += line;
			result += ending;
			line.clear();
		}

		line += piece;
		begin = end;
	}

	return result + line;
}

static string format_date(chrono::system_clock::time_point date) {
	return format("{:%a, %d %b %Y %H:%M:%S} +0000", chrono::floor<chrono::seconds>(date));
}

// Loading and saving a whole MIME message

// Returns the boundary line of an enclosing part that ended this part, or an empty string at the end of input.
string Part::load_part(istream &in, vector<string> &boundaries) {
	string line;
	size_t crlf_lines = 0;
	size_t lf_lines = 0;

	auto append_line = [&](string &destination) {
		destination += line;
		if (!in.eof())
			destination += '\n';
	};

	while (getline(in, line)) {
		if (is_any_boundary_line(line, boundaries))
			return line;

		if (line.ends_with('\r')) {
			crlf_lines++;
			line.pop_back();
		} else {
			lf_lines++;
		}

		if (line.empty())
			break;

		if (line[0] == ' ' || line[0] == '\t') {
			if (headers.empty())
				throw runtime_error(format("Invalid header continuation line \"{}\"", line));
			auto &value = headers.back().second;
			auto continuation = string_view(line);
			// The value can start on a continuation line, its leading whitespace is not part of it.
			if (value.empty())
				continuation.remove_prefix(min(continuation.find_first_not_of(" \t"), continuation.size()));
			value += continuation;
			continue;
		}

		// An mbox "From " line in front of the headers.
		if (headers.empty() && line.starts_with("From "))
			continue;

		auto colon = line.find(':');
		if (colon == string::npos || !is_field_name(string_view(line).substr(0, colon)))
			throw runtime_error(format("Invalid header line \"{}\"", line));

		auto value = string_view(line).substr(colon + 1);
		while (!value.empty() && is_space(value.front()))
			value.remove_prefix(1);

		headers.emplace_back(line.substr(0, colon), value);
	}

	crlf = crlf_lines > lf_lines;

	auto content_type = get_header("Content-Type");
	if (types_match(get_value(content_type), "multipart"))
		boundary = get_parameter(content_type, "boundary");
	multipart = !boundary.empty();

	if (!multipart) {
		while (getline(in, line)) {
			if (is_any_boundary_line(line, boundaries))
				return line;
			append_line(body);
		}
		return {};
	}

	boundaries.push_back(boundary);

	string terminator;
	while (getline(in, line)) {
		if (is_any_boundary_line(line, boundaries)) {
			terminator = line;
			break;
		}
		append_line(preamble);
	}

	while (is_boundary_line(terminator, boundary) && !is_final_boundary_line(terminator, boundary))
		terminator = parts.emplace_back().load_part(in, boundaries);

	boundaries.pop_back();

	// The final boundary is missing, the input is truncated or an enclosing part ended.
	if (!is_final_boundary_line(terminator, boundary))
		return terminator;

	while (getline(in, line)) {
		if (is_any_boundary_line(line, boundaries))
			return line;
		append_line(epilogue);
	}

	return {};
}

void Part::load(istream &in) {
	clear();
	vector<string> boundaries;
	load_part(in, boundaries);
	if (in.bad())
		throw runtime_error("Error reading message");
}

bool Part::ends_with_newline() const {
	if (multipart)
		return epilogue.empty() || epilogue.ends_with('\n');
	return body.empty() || body.ends_with('\n');
}

void Part::save(ostream &out) const {
	auto ending = line_ending(crlf);

	if (message && headers.empty())
		throw runtime_error("No headers specified");

	for (auto &[field, value]: headers) {
		check_header(field, value);
		out << fold_header(field, value, ending) << ending;
	}

	out << ending;

	if (!multipart) {
		out << body;
		return;
	}

	// Boundaries must start on a new line.
	out << preamble;
	bool at_line_start = preamble.empty() || preamble.ends_with('\n');
	for (auto &part: parts) {
		if (!at_line_start)
			out << ending;
		out << "--" << boundary << ending;
		part.save(out);
		at_line_start = part.ends_with_newline();
	}
	if (!at_line_start)
		out << ending;
	out << "--" << boundary << "--" << ending;
	out << epilogue;
}

void Part::load(const filesystem::path &filename) {
	ifstream in(filename, ios::binary);
	if (!in.is_open())
		throw runtime_error(format("Could not open message file {}", filename.string()));
	load(in);
}

void Part::save(const filesystem::path &filename) const {
	ofstream out(filename, ios::binary);
	if (!out.is_open())
		throw runtime_error(format("Could not open message file {}", filename.string()));
	save(out);
	out.close();
	if (out.fail())
		throw runtime_error(format("Could not write message file {}", filename.string()));
}

void Part::from_string(string_view data) {
	istringstream in{string(data)};
	load(in);
}

string Part::to_string() const {
	ostringstream out;
	save(out);
	return std::move(out).str();
}

void Part::set_crlf(bool value) {
	crlf = value;
}

// Low-level access

string Part::get_body() const {
	auto encoding = get_header_value("Content-Transfer-Encoding");
	string result;

	if (iequals(encoding, "quoted-printable"))
		result = quoted_printable_decode(body);
	else if (iequals(encoding, "base64"))
		result = base64_decode(body);
	else
		result = body;

	if (is_mime_type("text")) {
		auto charset = get_header_parameter("Content-Type", "charset");
		if (!is_utf8_compatible(charset))
			result = charset_decode(charset, result);
	}

	return result;
}

string Part::get_preamble() const {
	return preamble;
}

string Part::get_epilogue() const {
	return epilogue;
}

string Part::get_boundary() const {
	return boundary;
}

vector<Part> &Part::get_parts() {
	return parts;
}

const vector<Part> &Part::get_parts() const {
	return parts;
}

vector<Header> &Part::get_headers() {
	return headers;
}

const vector<Header> &Part::get_headers() const {
	return headers;
}

bool Part::is_multipart() const {
	return multipart;
}

bool Part::is_multipart(string_view subtype) const {
	return multipart && types_match(get_mime_type(), format("multipart/{}", subtype));
}

bool Part::is_singlepart() const {
	return !multipart;
}

bool Part::is_singlepart(string_view type) const {
	return !multipart && types_match(get_mime_type(), type);
}

bool Part::is_attachment() const {
	return iequals(get_header_value("Content-Disposition"), "attachment");
}

bool Part::is_inline() const {
	return iequals(get_header_value("Content-Disposition"), "inline");
}

void Part::set_body(string_view value) {
	if (multipart)
		throw runtime_error("Cannot set body of a multipart message");
	body = value;
}

void Part::set_preamble(string_view value) {
	if (!multipart)
		throw runtime_error("Cannot set preamble of a non-multipart message");
	preamble = value;
}

void Part::set_epilogue(string_view value) {
	if (!multipart)
		throw runtime_error("Cannot set epilogue of a non-multipart message");
	epilogue = value;
}

void Part::set_boundary(string_view value) {
	boundary = value;
	if (has_mime_type())
		set_header_parameter("Content-Type", "boundary", boundary);
}

void Part::set_parts(const vector<Part> &value) {
	if (!multipart)
		throw runtime_error("Cannot set parts of a non-multipart message");
	parts = value;
}

void Part::set_headers(const vector<Header> &value) {
	headers = value;
}

void Part::clear() {
	headers.clear();
	preamble.clear();
	body.clear();
	epilogue.clear();
	parts.clear();
	boundary.clear();
	multipart = false;
}

void Part::clear_body() {
	body.clear();
}

// Raw header manipulation

string Part::get_header(string_view field) const {
	auto it = ranges::find_if(headers, has_field(field));
	return it == headers.end() ? string{} : it->second;
}

void Part::set_header(string_view field, string_view value) {
	if (value.empty()) {
		erase_header(field);
		return;
	}

	check_header(field, value);
	auto it = ranges::find_if(headers, has_field(field));
	if (it == headers.end())
		headers.emplace_back(field, value);
	else
		it->second = value;
}

string &Part::operator[](string_view field) {
	auto it = ranges::find_if(headers, has_field(field));
	if (it != headers.end())
		return it->second;

	check_header(field, {});
	return headers.emplace_back(field, string{}).second;
}

const string &Part::operator[](string_view field) const {
	static const string empty_string;
	auto it = ranges::find_if(headers, has_field(field));
	return it == headers.end() ? empty_string : it->second;
}

void Part::append_header(string_view field, string_view value) {
	check_header(field, value);
	headers.emplace_back(field, value);
}

void Part::prepend_header(string_view field, string_view value) {
	check_header(field, value);
	headers.emplace(headers.begin(), field, value);
}

void Part::erase_header(string_view field) {
	erase_if(headers, has_field(field));
}

void Part::clear_headers() {
	headers.clear();
}

// Decoded header access

string Part::get_header_value(string_view field) const {
	auto value = get_header(field);
	if (has_parameters(field))
		return string(get_value(value));
	return decode_encoded_words(value);
}

void Part::set_header_value(string_view field, string_view value) {
	if (!has_parameters(field)) {
		set_header(field, encode_encoded_words(value));
		return;
	}

	auto it = ranges::find_if(headers, has_field(field));
	if (it == headers.end())
		append_header(field, value);
	else
		it->second = set_value(it->second, value);
}

string Part::get_header_parameter(string_view field, string_view parameter) const {
	return get_parameter(get_header(field), parameter);
}

void Part::set_header_parameter(string_view field, string_view parameter, string_view value) {
	auto it = ranges::find_if(headers, has_field(field));
	if (it == headers.end())
		append_header(field, set_parameter({}, parameter, value));
	else
		it->second = set_parameter(it->second, parameter, value);
}

string Part::get_multipart_type() const {
	if (!multipart)
		return {};
	auto type = get_mime_type();
	return type.substr(min(type.find('/') + 1, type.size()));
}

void Part::add_received(string_view text, chrono::system_clock::time_point date) {
	prepend_header("Received", format("{}; {}", text, format_date(date)));
}

void Part::generate_msgid(string_view domain) {
	set_header("Message-ID", format("<{}@{}>", random_string(32), domain));
}

void Part::set_date(chrono::system_clock::time_point date) {
	set_header("Date", format_date(date));
}

// Part manipulation

void Part::erase_content_headers() {
	erase_if(headers, [](auto &header) { return is_content_header(header.first); });
}

// Replaces the destination's Content-* headers with ours.
void Part::move_content_headers_to(Part &destination) {
	destination.erase_content_headers();
	ranges::copy_if(headers, back_inserter(destination.headers), is_content_header, &Header::first);
	erase_content_headers();
}

void Part::copy_content_from(const Part &source) {
	erase_content_headers();
	ranges::copy_if(source.headers, back_inserter(headers), is_content_header, &Header::first);
	preamble = source.preamble;
	body = source.body;
	epilogue = source.epilogue;
	parts = source.parts;
	boundary = source.boundary;
	multipart = source.multipart;
}

void Part::add_mime_version() {
	if (message)
		set_header("MIME-Version", "1.0");
}

Part &Part::append_part() {
	return parts.emplace_back();
}

Part &Part::append_part(const Part &part) {
	return parts.emplace_back(part);
}

Part &Part::prepend_part() {
	return *parts.emplace(parts.begin());
}

Part &Part::prepend_part(const Part &part) {
	return *parts.insert(parts.begin(), part);
}

void Part::clear_parts() {
	parts.clear();
}

void Part::make_multipart(string_view subtype, string_view suggested_boundary) {
	if (multipart) {
		if (is_multipart(subtype))
			return;
		Part part;
		part.preamble = exchange(preamble, {});
		part.epilogue = exchange(epilogue, {});
		part.parts = exchange(parts, {});
		part.boundary = exchange(boundary, {});
		part.multipart = true;
		part.crlf = crlf;
		move_content_headers_to(part);
		parts.push_back(std::move(part));
	} else {
		multipart = true;
		add_mime_version();

		if (body.empty()) {
			erase_content_headers();
		} else {
			auto &part = append_part();
			part.crlf = crlf;
			move_content_headers_to(part);
			part.body = exchange(body, {});
		}
	}

	if (!suggested_boundary.empty())
		boundary = suggested_boundary;
	if (boundary.empty())
		boundary = random_string(32);

	set_header("Content-Type", format("multipart/{}; boundary={}", subtype, quote_token(boundary)));
}

bool Part::flatten() {
	if (!multipart)
		return true;

	if (parts.empty()) {
		erase_content_headers();
		preamble.clear();
		epilogue.clear();
		boundary.clear();
		multipart = false;
		return true;
	}

	if (parts.size() > 1)
		return false;

	auto child = std::move(parts.front());
	child.move_content_headers_to(*this);
	preamble = std::move(child.preamble);
	body = std::move(child.body);
	epilogue = std::move(child.epilogue);
	parts = std::move(child.parts);
	boundary = std::move(child.boundary);
	multipart = child.multipart;

	return true;
}

// Body and attachments

string Part::get_mime_type() const {
	return get_header_value("Content-Type");
}

void Part::set_mime_type(string_view type) {
	set_header_value("Content-Type", type);
}

bool Part::is_mime_type(string_view type) const {
	return types_match(get_mime_type(), type);
}

bool Part::has_mime_type() const {
	return !get_mime_type().empty();
}

namespace {

enum class TransferEncoding {
	seven_bit,
	quoted_printable,
	base64,
};

}

static TransferEncoding choose_transfer_encoding(string_view data, bool is_text) {
	// Only base64 preserves the exact bytes, including line endings and the absence of a final newline.
	if (!is_text)
		return TransferEncoding::base64;

	static constexpr size_t max_line_length = 998;
	size_t unsafe_bytes = 0;
	size_t line_length = 0;
	bool has_long_line = false;

	for (size_t i = 0; i < data.size(); i++) {
		auto c = static_cast<unsigned char>(data[i]);
		bool is_line_break = c == '\n' || (c == '\r' && i + 1 < data.size() && data[i + 1] == '\n');
		if (c == '\n') {
			line_length = 0;
			continue;
		}
		if (!is_line_break && (c >= 128 || c == 0 || c == '\r'))
			unsafe_bytes++;
		if (++line_length > max_line_length)
			has_long_line = true;
	}

	if (!unsafe_bytes && !has_long_line)
		return TransferEncoding::seven_bit;

	// Quoted-printable triples the size of unsafe bytes, base64 grows everything by a third.
	if (unsafe_bytes * 5 < data.size())
		return TransferEncoding::quoted_printable;

	return TransferEncoding::base64;
}

void Part::set_encoded_body(string_view data, bool is_text) {
	static constexpr size_t base64_line_length = 76;
	auto ending = line_ending(crlf);

	switch (choose_transfer_encoding(data, is_text)) {
	case TransferEncoding::seven_bit:
		erase_header("Content-Transfer-Encoding");
		body = data;
		break;
	case TransferEncoding::quoted_printable:
		set_header("Content-Transfer-Encoding", "quoted-printable");
		body = quoted_printable_encode(data, ending);
		break;
	case TransferEncoding::base64:
		set_header("Content-Transfer-Encoding", "base64");
		body = base64_encode(data, base64_line_length, ending);
		break;
	}
}

template <typename Self>
Self *Part::find_first_matching_part(Self &self, const function<bool(const Part &)> &predicate) {
	if (!self.multipart) {
		if (self.body.empty() && !self.has_mime_type())
			return nullptr;
		if (self.is_attachment())
			return nullptr;
	}

	if (predicate(self))
		return &self;

	for (auto &part: self.parts)
		if (auto result = find_first_matching_part(part, predicate))
			return result;

	return nullptr;
}

const Part *Part::get_first_matching_part(const function<bool(const Part &)> &predicate) const {
	return find_first_matching_part(*this, predicate);
}

Part *Part::get_first_matching_part(const function<bool(const Part &)> &predicate) {
	return find_first_matching_part(*this, predicate);
}

static function<bool(const Part &)> has_type(string_view type) {
	return [type = string(type)](const Part &part) {
		return types_match(part.has_mime_type() ? part.get_mime_type() : "text/plain", type);
	};
}

const Part *Part::get_first_matching_part(string_view type) const {
	return find_first_matching_part(*this, has_type(type));
}

Part *Part::get_first_matching_part(string_view type) {
	return find_first_matching_part(*this, has_type(type));
}

string Part::get_first_matching_body(string_view type) const {
	auto part = get_first_matching_part(type);
	return part ? part->get_body() : string{};
}

Part &Part::set_alternative(string_view subtype, string_view text) {
	auto type = format("text/{}", subtype);
	Part *part = nullptr;

	add_mime_version();

	if (!multipart) {
		// Try to put it in the body first.
		if (body.empty() || is_mime_type(type)) {
			part = this;
		} else if (is_mime_type("text") && !is_attachment()) {
			make_multipart("alternative");
			part = &append_part();
		} else {
			make_multipart("mixed");
			part = &prepend_part();
		}
	} else {
		// If there is already a part of this type, replace it.
		part = get_first_matching_part(type);

		// If there is already a multipart/alternative with text, add to that.
		if (!part) {
			part = get_first_matching_part([](const Part &candidate) {
				return candidate.is_multipart("alternative") && candidate.get_first_matching_part("text");
			});
			if (part)
				part = &part->append_part();
		}

		// If there is already inline text, make it multipart/alternative.
		if (!part && (part = get_first_matching_part("text"))) {
			part->make_multipart("alternative");
			part = &part->append_part();
		}

		// Otherwise, assume we're multipart/mixed.
		if (!part)
			part = &prepend_part();
	}

	part->erase_content_headers();
	part->set_header("Content-Type", format("{}; charset=utf-8", type));
	part->set_encoded_body(text, true);

	return *part;
}

void Part::set_plain(string_view text) {
	set_alternative("plain", text);
}

void Part::set_html(string_view text) {
	set_alternative("html", text);
}

string Part::get_plain() const {
	return get_first_matching_body("text/plain");
}

string Part::get_html() const {
	return get_first_matching_body("text/html");
}

string Part::get_text() const {
	return get_first_matching_body("text");
}

Part &Part::attach(const Part &attachment) {
	Part *part = this;
	if (multipart || !body.empty()) {
		make_multipart("mixed");
		part = &append_part();
	} else {
		add_mime_version();
	}

	if (attachment.message) {
		part->erase_content_headers();
		part->set_header("Content-Type", "message/rfc822");
		part->body = attachment.to_string();
	} else {
		part->copy_content_from(attachment);
	}

	if (part->get_header("Content-Disposition").empty())
		part->set_header("Content-Disposition", "attachment");
	else
		part->set_header_value("Content-Disposition", "attachment");

	return *part;
}

Part &Part::attach(string_view data, string_view type, string_view filename) {
	Part *part = this;
	if (multipart || !body.empty()) {
		make_multipart("mixed");
		part = &append_part();
	} else {
		add_mime_version();
	}

	part->erase_content_headers();
	part->set_header("Content-Type", type.empty() ? "text/plain" : type);
	part->set_header("Content-Disposition", "attachment");
	if (!filename.empty())
		part->set_header_parameter("Content-Disposition", "filename", filename);
	part->set_encoded_body(data, part->is_mime_type("text"));

	return *part;
}

Part &Part::attach(istream &in, string_view type, string_view filename) {
	string data{istreambuf_iterator<char>(in), istreambuf_iterator<char>()};
	return attach(data, type, filename);
}

vector<const Part *> Part::get_attachments() const {
	if (!multipart && is_attachment())
		return {this};

	vector<const Part *> attachments;
	for (auto &part: parts)
		ranges::copy(part.get_attachments(), back_inserter(attachments));

	return attachments;
}

void Part::simplify() {
	if (!multipart)
		return;

	for (auto &part: parts)
		part.simplify();

	erase_if(parts, [](const Part &part) {
		return part.headers.empty() && part.body.empty();
	});

	if (parts.empty()) {
		if (message)
			flatten();
		else
			clear();
	} else if (parts.size() == 1) {
		flatten();
	}
}

void Part::clear_attachments() {
	if (multipart) {
		for (auto &part: parts)
			part.clear_attachments();
		simplify();
	} else if (is_attachment()) {
		if (message) {
			erase_content_headers();
			body.clear();
		} else {
			clear();
		}
	}
}

void Part::clear_alternative(string_view type) {
	bool cleared = false;

	while (auto part = get_first_matching_part(type)) {
		// Keep the headers of the message itself.
		if (part->message) {
			part->erase_content_headers();
			part->body.clear();
		} else {
			part->clear();
		}
		cleared = true;
	}

	if (cleared)
		simplify();
}

void Part::clear_text() {
	clear_alternative("text");
}

void Part::clear_plain() {
	clear_alternative("text/plain");
}

void Part::clear_html() {
	clear_alternative("text/html");
}

bool Part::has_text() const {
	return get_first_matching_part("text");
}

bool Part::has_plain() const {
	return get_first_matching_part("text/plain");
}

bool Part::has_html() const {
	return get_first_matching_part("text/html");
}

bool Part::has_attachments() const {
	return is_attachment() || ranges::any_of(parts, &Part::has_attachments);
}

// RFC2822 messages

Message::Message() {
	message = true;
}

ostream &operator<<(ostream &out, const Part &part) {
	part.save(out);
	return out;
}

istream &operator>>(istream &in, Part &part) {
	part.load(in);
	return in;
}

}
