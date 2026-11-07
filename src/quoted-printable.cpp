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

#include "quoted-printable.hpp"

#include <algorithm>
#include <format>

#include "string-utils.hpp"


namespace Mimesis {

static constexpr std::size_t max_line_length = 76;

std::string quoted_printable_encode(std::string_view in, std::string_view line_ending) {
	std::string out;
	std::size_t column = 0;

	auto append = [&](std::string_view encoded) {
		// Leave room for the '=' of a soft line break.
		if (column + encoded.size() > max_line_length - 1) {
			out += '=';
			out += line_ending;
			column = 0;
		}
		out += encoded;
		column += encoded.size();
	};

	while (!in.empty()) {
		auto newline = in.find('\n');
		bool has_newline = newline != std::string_view::npos;
		auto line = in.substr(0, newline);
		in.remove_prefix(has_newline ? newline + 1 : in.size());
		if (has_newline && line.ends_with('\r'))
			line.remove_suffix(1);

		for (std::size_t i = 0; i < line.size(); i++) {
			auto c = static_cast<unsigned char>(line[i]);
			bool is_last = i + 1 == line.size();
			bool is_literal = (c >= 33 && c <= 126 && c != '=') || ((c == ' ' || c == '\t') && !is_last);
			if (is_literal)
				append(line.substr(i, 1));
			else
				append(std::format("={:02X}", c));
		}

		if (has_newline) {
			out += line_ending;
			column = 0;
		}
	}

	return out;
}

std::string quoted_printable_decode(std::string_view in) {
	std::string out;
	out.reserve(in.size());

	for (std::size_t i = 0; i < in.size(); i++) {
		if (in[i] != '=') {
			out.push_back(in[i]);
			continue;
		}

		// A soft line break, possibly with trailing whitespace added by a transport.
		auto after_whitespace = in.find_first_not_of(" \t", i + 1);
		if (after_whitespace == std::string_view::npos) {
			i = in.size();
			continue;
		}
		if (in.substr(after_whitespace).starts_with("\r\n")) {
			i = after_whitespace + 1;
			continue;
		}
		if (in[after_whitespace] == '\n') {
			i = after_whitespace;
			continue;
		}

		if (i + 2 < in.size() && hex_value(in[i + 1]) >= 0 && hex_value(in[i + 2]) >= 0) {
			out.push_back(static_cast<char>(hex_value(in[i + 1]) * 16 + hex_value(in[i + 2])));
			i += 2;
			continue;
		}

		// Not a valid escape sequence, keep it literally.
		out.push_back('=');
	}

	return out;
}

std::string q_decode(std::string_view in) {
	std::string text(in);
	std::ranges::replace(text, '_', ' ');
	return quoted_printable_decode(text);
}

}
