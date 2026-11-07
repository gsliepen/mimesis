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

#include "charset.hpp"

#include <cerrno>
#include <format>
#include <iconv.h>
#include <stdexcept>

#include "string-utils.hpp"

using namespace std;

namespace Mimesis {

namespace {

class Converter {
	iconv_t descriptor;

	public:
	Converter(const string &to, const string &from): descriptor(iconv_open(to.c_str(), from.c_str())) {
		if (descriptor == iconv_t(-1))
			throw runtime_error(format("Unsupported character set \"{}\"", from));
	}

	~Converter() {
		iconv_close(descriptor);
	}

	Converter(const Converter &) = delete;
	Converter &operator=(const Converter &) = delete;

	// Passing null input flushes any pending shift state.
	size_t convert(char **input, size_t *input_left, char **output, size_t *output_left) {
		return iconv(descriptor, input, input_left, output, output_left);
	}
};

}

string charset_decode(string_view charset, string_view text) {
	Converter converter("UTF-8", string(charset));

	string out;
	out.reserve(text.size());

	char *input = const_cast<char *>(text.data());
	size_t input_left = text.size();
	bool flushing = false;

	while (true) {
		char buffer[4096];
		char *output = buffer;
		size_t output_left = sizeof buffer;

		auto result = flushing
			? converter.convert(nullptr, nullptr, &output, &output_left)
			: converter.convert(&input, &input_left, &output, &output_left);
		out.append(buffer, output);

		if (result == static_cast<size_t>(-1)) {
			if (errno == E2BIG)
				continue;
			throw runtime_error(format("Invalid {} text", charset));
		}

		if (flushing)
			return out;

		flushing = true;
	}
}

bool is_utf8_compatible(string_view charset) {
	charset = trim(charset);
	return charset.empty() || iequals(charset, "utf-8") || iequals(charset, "utf8") || iequals(charset, "us-ascii") || iequals(charset, "ascii");
}

}
