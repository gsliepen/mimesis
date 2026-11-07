#pragma once

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

#include <string>
#include <string_view>

namespace Mimesis {

// Line breaks (LF or CRLF) in the input are written as line_ending.
std::string quoted_printable_encode(std::string_view in, std::string_view line_ending = "\r\n");
std::string quoted_printable_decode(std::string_view in);

// The "Q" encoding of RFC 2047 encoded words, where an underscore represents a space.
std::string q_decode(std::string_view in);

}
