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

#include "encoded-words.hpp"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <vector>

#include "base64.hpp"
#include "charset.hpp"
#include "quoted-printable.hpp"
#include "string-utils.hpp"


namespace Mimesis {

namespace {

struct DecodedWord {
	std::string text;
	std::size_t length;
};

struct WordRange {
	std::size_t begin;
	std::size_t end;
};

}

// Parses an encoded word of the form =?charset?encoding?payload?= at the start of text.
static std::optional<DecodedWord> decode_encoded_word(std::string_view text) {
	if (!text.starts_with("=?"))
		return std::nullopt;

	auto charset_end = text.find('?', 2);
	if (charset_end == std::string_view::npos || charset_end + 2 >= text.size() || text[charset_end + 2] != '?')
		return std::nullopt;

	auto payload_begin = charset_end + 3;
	auto payload_end = text.find("?=", payload_begin);
	if (payload_end == std::string_view::npos)
		return std::nullopt;

	auto charset = text.substr(2, charset_end - 2);
	charset = charset.substr(0, charset.find('*')); // Strip the RFC 2231 language suffix.
	auto encoding = to_lower(text[charset_end + 1]);
	auto payload = text.substr(payload_begin, payload_end - payload_begin);

	if (charset.empty() || std::ranges::any_of(payload, is_space))
		return std::nullopt;

	std::string decoded;
	if (encoding == 'b')
		decoded = base64_decode(payload);
	else if (encoding == 'q')
		decoded = q_decode(payload);
	else
		return std::nullopt;

	try {
		return DecodedWord{charset_decode(charset, decoded), payload_end + 2};
	} catch (const std::runtime_error &) {
		return std::nullopt;
	}
}

std::string decode_encoded_words(std::string_view text) {
	std::string result;
	bool after_encoded_word = false;

	while (!text.empty()) {
		auto start = text.find("=?");
		auto literal = text.substr(0, start);

		if (start == std::string_view::npos) {
			result += literal;
			break;
		}

		auto word = decode_encoded_word(text.substr(start));
		if (!word) {
			result += text.substr(0, start + 2);
			text.remove_prefix(start + 2);
			after_encoded_word = false;
			continue;
		}

		// Whitespace between two encoded words is not part of the text.
		if (!after_encoded_word || !std::ranges::all_of(literal, is_space))
			result += literal;

		result += word->text;
		text.remove_prefix(start + word->length);
		after_encoded_word = true;
	}

	return result;
}

// Splits UTF-8 text into encoded words of at most 75 characters each.
static std::string make_encoded_words(std::string_view text) {
	static constexpr std::size_t max_bytes_per_word = 45; // Encodes to 60 base64 characters.
	std::string result;

	while (!text.empty()) {
		auto length = std::min(text.size(), max_bytes_per_word);
		while (length > 0 && length < text.size() && (static_cast<unsigned char>(text[length]) & 0xc0) == 0x80)
			length--;
		if (length == 0)
			length = std::min(text.size(), max_bytes_per_word);

		if (!result.empty())
			result += ' ';
		result += "=?utf-8?b?" + base64_encode(text.substr(0, length)) + "?=";
		text.remove_prefix(length);
	}

	return result;
}

std::string encode_encoded_words(std::string_view text) {
	std::vector<WordRange> words;
	for (std::size_t i = 0; i < text.size();) {
		if (is_space(text[i])) {
			i++;
			continue;
		}
		std::size_t begin = i;
		while (i < text.size() && !is_space(text[i]))
			i++;
		words.push_back({begin, i});
	}

	auto needs_encoding = [&](const WordRange &range) {
		auto word = text.substr(range.begin, range.end - range.begin);
		return !is_ascii(word) || word.find("=?") != std::string_view::npos;
	};

	std::string result;
	std::size_t copied = 0;

	// Consecutive words that need encoding are combined, including the whitespace between them.
	for (std::size_t first = 0; first < words.size(); first++) {
		if (!needs_encoding(words[first]))
			continue;

		auto last = first;
		while (last + 1 < words.size() && needs_encoding(words[last + 1]))
			last++;

		result += text.substr(copied, words[first].begin - copied);
		result += make_encoded_words(text.substr(words[first].begin, words[last].end - words[first].begin));
		copied = words[last].end;
		first = last;
	}

	result += text.substr(copied);
	return result;
}

}
