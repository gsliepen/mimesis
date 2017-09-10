/* Mimesis -- a library for parsing and creating RFC2822 messages
   Copyright © 2017 Guus Sliepen <guus@lightbts.info>
 
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
#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace Mimesis {

static std::random_device rnd;

static const string base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static string base64_encode(const void *data, size_t len) {
	string out;
	size_t outlen = ((len + 2) / 3) * 4;
	out.reserve(outlen);

	auto in = static_cast<const uint8_t *>(data);
	size_t i;

	for (i = 0; i < (len / 3) * 3; i += 3) {
		out.push_back(base64[                        (in[i + 0] >> 2)]);
		out.push_back(base64[(in[i + 0] << 4 & 63) | (in[i + 1] >> 4)]);
		out.push_back(base64[(in[i + 1] << 2 & 63) | (in[i + 2] >> 6)]);
		out.push_back(base64[(in[i + 2] << 0 & 63)                   ]);
	}

	while (i++ < len)
		out.push_back('=');

	return out;
}

static string generate_boundary() {
	unsigned int nonce[24 / sizeof(unsigned int)];
	for (auto &val: nonce)
		val = rnd();
	return base64_encode(nonce, sizeof nonce);
}

static bool is_boundary(const std::string &line, const std::string &boundary) {
	if (boundary.empty())
		return false;

	if (line.compare(0, 2, "--"))
		return false;

	if (line.compare(2, boundary.size(), boundary))
		return false;

	return true;
}

static bool is_final_boundary(const std::string &line, const std::string &boundary) {
	if (line.compare(2 + boundary.size(), 2, "--"))
		return false;

	return is_boundary(line, boundary);
}

static bool types_match(const std::string &a, const std::string &b) {
	auto a_slash = a.find('/');
	auto b_slash = b.find('/');
	if (a_slash == string::npos || b_slash == string::npos)
		return !a.compare(0, a_slash, b, 0, b_slash);
	else
		return !a.compare(b);
}

static void set_value(string &str, const string &value) {
	size_t semicolon = str.find(';');

	if (semicolon == string::npos)
		str = value;
	else
		str.replace(0, semicolon, value);
}

static string get_value(const string &str) {
	return str.substr(0, str.find(';'));
}

static size_t get_parameter_value_start(const string &str, const string &parameter) {
	size_t pos = 0;

	while((pos = str.find(';', pos)) != string::npos) {
		pos++;
		while (isspace(str[pos]))
			pos++;
		if (str.compare(pos, parameter.size(), parameter))
			continue;
		pos += parameter.size();
		while (isspace(str[pos]))
			pos++;
		if (str[pos] != '=')
			continue;
		pos++;
		while (isspace(str[pos]))
			pos++;
		break;
	}

	return pos;
}

static void set_parameter(string &str, const string &parameter, const string &value) {
	size_t pos = get_parameter_value_start(str, parameter);

	if (pos == string::npos)
		str += "; " + parameter + "=" + value;
	else
		str.replace(pos, str.find(';', pos) - pos, value);
}

static string get_parameter(const string &str, const string &parameter) {
	size_t pos = get_parameter_value_start(str, parameter);

	if (pos == string::npos)
		return {};

	return str.substr(pos, str.find(';', pos) - pos);
}

static const string ending[2] = {"\n", "\r\n"};

Part::Part():
		headers(),
		preamble(),
		body(),
		epilogue(),
		parts(),
		boundary(),
		multipart(false),
		crlf(true),
		message(false)
{}

// Loading and saving a whole MIME message

string Part::load(istream &in, const string &parent_boundary) {
	string line;
	string content_type;
	int ncrlf = 0;
	int nlf = 0;

	while (getline(in, line)) {
		if (is_boundary(line, parent_boundary))
			return line;

		if (line.size() && line.back() == '\r') {
			ncrlf++;
			line.erase(line.size() - 1);
		} else {
			nlf++;
		}

		if (line.empty())
			break;

		if (isspace(line[0])) {
			if (headers.empty())
				throw runtime_error("invalid header line");
			headers.back().second.append("\r\n" + line);
			continue;
		}

		size_t colon = string::npos;

		for (size_t i = 0; i < line.size(); ++i) {
			if (line[i] == ':') {
				colon = i;
				break;
			}

			if (line[i] < 33 || static_cast<uint8_t>(line[i]) > 127)
				throw runtime_error("invalid header line");
		}

		if (colon == 0 || colon == string::npos)
			throw runtime_error("invalid header line");

		auto start = colon + 1;
		while (start < line.size() && isspace(line[start]))
			start++;

		if (start >= line.size())
			throw runtime_error("invalid header line");

		auto field = line.substr(0, colon);
		auto value = line.substr(start);

		headers.emplace_back(field, value);

		if (field == "Content-Type")
			content_type = value;
	}

	crlf = ncrlf > nlf;

	if (types_match(content_type, "multipart")) {
		boundary = get_parameter(content_type, "boundary");
		if (boundary.empty())
			throw runtime_error("multipart but no boundary specified");
		multipart = true;
	} else {
		multipart = false;
	}

	if (!multipart) {
		while (getline(in, line)) {
			if (is_boundary(line, parent_boundary))
				return line;
			line.push_back('\n');
			body.append(line);
		}
	} else {
		while (getline(in, line)) {
			if (is_boundary(line, parent_boundary))
				return line;
			if (is_boundary(line, boundary))
				break;
			line.push_back('\n');
			preamble.append(line);
		}

		while (true) {
			parts.emplace_back();
			string last_line = parts.back().load(in, boundary);
			if (!is_boundary(last_line, boundary))
				throw runtime_error("invalid boundary");
			if (is_final_boundary(last_line, boundary))
				break;
		}

		while (getline(in, line)) {
			if (is_boundary(line, parent_boundary))
				return line;
			line.push_back('\n');
			epilogue.append(line);
		}
	}

	return {};
}

void Part::save(ostream &out) const {
	bool has_headers = false;

	for (auto &header: headers) {
		if (!header.second.empty()) {
			out << header.first << ": " << header.second << ending[crlf];
			has_headers = true;
		}
	}

	if (message && !has_headers)
		throw runtime_error("no headers specified");

	out << ending[crlf];

	if (parts.empty()) {
		out << body;
	} else {
		out << preamble;
		for (auto &part: parts) {
			out << "--" << boundary << ending[crlf];
			part.save(out);
		}
		out << "--" << boundary << "--" << ending[crlf];
		out << epilogue;
	}
}

void Part::load(const string &filename) {
	ifstream in(filename);
	load(in);
}

void Part::save(const string &filename) const {
	ofstream out(filename);
	save(out);
}

void Part::from_string(const string &data) {
	stringstream in(data, ios_base::in);
	load(in);
}

string Part::to_string() const {
	stringstream out(ios_base::out);
	save(out);
	return out.str();
}

// Low-level access

string Part::get_body() const {
	return body;
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

vector<pair<string, string>> &Part::get_headers() {
	return headers;
}

const vector<pair<string, string>> &Part::get_headers() const {
	return headers;
}

bool Part::is_multipart() const {
	return multipart;
}

void Part::set_body(const string &value) {
	if (multipart)
		throw runtime_error("Cannot set body of a multipart message");
	body = value;
}

void Part::set_preamble(const string &value) {
	if (!multipart)
		throw runtime_error("Cannot set preamble of a non-multipart message");
	preamble = value;
}

void Part::set_epilogue(const string &value) {
	if (!multipart)
		throw runtime_error("Cannot set epilogue of a non-multipart message");
	epilogue = value;
}

void Part::set_boundary(const std::string &value) {
	boundary = value;
	if (!get_header("Content-Type").empty())
		set_header_parameter("Content-Type", "boundary", boundary);
}

void Part::set_parts(const vector<Part> &value) {
	if (!multipart)
		throw runtime_error("Cannot set parts of a non-multipart message");
	parts = value;
}

void Part::set_headers(const vector<pair<string, string>> &value) {
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

// Header manipulation

static bool iequals(const string &a, const string &b) {
	if (a.size() != b.size())
		return false;

	for (size_t i = 0; i < a.size(); ++i)
		if (tolower(a[i]) != tolower(b[i]))
			return false;

	return true;
}

string Part::get_header(const string &field) const {
	for (const auto &header: headers)
		if (iequals(header.first, field))
			return header.second;

	return {};
}

void Part::set_header(const string &field, const string &value) {
	for (auto &header: headers) {
		if (iequals(header.first, field)) {
			header.second = value;
			return;
		}
	}

	append_header(field, value);
}

string &Part::operator[](const string &field) {
	for (auto &header: headers)
		if (iequals(header.first, field))
			return header.second;

	append_header(field, {});
	return headers.back().second;
}

const string &Part::operator[](const string &field) const {
	for (auto &header: headers)
		if (iequals(header.first, field))
			return header.second;

	static string empty_string;
	return empty_string;
}

void Part::append_header(const string &field, const string &value) {
	headers.push_back(make_pair(field, value));
}


void Part::prepend_header(const string &field, const string &value) {
	headers.insert(begin(headers), make_pair(field, value));
}


void Part::erase_header(const string &field) {
	headers.erase(remove_if(begin(headers), end(headers), [&](pair<string, string> &header){
		return header.first == field;
	}), end(headers));
}

void Part::clear_headers() {
	headers.clear();
}

string Part::get_header_value(const string &field) const {
	return get_value(get_header(field));
}

string Part::get_header_parameter(const string &field, const string &parameter) const {
	return get_parameter(get_header(field), parameter);
}

void Part::set_header_value(const string &field, const string &value) {
	for (auto &header: headers) {
		if (iequals(header.first, field)) {
			set_value(header.second, value);
			return;
		}
	}

	append_header(field, value);
}

void Part::set_header_parameter(const string &field, const string &parameter, const string &value) {
	for (auto &header: headers) {
		if (iequals(header.first, field)) {
			set_parameter(header.second, parameter, value);
			return;
		}
	}

	append_header(field, "; " + parameter + "=" + value);
}

// Part manipulation

Part &Part::append_part(const Part &part) {
	parts.push_back(part);
	return parts.back();
}

Part &Part::prepend_part(const Part &part) {
	parts.insert(begin(parts), part);
	return parts.front();
}

void Part::clear_parts() {
	parts.clear();
}

void Part::make_multipart(const string &subtype, const string &suggested_boundary) {
	if (multipart) {
		if (get_mime_type() == "multipart/" + subtype)
			return;
		Part part;
		part.preamble = move(preamble);
		part.epilogue = move(epilogue);
		part.parts = move(parts);
		part.boundary = move(boundary);
		part.multipart = true;
		part.set_header("Content-Type", get_header("Content-Type"));
		part.set_header("Content-Disposition", get_header("Content-Disposition"));
		set_header("Content-Disposition", {});
		part.crlf = crlf;
		parts.emplace_back(move(part));
	} else {
		multipart = true;

		if (message)
			set_header("MIME-Version", "1.0");

		if (!body.empty()) {
			auto &part = append_part();
			part.set_header("Content-Type", get_header("Content-Type"));
			part.set_header("Content-Disposition", get_header("Content-Disposition"));
			set_header("Content-Disposition", {});
			part.body = move(body);
		}
	}

	if (!suggested_boundary.empty())
		set_boundary(suggested_boundary);
	if (boundary.empty())
		boundary = generate_boundary();

	set_header("Content-Type", "multipart/" + subtype + "; boundary=" + boundary);
}

bool Part::make_singlepart() {
	if (!multipart)
		return true;

	if (parts.size() > 1)
		return false;

	multipart = false;

	if (parts.empty())
		return true;

	auto &part = parts.front();
	set_header("Content-Type", part.get_header("Content-Type"));
	set_body(part.get_body());
	parts.clear();

	return true;
}

// Body and attachments

string Part::get_mime_type() const {
	return get_header_value("Content-Type");
}

const Part *Part::get_first_matching_part(const string &type) const {
	if (!multipart) {
		if (get_header_value("Content-Disposition") == "attachment")
			return nullptr;
		auto my_type = get_mime_type();
		if (types_match(my_type.empty() ? "text/plain" : my_type, type))
			return this;
	} else {
		for (auto &part: parts) {
			auto result = part.get_first_matching_part(type);
			if (result)
				return result;
		}
	}

	return nullptr;
}

Part *Part::get_first_matching_part(const string &type) {
	auto result = ((const Part *)this)->get_first_matching_part(type);
	return const_cast<Part *>(result);
}

string Part::get_first_matching_body(const string &type) const {
	const auto &part = get_first_matching_part(type);
	if (part)
		return part->get_body();
	else
		return {};
}

Part &Part::set_alternative(const string &subtype, const string &text) {
	string type = "text/" + subtype;
	Part *part = nullptr;

	// Try to put it in the body first.
	if (!multipart) {
		if (body.empty() || get_header_value("Content-Type") == type) {
			part = this;
		} else if (types_match(get_header_value("Content-Type"), "text") && get_header_value("Content-Disposition") != "attachment") {
			make_multipart("alternative");
			part = &append_part();
		} else {
			make_multipart("mixed");
			part = &prepend_part();
		}
	} else {
		// If there is already a text/plain part, use that one.
		part = get_first_matching_part(type);
		if (part) {
			part->set_header_value("Content-Type", type);
			part->set_body(text);
			return *part;
		}

		// If there is already a multipart/alternative with text, use that one.
		part = get_first_matching_part("multipart/alternative");
		if (part && part->get_first_matching_part("text"))
			part = &part->append_part();

		// If there is already inline text, make it multipart/alternative.

		if (!part && (part = get_first_matching_part("text"))) {
			part->make_multipart("alternative");
			part = &part->append_part();
		}

		// Otherwise, assume we're multipart/mixed.
		if (!part)
			part = &prepend_part();
	}

	part->set_header("Content-Type", type);
	part->set_body(text);

	return *part;
}

void Part::set_plain(const string &text) {
	set_alternative("plain", text);
}

void Part::set_html(const string &html) {
	set_alternative("html", html);
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
	if (!multipart && body.empty()) {
		if (attachment.message)
			set_header("Content-Type", "message/rfc822");
		else
			set_header("Content-Type", attachment.get_header("Content-Type"));
		body = attachment.to_string();
		return *this;
	}

	make_multipart("mixed");
	append_part(attachment);
	return parts.back();
}

Part &Part::attach(const string &data, const string &type, const string &filename) {
	if (!multipart && body.empty()) {
		set_header("Content-Type", type.empty() ? "text/plain" : type);
		set_header("Content-Disposition", "attachment");
		if (!filename.empty())
			set_header_parameter("Content-Disposition", "filename", filename);
		body = data;
		return *this;
	}

	make_multipart("mixed");
	auto &part = append_part();
	part.set_header("Content-Type", type.empty() ? "text/plain" : type);
	part.set_header("Content-Disposition", "attachment");
	if (!filename.empty())
		part.set_header_parameter("Content-Disposition", "filename", filename);
	part.set_body(data);
	return part;
}

Part &Part::attach(istream &in, const string &type, const string &filename) {
	string foo;
	(void)in;
	return attach(foo, type, filename);
}

vector<const Part *> Part::get_attachments() const {
	vector<const Part *> attachments;

	if (!multipart && get_header_value("Content-Disposition") == "attachment") {
		attachments.push_back(this);
		return attachments;
	}

	for (auto &part: parts) {
		auto sub = part.get_attachments();
		attachments.insert(end(attachments), begin(sub), end(sub));
	}

	return attachments;
}

void Part::clear_attachments() {
}

void Part::clear_text() {
}

void Part::clear_plain() {
}

void Part::clear_html() {
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
	return false;
}

// RFC2822 messages

Message::Message() {
	message = true;
}

}
