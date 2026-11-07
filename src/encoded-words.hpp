#pragma once

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

#include <string>
#include <string_view>

// RFC 2047 encoded words in header values.

namespace Mimesis {

// Decodes all encoded words to UTF-8. Malformed encoded words are left as they are.
std::string decode_encoded_words(std::string_view text);

// Encodes only the words of UTF-8 text that need it, so ASCII parts like email addresses stay intact.
std::string encode_encoded_words(std::string_view text);

}
