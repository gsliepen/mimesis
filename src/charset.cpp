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

#include "charset.hpp"

#include <iconv.h>
#include <stdexcept>

using namespace std;

struct iconv_state {
	iconv_t cd;

	iconv_state(const char *tocode, const char *fromcode) {
		cd = iconv_open(tocode, fromcode);
		if (cd == (iconv_t)-1)
			throw runtime_error("Unsupported character set");
	}

	~iconv_state() {
		iconv_close(cd);
	}

	size_t convert(char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft) {
		return ::iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft);
	}
};

string charset_decode(const string &charset, const string &text) {
	iconv_state cd("utf-8", charset.c_str());

	string out;
	out.reserve((text.size() * 102) / 100);

	char *inbuf = const_cast<char *>(text.c_str());
	size_t inbytesleft = text.size();

	char buf[1024];
	char *outbuf;
	size_t outbytesleft;

	while (inbytesleft) {
		outbuf = buf;
		outbytesleft = sizeof outbuf;
		size_t result = cd.convert(&inbuf, &inbytesleft, &outbuf, &outbytesleft);
		if (result == (size_t)-1) {
			if (errno != E2BIG)
				throw runtime_error("Character set conversion error");
		}
		out.append(buf, outbuf - buf);
	}

	return out;
}
