#pragma once

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

#include <string>
#include <string_view>

// Header values with parameters, like "text/plain; charset=utf-8" (RFC 2045, RFC 2231).

namespace Mimesis {

// The part before the first semicolon, without surrounding whitespace.
std::string_view get_value(std::string_view header);
std::string set_value(std::string_view header, std::string_view value);

// Returns the decoded parameter value, or an empty string if it is not present.
std::string get_parameter(std::string_view header, std::string_view name);

// Replaces all existing occurrences of the parameter, or appends it.
std::string set_parameter(std::string_view header, std::string_view name, std::string_view value);

// Quotes the value if it is not a valid RFC 2045 token.
std::string quote_token(std::string_view value);

}
