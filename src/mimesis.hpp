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

#include <chrono>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Mimesis {

using Header = std::pair<std::string, std::string>;

class Part {
	std::vector<Header> headers;
	std::string preamble;
	std::string body;
	std::string epilogue;
	std::vector<Part> parts;
	std::string boundary;
	bool multipart = false;
	bool crlf = true;

	std::string load_part(std::istream &in, std::vector<std::string> &boundaries);
	void move_content_headers_to(Part &destination);
	void erase_content_headers();
	void copy_content_from(const Part &source);
	void set_encoded_body(std::string_view data, bool is_text);
	void add_mime_version();
	bool ends_with_newline() const;

	template <typename Self>
	static Self *find_first_matching_part(Self &self, const std::function<bool(const Part &)> &predicate);

	protected:
	bool message = false;

	public:
	bool operator==(const Part &other) const = default;

	// Loading and saving a whole MIME message
	void load(std::istream &in);
	void load(const std::filesystem::path &filename);
	void save(std::ostream &out) const;
	void save(const std::filesystem::path &filename) const;
	void from_string(std::string_view data);
	std::string to_string() const;

	// Low-level access
	std::string get_body() const;
	std::string get_preamble() const;
	std::string get_epilogue() const;
	std::string get_boundary() const;
	std::vector<Part> &get_parts();
	const std::vector<Part> &get_parts() const;
	std::vector<Header> &get_headers();
	const std::vector<Header> &get_headers() const;
	bool is_multipart() const;
	bool is_multipart(std::string_view subtype) const;
	bool is_singlepart() const;
	bool is_singlepart(std::string_view type) const;

	void set_body(std::string_view body);
	void set_preamble(std::string_view preamble);
	void set_epilogue(std::string_view epilogue);
	void set_boundary(std::string_view boundary);
	void set_parts(const std::vector<Part> &parts);
	void set_headers(const std::vector<Header> &headers);

	void clear();
	void clear_body();

	// Raw header manipulation
	std::string get_header(std::string_view field) const;
	// Setting an empty value erases the header.
	void set_header(std::string_view field, std::string_view value);
	std::string &operator[](std::string_view field);
	const std::string &operator[](std::string_view field) const;
	void append_header(std::string_view field, std::string_view value);
	void prepend_header(std::string_view field, std::string_view value);
	void erase_header(std::string_view field);
	void clear_headers();

	// Decoded header access. For Content-Type and Content-Disposition, the value is the part before
	// the parameters. For other fields it is the whole value, with RFC 2047 encoded words handled.
	std::string get_header_value(std::string_view field) const;
	void set_header_value(std::string_view field, std::string_view value);
	std::string get_header_parameter(std::string_view field, std::string_view parameter) const;
	void set_header_parameter(std::string_view field, std::string_view parameter, std::string_view value);

	std::string get_multipart_type() const;

	void add_received(std::string_view text, std::chrono::system_clock::time_point date = std::chrono::system_clock::now());
	void generate_msgid(std::string_view domain);
	void set_date(std::chrono::system_clock::time_point date = std::chrono::system_clock::now());

	// Part manipulation
	Part &append_part();
	Part &append_part(const Part &part);
	Part &prepend_part();
	Part &prepend_part(const Part &part);
	void clear_parts();
	void make_multipart(std::string_view subtype, std::string_view boundary = {});
	bool flatten();

	std::string get_mime_type() const;
	void set_mime_type(std::string_view type);
	bool is_mime_type(std::string_view type) const;
	bool has_mime_type() const;

	// Body and attachments; text is UTF-8 and is encoded as needed.
	Part &set_alternative(std::string_view subtype, std::string_view text);
	void set_plain(std::string_view text);
	void set_html(std::string_view text);

	const Part *get_first_matching_part(const std::function<bool(const Part &)> &predicate) const;
	Part *get_first_matching_part(const std::function<bool(const Part &)> &predicate);
	const Part *get_first_matching_part(std::string_view type) const;
	Part *get_first_matching_part(std::string_view type);
	std::string get_first_matching_body(std::string_view type) const;
	std::string get_text() const;
	std::string get_plain() const;
	std::string get_html() const;

	Part &attach(const Part &attachment);
	Part &attach(std::string_view data, std::string_view mime_type, std::string_view filename = {});
	Part &attach(std::istream &in, std::string_view mime_type, std::string_view filename = {});
	std::vector<const Part *> get_attachments() const;

	void clear_alternative(std::string_view type);
	void clear_text();
	void clear_plain();
	void clear_html();
	void clear_attachments();

	void simplify();

	bool has_text() const;
	bool has_plain() const;
	bool has_html() const;
	bool has_attachments() const;
	bool is_attachment() const;
	bool is_inline() const;

	// Format manipulation
	void set_crlf(bool value = true);
};

class Message: public Part {
	public:
	Message();
};

std::ostream &operator<<(std::ostream &out, const Part &part);
std::istream &operator>>(std::istream &in, Part &part);

}
