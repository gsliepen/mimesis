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
#include <sstream>
#include <stdexcept>

using namespace std;

namespace Mimesis {

static const string ending[2] = {"\n", "\r\n"};

MIMEPart::MIMEPart(): multipart(false), crlf(true) {}

// Loading and saving a whole MIME message

string MIMEPart::load(istream &in, const string &parent_boundary) {
	string line;
	string *content_type = nullptr;
	int ncrlf = 0;
	int nlf = 0;

	while (getline(in, line)) {
		if (!parent_boundary.empty() && line.find(parent_boundary) == 0)
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

			if (line[i] < 33 || !(line[i] <= 127))
				throw runtime_error("invalid header line");
		}

		if (colon == 0 || colon == string::npos)
			throw runtime_error("invalid header line");

		auto start = colon + 1;
		while (start < line.size() && isspace(line[start]))
			start++;

		if (start >= line.size())
			throw runtime_error("invalid header line");

		headers.emplace_back(line.substr(0, colon), line.substr(start));

		if (headers.back().first == "Content-Type")
			content_type = &headers.back().second;
	}

	crlf = ncrlf > nlf;

	if (content_type && content_type->substr(0, 10) == "multipart/") {
		auto b = content_type->find("boundary=");
		if (b == string::npos)
			throw runtime_error("multipart but no boundary specified");
		boundary = content_type->substr(b + 9);
		if (boundary[0] == '"')
			boundary = "--" + boundary.substr(1, boundary.size() - 2);
		else
			boundary = "--" + boundary;
		multipart = true;
	} else {
		multipart = false;
	}

	if (!multipart) {
		while (getline(in, line)) {
			if (!parent_boundary.empty() && line.find(parent_boundary) == 0)
				return line;
			line.push_back('\n');
			body.append(line);
		}
	} else {
		while (getline(in, line)) {
			if (!parent_boundary.empty() && line.find(parent_boundary) == 0)
				return line;
			if (line.find(boundary) == 0)
				break;
			line.push_back('\n');
			preamble.append(line);
		}

		while (true) {
			parts.emplace_back();
			string child_boundary = parts.back().load(in, boundary);
			if (child_boundary.find(boundary + "--") == 0)
				break;
			if (child_boundary.find(boundary) == 0)
				continue;
			throw runtime_error("invalid boundary");
		}

		while (getline(in, line)) {
			if (!parent_boundary.empty() && line.find(parent_boundary) == 0)
				return line;
			line.push_back('\n');
			epilogue.append(line);
		}
	}

	return {};
}

void MIMEPart::save(ostream &out) const {
	bool has_headers = false;

	for (auto &header: headers) {
		if (!header.second.empty()) {
			out << header.first << ": " << header.second << ending[crlf];
			has_headers = true;
		}
	}

	if (!has_headers)
		throw runtime_error("no headers specified");

	out << ending[crlf];

	if (parts.empty()) {
		out << body;
	} else {
		out << preamble;
		for (auto &part: parts) {
			out << boundary << ending[crlf];
			part.save(out);
		}
		out << boundary << "--" << ending[crlf];
		out << epilogue;
	}
}

void MIMEPart::load(const string &filename) {
	ifstream in(filename);
	load(in);
}

void MIMEPart::save(const string &filename) const {
	ofstream out(filename);
	save(out);
}

void MIMEPart::from_string(const string &data) {
	stringstream in(data, ios_base::in);
	load(in);
}

string MIMEPart::to_string() const {
	stringstream out(ios_base::out);
	save(out);
	return out.str();
}

// Low-level access

string MIMEPart::get_body() const {
	return body;
}

string MIMEPart::get_preamble() const {
	return preamble;
}

string MIMEPart::get_epilogue() const {
	return epilogue;
}

string MIMEPart::get_boundary() const {
	return boundary;
}

vector<MIMEPart> &MIMEPart::get_parts() {
	return parts;
}

const vector<MIMEPart> &MIMEPart::get_parts() const {
	return parts;
}

vector<pair<string, string>> &MIMEPart::get_headers() {
	return headers;
}

const vector<pair<string, string>> &MIMEPart::get_headers() const {
	return headers;
}

bool MIMEPart::is_multipart() const {
	return multipart;
}

void MIMEPart::set_body(const string &value) {
	if (multipart)
		throw runtime_error("Cannot set body of a multipart message");
	body = value;
}

void MIMEPart::set_preamble(const string &value) {
	if (!multipart)
		throw runtime_error("Cannot set preamble of a non-multipart message");
	preamble = value;
}

void MIMEPart::set_epilogue(const string &value) {
	if (!multipart)
		throw runtime_error("Cannot set epilogue of a non-multipart message");
	epilogue = value;
}

void MIMEPart::set_boundary(const std::string &value) {
	boundary = "--" + value;
}

void MIMEPart::set_parts(const vector<MIMEPart> &value) {
	if (!multipart)
		throw runtime_error("Cannot set parts of a non-multipart message");
	parts = value;
}

void MIMEPart::set_headers(const vector<pair<string, string>> &value) {
	headers = value;
}

void MIMEPart::clear() {
	headers.clear();
	preamble.clear();
	body.clear();
	epilogue.clear();
	parts.clear();
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

string MIMEPart::get_header(const string &field) const {
	for (const auto &header: headers)
		if (iequals(header.first, field))
			return header.second;

	return {};
}

void MIMEPart::set_header(const string &field, const string &value) {
	for (auto &header: headers) {
		if (iequals(header.first, field)) {
			header.second = value;
			return;
		}
	}

	append_header(field, value);
}

string &MIMEPart::operator[](const string &field) {
	for (auto &header: headers)
		if (iequals(header.first, field))
			return header.second;

	append_header(field, {});
	return headers.back().second;
}

const string &MIMEPart::operator[](const string &field) const {
	for (auto &header: headers)
		if (iequals(header.first, field))
			return header.second;

	static string empty_string;
	return empty_string;
}

void MIMEPart::append_header(const string &field, const string &value) {
	headers.push_back(make_pair(field, value));
}


void MIMEPart::prepend_header(const string &field, const string &value) {
	headers.insert(begin(headers), make_pair(field, value));
}


void MIMEPart::erase_header(const string &field) {
	headers.erase(remove_if(begin(headers), end(headers), [&](pair<string, string> &header){
		return header.first == field;
	}));
}

// Part manipulation

MIMEPart &MIMEPart::append_part(const MIMEPart &part) {
	parts.push_back(part);
	return parts.back();
}

MIMEPart &MIMEPart::prepend_part(const MIMEPart &part) {
	parts.insert(begin(parts), part);
	return parts.front();
}

void MIMEPart::remove_all_parts() {
	parts.clear();
}

void MIMEPart::make_multipart(const string &type) {
	if (multipart)
		return;

	multipart = true;

	set_header("MIME-Version", "1.0");
	set_header("Content-Type", "multipart/" + type);

	if (!body.empty()) {
		auto part = append_part({});
		part.set_header("Content-Type", part.get_header("Content-Type"));
		part.set_body(get_body());
		body.clear();
	}
}

bool MIMEPart::make_singlepart() {
	if (!multipart)
		return true;

	if (parts.size() > 1)
		return false;

	multipart = false;

	if (parts.empty())
		return true;

	auto part = parts.front();
	set_header("Content-Type", part.get_header("Content-Type"));
	set_body(part.get_body());
	parts.clear();

	return true;
}

}
