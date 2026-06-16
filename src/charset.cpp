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
#include <stdexcept>
using namespace std;

#ifdef _WIN32

#include <windows.h>
#include <string>

class iconv_state {
public:
	UINT from_cp;
	UINT to_cp;

	iconv_state(const char* tocode, const char* fromcode) {
		from_cp = codepage_from_name(fromcode);
		to_cp = codepage_from_name(tocode);
		if (from_cp == 0 || to_cp == 0)
			throw std::runtime_error("Unsupported character set");
	}

	size_t convert(char** inbuf, size_t* inbytesleft,
		char** outbuf, size_t* outbytesleft)
	{
		if (!inbuf || !*inbuf)
			return 0;

		// 1. from multibyte → UTF-16
		int wlen = MultiByteToWideChar(
			from_cp, 0,
			*inbuf, (int)*inbytesleft,
			nullptr, 0);

		if (wlen <= 0)
			return (size_t)-1;

		std::wstring wide(wlen, L'\0');
		MultiByteToWideChar(
			from_cp, 0,
			*inbuf, (int)*inbytesleft,
			&wide[0], wlen);

		// 2. UTF-16 → target multibyte
		int outlen = WideCharToMultiByte(
			to_cp, 0,
			wide.data(), wlen,
			nullptr, 0,
			nullptr, nullptr);

		if ((size_t)outlen > *outbytesleft)
			return (size_t)-1;

		WideCharToMultiByte(
			to_cp, 0,
			wide.data(), wlen,
			*outbuf, outlen,
			nullptr, nullptr);

		*inbuf += *inbytesleft;
		*outbuf += outlen;
		*outbytesleft -= outlen;
		*inbytesleft = 0;

		return outlen;
	}

private:
	static UINT codepage_from_name(const char* name) {
		// Unicode encodings
		if (_stricmp(name, "UTF-8") == 0) return CP_UTF8;
		if (_stricmp(name, "UTF-16LE") == 0) return 1200;
		if (_stricmp(name, "UTF-16BE") == 0) return 1201;
		if (_stricmp(name, "UTF-16") == 0) return 1200;
		if (_stricmp(name, "UTF-32LE") == 0) return 12000;
		if (_stricmp(name, "UTF-32BE") == 0) return 12001;
		if (_stricmp(name, "UTF-32") == 0) return 12000;
		// Chinese encodings
		if (_stricmp(name, "GBK") == 0) return 936;
		if (_stricmp(name, "GB2312") == 0) return 936;
		if (_stricmp(name, "GB18030") == 0) return 936;
		if (_stricmp(name, "GB18030-2000") == 0) return 54936;
		if (_stricmp(name, "BIG5") == 0) return 950;
		if (_stricmp(name, "ASCII") == 0) return 20127;
		if (_stricmp(name, "US-ASCII") == 0) return 20127;

		return 0;
	}
};
/// @brief utf8 std::string  to wstring utf16le in windows
/// @param utf8_str ptr
/// @return wstring
std::wstring UTF8ToWString(const std::string& utf8_str) {
	int wstr_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
	if (wstr_len == 0) {// error
		return L"";
	}
	std::wstring wstr(wstr_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wstr[0], wstr_len);
	return wstr;
}


#else 
#include <iconv.h>

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

#endif


#include<vector>
string charset_decode(const string &charset, string_view in) {
	iconv_state cd("utf-8", charset.c_str());

	string out;
	out.reserve(in.size() * 2); //Avoid insufficient space
	//避免空间不足

	char *inbuf = const_cast<char *>(in.data());
	size_t inbytesleft = in.size();

	std::vector<char> buf(in.size() * 2, 0);//Avoid insufficient stack space
	char *outbuf;
	outbuf = &buf[0];

	size_t outbytesleft;
	outbytesleft = buf.size();

	while (inbytesleft) {
		char *outbuf_start = outbuf;
		size_t outbytesleft_start = outbytesleft;
		size_t result = cd.convert(&inbuf, &inbytesleft, &outbuf, &outbytesleft);
		if (result == (size_t)-1) {
			if (errno != E2BIG)
				throw runtime_error("Character set conversion error");
		}
		out.append(outbuf_start, outbytesleft_start - outbytesleft);
	}

	return out;
}
