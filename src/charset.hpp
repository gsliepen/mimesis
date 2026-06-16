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

#include "string_view.hpp"

std::string charset_decode(const std::string &charset, std::string_view text);

#ifdef _WIN32
/// @brief utf8 std::string  to wstring utf16 in windows
/// @param utf8_str ptr
/// @return wstring
std::wstring UTF8ToWString(const std::string& utf8_str);
#endif