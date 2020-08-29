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

#include "base64.hpp"

using namespace std;

static const string base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const signed char base64_inverse[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

string base64_encode(string_view in) {
	string out;
	size_t outlen = ((in.size() + 2) / 3) * 4;
	out.reserve(outlen);

	size_t i;
	const uint8_t *uin = (const uint8_t *)in.data();

	for (i = 0; i < (in.size() / 3) * 3; i += 3) {
		out.push_back(base64[                         (uin[i + 0] >> 2)]);
		out.push_back(base64[(uin[i + 0] << 4 & 63) | (uin[i + 1] >> 4)]);
		out.push_back(base64[(uin[i + 1] << 2 & 63) | (uin[i + 2] >> 6)]);
		out.push_back(base64[(uin[i + 2] << 0 & 63)                    ]);
	}

	while (i++ < in.size())
		out.push_back('=');

	return out;
}

string base64_decode(string_view in) {
	string out;
	size_t estimate = (in.size() / 4) * 3;
	out.reserve(estimate);

	int i = 0;
	uint32_t triplet = 0;

	for(uint8_t c: in) {
		auto d = base64_inverse[c];
		if (d == -1) {
			if (c == '=')
				break;
			else
				continue;
		}

		triplet <<= 6;
		triplet |= d;

		if((i & 3) == 3) {
			out.push_back(static_cast<char>(triplet >> 16));
			out.push_back(static_cast<char>(triplet >> 8));
			out.push_back(static_cast<char>(triplet));
		}

		i++;
	}

	if((i & 3) == 3) {
		out.push_back(static_cast<char>(triplet >> 10));
		out.push_back(static_cast<char>(triplet >> 2));
	} else if((i & 3) == 2) {
		out.push_back(static_cast<char>(triplet >> 4));
	}

	return out;
}
