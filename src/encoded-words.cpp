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

using namespace std;

namespace Mimesis {

namespace {

struct DecodedWord {
	string text;
	size_t length;
};

struct WordRange {
	size_t begin;
	size_t end;
};

}

// Parses an encoded word of the form =?charset?encoding?payload?= at the start of text.
static optional<DecodedWord> decode_encoded_word(string_view text) {
	if (!text.starts_with("=?"))
		return nullopt;

	auto charset_end = text.find('?', 2);
	if (charset_end == string_view::npos || charset_end + 2 >= text.size() || text[charset_end + 2] != '?')
		return nullopt;

	auto payload_begin = charset_end + 3;
	auto payload_end = text.find("?=", payload_begin);
	if (payload_end == string_view::npos)
		return nullopt;

	auto charset = text.substr(2, charset_end - 2);
	charset = charset.substr(0, charset.find('*')); // Strip the RFC 2231 language suffix.
	auto encoding = to_lower(text[charset_end + 1]);
	auto payload = text.substr(payload_begin, payload_end - payload_begin);

	if (charset.empty() || ranges::any_of(payload, is_space))
		return nullopt;

	string decoded;
	if (encoding == 'b')
		decoded = base64_decode(payload);
	else if (encoding == 'q')
		decoded = q_decode(payload);
	else
		return nullopt;

	try {
		return DecodedWord{charset_decode(charset, decoded), payload_end + 2};
	} catch (const runtime_error &) {
		return nullopt;
	}
}

string decode_encoded_words(string_view text) {
	string result;
	bool after_encoded_word = false;

	while (!text.empty()) {
		auto start = text.find("=?");
		auto literal = text.substr(0, start);

		if (start == string_view::npos) {
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
		if (!after_encoded_word || !ranges::all_of(literal, is_space))
			result += literal;

		result += word->text;
		text.remove_prefix(start + word->length);
		after_encoded_word = true;
	}

	return result;
}

// Splits UTF-8 text into encoded words of at most 75 characters each.
static string make_encoded_words(string_view text) {
	static constexpr size_t max_bytes_per_word = 45; // Encodes to 60 base64 characters.
	string result;

	while (!text.empty()) {
		auto length = min(text.size(), max_bytes_per_word);
		while (length > 0 && length < text.size() && (static_cast<unsigned char>(text[length]) & 0xc0) == 0x80)
			length--;
		if (length == 0)
			length = min(text.size(), max_bytes_per_word);

		if (!result.empty())
			result += ' ';
		result += "=?utf-8?b?" + base64_encode(text.substr(0, length)) + "?=";
		text.remove_prefix(length);
	}

	return result;
}

string encode_encoded_words(string_view text) {
	vector<WordRange> words;
	for (size_t i = 0; i < text.size();) {
		if (is_space(text[i])) {
			i++;
			continue;
		}
		size_t begin = i;
		while (i < text.size() && !is_space(text[i]))
			i++;
		words.push_back({begin, i});
	}

	auto needs_encoding = [&](const WordRange &range) {
		auto word = text.substr(range.begin, range.end - range.begin);
		return !is_ascii(word) || word.find("=?") != string_view::npos;
	};

	string result;
	size_t copied = 0;

	// Consecutive words that need encoding are combined, including the whitespace between them.
	for (size_t first = 0; first < words.size(); first++) {
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
