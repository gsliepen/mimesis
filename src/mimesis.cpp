/* Mimesis -- a library for parsing and creating RFC2822 messages
   Copyright © 2017 Guus Sliepen <guus@lightbts.info>
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <stdexcept>

#include "mimesis.hpp"

using namespace std;

namespace Mimesis {

string MIMEPart::load(istream &in, const string &parent_boundary) {
	string line;
	string *content_type = nullptr;

	while (getline(in, line)) {
		if (!parent_boundary.empty() && line.find(parent_boundary) == 0)
			return line;

		if (line.size() && line.back() == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			break;

		if (isspace(line[0])) {
			if (headers.empty())
				throw runtime_error("invalid header line");
			headers.back().second.append("\r\n" + line);
			continue;
		}

		auto colon = line.find(':');
		if (colon == string::npos)
			throw runtime_error("invalid header line");

		auto start = colon + 1;
		while (start < line.size() && isspace(line[start]))
			start++;

		headers.emplace_back(line.substr(0, colon), line.substr(start));

		if (headers.back().first == "Content-Type")
			content_type = &headers.back().second;
	}

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

void MIMEPart::save(ostream &out) {
	for (auto &header: headers)
		out << header.first << ": " << header.second << "\r\n";

	out << "\r\n";

	if (parts.empty()) {
		out << body;
	} else {
		out << preamble;
		for (auto &part: parts) {
			out << boundary << "\r\n";
			part.save(out);
		}
		out << boundary << "--\r\n";
		out << epilogue;
	}
}

}

/* Example usage:

#include <iostream>
int main() {
	Mimesis::MIMEPart p;
	p.load(cin);
	p.save(cout);
}

*/
