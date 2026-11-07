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

#include <algorithm>
#include <string_view>

// Locale-independent ASCII helpers, as required by the RFCs.

namespace Mimesis {

constexpr bool is_space(char c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

constexpr char to_lower(char c) {
	return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

constexpr bool is_ascii(std::string_view text) {
	return std::ranges::all_of(text, [](unsigned char c) { return c < 128; });
}

constexpr bool iequals(std::string_view a, std::string_view b) {
	return std::ranges::equal(a, b, {}, to_lower, to_lower);
}

constexpr bool istarts_with(std::string_view text, std::string_view prefix) {
	return text.size() >= prefix.size() && iequals(text.substr(0, prefix.size()), prefix);
}

// Returns -1 if c is not a hexadecimal digit.
constexpr int hex_value(char c) {
	c = to_lower(c);
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

constexpr std::string_view trim(std::string_view text) {
	while (!text.empty() && is_space(text.front()))
		text.remove_prefix(1);
	while (!text.empty() && is_space(text.back()))
		text.remove_suffix(1);
	return text;
}

}
