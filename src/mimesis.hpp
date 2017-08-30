#pragma once

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

#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

namespace Mimesis {

class MIMEPart {
	std::vector<std::pair<std::string, std::string>> headers;
	std::string preamble;
	std::string body;
	std::string epilogue;
	std::vector<MIMEPart> parts;
	std::string boundary;
	bool multipart;

	public:
	std::string load(std::istream &in, const std::string &parent_boundary = {});
	void save(std::ostream &out);
};

}
