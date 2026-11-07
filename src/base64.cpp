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

#include "base64.hpp"

#include <algorithm>
#include <array>
#include <cstdint>


namespace Mimesis {

static constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static constexpr auto inverse_alphabet = [] {
	std::array<std::int8_t, 256> inverse{};
	inverse.fill(-1);
	for (std::size_t i = 0; i < alphabet.size(); i++)
		inverse[static_cast<std::uint8_t>(alphabet[i])] = static_cast<std::int8_t>(i);
	return inverse;
}();

std::string base64_encode(std::string_view in, std::size_t line_length, std::string_view line_ending) {
	std::string out;
	out.reserve((in.size() + 2) / 3 * 4);
	std::size_t column = 0;

	auto emit = [&](char c) {
		if (line_length && column == line_length) {
			out += line_ending;
			column = 0;
		}
		out.push_back(c);
		column++;
	};

	for (std::size_t i = 0; i < in.size(); i += 3) {
		std::size_t available = std::min<std::size_t>(in.size() - i, 3);
		std::uint32_t triplet = 0;
		for (std::size_t j = 0; j < 3; j++)
			triplet = triplet << 8 | (j < available ? static_cast<std::uint8_t>(in[i + j]) : 0);
		for (std::size_t j = 0; j < 4; j++)
			emit(j <= available ? alphabet[(triplet >> (18 - 6 * j)) & 63] : '=');
	}

	if (line_length && !out.empty())
		out += line_ending;

	return out;
}

std::string base64_decode(std::string_view in) {
	std::string out;
	out.reserve(in.size() / 4 * 3);

	std::size_t count = 0;
	std::uint32_t triplet = 0;

	for (std::uint8_t c: in) {
		auto value = inverse_alphabet[c];
		if (value == -1) {
			if (c == '=')
				break;
			continue;
		}

		triplet = triplet << 6 | static_cast<std::uint32_t>(value);

		if (++count % 4 == 0) {
			out.push_back(static_cast<char>(triplet >> 16));
			out.push_back(static_cast<char>(triplet >> 8));
			out.push_back(static_cast<char>(triplet));
		}
	}

	if (count % 4 == 3) {
		out.push_back(static_cast<char>(triplet >> 10));
		out.push_back(static_cast<char>(triplet >> 2));
	} else if (count % 4 == 2) {
		out.push_back(static_cast<char>(triplet >> 4));
	}

	return out;
}

}
