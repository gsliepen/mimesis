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

#include "parameters.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>
#include <vector>

#include "charset.hpp"
#include "encoded-words.hpp"
#include "string-utils.hpp"

using namespace std;

namespace Mimesis {

namespace {

struct Parameter {
	string_view name;
	string_view value;
	size_t begin; // Position of the semicolon in front of it.
	size_t end;
};

}

static vector<Parameter> parse_parameters(string_view header) {
	vector<Parameter> parameters;
	auto skip_space = [&](size_t &i) {
		while (i < header.size() && is_space(header[i]))
			i++;
	};

	for (auto semicolon = header.find(';'); semicolon != string_view::npos; semicolon = header.find(';', semicolon)) {
		auto i = semicolon + 1;
		skip_space(i);

		auto name_begin = i;
		while (i < header.size() && header[i] != '=' && header[i] != ';' && !is_space(header[i]))
			i++;
		auto name = header.substr(name_begin, i - name_begin);
		skip_space(i);

		string_view value;
		if (i < header.size() && header[i] == '=') {
			i++;
			skip_space(i);
			auto value_begin = i;
			if (i < header.size() && header[i] == '"') {
				for (i++; i < header.size() && header[i] != '"'; i++)
					if (header[i] == '\\')
						i++;
				i = min(i + 1, header.size());
			} else {
				while (i < header.size() && header[i] != ';' && !is_space(header[i]))
					i++;
			}
			value = header.substr(value_begin, i - value_begin);
		}

		if (!name.empty())
			parameters.push_back({name, value, semicolon, i});

		semicolon = i;
	}

	return parameters;
}

static string unquote(string_view value) {
	if (!value.starts_with('"'))
		return string(value);

	string unquoted;
	for (size_t i = 1; i < value.size() && value[i] != '"'; i++) {
		if (value[i] == '\\' && i + 1 < value.size())
			i++;
		unquoted.push_back(value[i]);
	}
	return unquoted;
}

static string percent_decode(string_view text) {
	string decoded;
	for (size_t i = 0; i < text.size(); i++) {
		if (text[i] == '%' && i + 2 < text.size() && hex_value(text[i + 1]) >= 0 && hex_value(text[i + 2]) >= 0) {
			decoded.push_back(static_cast<char>(hex_value(text[i + 1]) * 16 + hex_value(text[i + 2])));
			i += 2;
		} else {
			decoded.push_back(text[i]);
		}
	}
	return decoded;
}

static const Parameter *find_parameter(const vector<Parameter> &parameters, string_view name) {
	auto it = ranges::find_if(parameters, [&](const Parameter &parameter) { return iequals(parameter.name, name); });
	return it == parameters.end() ? nullptr : &*it;
}

// Splits an RFC 2231 extended value into its charset and the percent-decoded text.
static pair<string, string> decode_extended_value(string_view value) {
	auto charset_end = value.find('\'');
	auto language_end = charset_end == string_view::npos ? string_view::npos : value.find('\'', charset_end + 1);
	if (language_end == string_view::npos)
		return {{}, percent_decode(value)};
	return {string(value.substr(0, charset_end)), percent_decode(value.substr(language_end + 1))};
}

static string convert_to_utf8(const string &charset, string text) {
	if (is_utf8_compatible(charset))
		return text;
	try {
		return charset_decode(charset, text);
	} catch (const runtime_error &) {
		return text;
	}
}

string get_parameter(string_view header, string_view name) {
	auto parameters = parse_parameters(header);
	string name_string(name);

	if (auto extended = find_parameter(parameters, name_string + "*")) {
		auto [charset, text] = decode_extended_value(unquote(extended->value));
		return convert_to_utf8(charset, std::move(text));
	}

	string charset;
	string continued;
	for (size_t index = 0;; index++) {
		auto section_name = format("{}*{}", name, index);
		if (auto section = find_parameter(parameters, section_name + "*")) {
			if (index == 0) {
				auto [section_charset, text] = decode_extended_value(unquote(section->value));
				charset = std::move(section_charset);
				continued += text;
			} else {
				continued += percent_decode(unquote(section->value));
			}
		} else if (auto section = find_parameter(parameters, section_name)) {
			continued += unquote(section->value);
		} else {
			if (index > 0)
				return convert_to_utf8(charset, std::move(continued));
			break;
		}
	}

	if (auto parameter = find_parameter(parameters, name))
		return decode_encoded_words(unquote(parameter->value));

	return {};
}

static bool is_parameter_or_section(string_view parameter, string_view name) {
	return iequals(parameter, name) || (istarts_with(parameter, name) && parameter[name.size()] == '*');
}

static bool is_attribute_char(unsigned char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || string_view("!#$&+-.^_`|~").find(static_cast<char>(c)) != string_view::npos;
}

static string format_parameter(string_view name, string_view value) {
	auto is_printable = [](unsigned char c) { return c >= 32 && c < 127; };
	if (ranges::all_of(value, is_printable))
		return format("{}={}", name, quote_token(value));

	string encoded = format("{}*=utf-8''", name);
	for (unsigned char c: value) {
		if (is_attribute_char(c))
			encoded.push_back(static_cast<char>(c));
		else
			encoded += format("%{:02X}", c);
	}
	return encoded;
}

string set_parameter(string_view header, string_view name, string_view value) {
	auto new_parameter = "; " + format_parameter(name, value);
	string result;
	size_t copied = 0;
	bool replaced = false;

	for (auto &parameter: parse_parameters(header)) {
		if (!is_parameter_or_section(parameter.name, name))
			continue;
		result += header.substr(copied, parameter.begin - copied);
		if (!replaced)
			result += new_parameter;
		replaced = true;
		copied = parameter.end;
	}

	result += header.substr(copied);
	if (!replaced)
		result += new_parameter;

	return result;
}

string_view get_value(string_view header) {
	return trim(header.substr(0, header.find(';')));
}

string set_value(string_view header, string_view value) {
	auto semicolon = header.find(';');
	if (semicolon == string_view::npos)
		return string(value);
	return string(value) + string(header.substr(semicolon));
}

string quote_token(string_view value) {
	static constexpr string_view special_characters = "()<>@,;:\\\"/[]?=";
	auto is_token_char = [](unsigned char c) {
		return c > 32 && c < 127 && special_characters.find(static_cast<char>(c)) == string_view::npos;
	};

	if (!value.empty() && ranges::all_of(value, is_token_char))
		return string(value);

	string quoted = "\"";
	for (auto c: value) {
		if (c == '"' || c == '\\')
			quoted.push_back('\\');
		quoted.push_back(c);
	}
	quoted.push_back('"');
	return quoted;
}

}
