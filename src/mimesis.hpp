#pragma once

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

/// \file
/// The Mimesis API, consisting of the Mimesis::Part and Mimesis::Message classes.

#include <chrono>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(__GNUC__)
#define MIMESIS_EXPORT __attribute__((visibility("default")))
#else
#define MIMESIS_EXPORT
#endif

/// Parsing and building RFC 2822 messages with MIME content.
///
/// Parsing a message, and getting its text and attachments:
/// \code
/// Mimesis::Message message;
/// message.load("message.eml");
/// std::cout << message.get_header_value("Subject") << "\n";
/// std::cout << message.get_text();
/// for (auto attachment: message.get_attachments())
///     std::cout << attachment->get_header_parameter("Content-Disposition", "filename") << "\n";
/// \endcode
///
/// Building a message:
/// \code
/// Mimesis::Message message;
/// message.set_header_value("From", "Jürgen <j@example.org>");
/// message.set_header_value("To", "someone@example.org");
/// message.set_header_value("Subject", "Report");
/// message.set_date();
/// message.generate_msgid("example.org");
/// message.set_plain("Here is the report.\r\n");
/// message.set_html("<p>Here is the <b>report</b>.</p>\r\n");
/// message.attach(report_data, "application/pdf", "report.pdf");
/// message.save("message.eml");
/// \endcode
namespace Mimesis {

/// A header field name and its raw value.
using Header = std::pair<std::string, std::string>;

/// A MIME part: headers, followed by either a body, or by a preamble, child parts and an epilogue.
///
/// The content is stored as it appears in the message. Low-level functions like get_header() and
/// set_body() access it unmodified. High-level functions like get_header_value(), get_body(),
/// set_plain() and attach() take care of encoding, character sets and the multipart structure.
///
/// Field names, MIME types and parameter names are case-insensitive.
class MIMESIS_EXPORT Part {
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
	void set_encoded_body(std::string_view transfer_encoding, std::string encoded_body);
	void set_text_body(std::string_view text);
	void set_attachment_body(std::string_view data);
	void add_mime_version();
	bool ends_with_newline() const;

	template <typename Self>
	static Self *find_first_matching_part(Self &self, const std::function<bool(const Part &)> &predicate);

	protected:
	/// Whether this is a Message.
	bool message = false;

	public:
	/// Compares all content, including the line ending style and whether it is a Message.
	/// \param other The part to compare with.
	/// \return Whether both parts are equal.
	bool operator==(const Part &other) const = default;

	/// \name Loading and saving
	/// @{

	/// Parses a message, replacing the current content.
	///
	/// The line ending style is detected from the headers. Truncated multipart content is accepted.
	/// \param in The stream to read the message from.
	/// \throws std::runtime_error if a header line is invalid or reading fails.
	void load(std::istream &in);

	/// Parses a message from a file, replacing the current content.
	/// \param filename The file to read the message from.
	/// \throws std::runtime_error if the file cannot be read or a header line is invalid.
	void load(const std::filesystem::path &filename);

	/// Writes the message.
	/// \param out The stream to write the message to.
	/// \throws std::runtime_error if this is a Message without headers.
	/// \throws std::invalid_argument if a header field name is invalid, or a value contains a line break
	/// that is not followed by whitespace.
	void save(std::ostream &out) const;

	/// Writes the message to a file.
	/// \param filename The file to write the message to.
	/// \throws std::runtime_error if the file cannot be written, or for the reasons save(std::ostream &) throws.
	void save(const std::filesystem::path &filename) const;

	/// Parses a message from a string, see load(std::istream &).
	/// \param data The complete message.
	void from_string(std::string_view data);

	/// Converts the message to a string, see save(std::ostream &).
	/// \return The complete message.
	std::string to_string() const;

	/// @}

	/// \name Low-level access
	/// @{

	/// Gets the body with its Content-Transfer-Encoding decoded.
	///
	/// The body of text parts is converted to UTF-8 if the Content-Type has another charset.
	/// \return The decoded body.
	/// \throws std::runtime_error if the charset is unknown or the text is invalid.
	std::string get_body() const;

	/// Gets the raw text in front of the first child part.
	/// \return The preamble.
	std::string get_preamble() const;

	/// Gets the raw text after the last child part.
	/// \return The epilogue.
	std::string get_epilogue() const;

	/// Gets the boundary between child parts.
	/// \return The boundary, or an empty string if this is not multipart.
	std::string get_boundary() const;

	/// Gets the child parts.
	/// \return The child parts, which are empty if this is not multipart.
	std::vector<Part> &get_parts();

	/// Gets the child parts.
	/// \return The child parts, which are empty if this is not multipart.
	const std::vector<Part> &get_parts() const;

	/// Gets all headers in order.
	/// \return The headers, with their raw values.
	std::vector<Header> &get_headers();

	/// Gets all headers in order.
	/// \return The headers, with their raw values.
	const std::vector<Header> &get_headers() const;

	/// Checks whether this part is multipart.
	/// \return Whether this part has child parts instead of a body.
	bool is_multipart() const;

	/// Checks whether this part is multipart with the given subtype.
	/// \param subtype The multipart subtype, for example "alternative".
	/// \return Whether this part is multipart with the given subtype.
	bool is_multipart(std::string_view subtype) const;

	/// Checks whether this part is a single part.
	/// \return Whether this part has a body instead of child parts.
	bool is_singlepart() const;

	/// Checks whether this part is a single part with the given MIME type.
	/// \param type A full MIME type like "text/plain", or only the top-level type like "text".
	/// \return Whether this part has a body and the given MIME type.
	bool is_singlepart(std::string_view type) const;

	/// Sets the raw body, without encoding it.
	///
	/// Use set_plain(), set_html() or attach() to have the body encoded as needed.
	/// \param body The body, as it should appear in the message.
	/// \throws std::runtime_error if this is multipart.
	void set_body(std::string_view body);

	/// Sets the raw text in front of the first child part.
	/// \param preamble The preamble, as it should appear in the message.
	/// \throws std::runtime_error if this is not multipart.
	void set_preamble(std::string_view preamble);

	/// Sets the raw text after the last child part.
	/// \param epilogue The epilogue, as it should appear in the message.
	/// \throws std::runtime_error if this is not multipart.
	void set_epilogue(std::string_view epilogue);

	/// Sets the boundary between child parts, and updates the Content-Type header if there is one.
	/// \param boundary The new boundary.
	void set_boundary(std::string_view boundary);

	/// Replaces the child parts.
	/// \param parts The new child parts.
	/// \throws std::runtime_error if this is not multipart.
	void set_parts(const std::vector<Part> &parts);

	/// Replaces all headers.
	/// \param headers The new headers, with raw values.
	void set_headers(const std::vector<Header> &headers);

	/// Removes all headers and content. Whether this is a Message and the line ending style are kept.
	void clear();

	/// Removes the body.
	void clear_body();

	/// @}

	/// \name Raw header access
	/// These functions read and write header values exactly as they appear in the message.
	/// @{

	/// Gets the raw value of the first header with the given field name.
	/// \param field The field name.
	/// \return The raw value, or an empty string if there is no such header.
	std::string get_header(std::string_view field) const;

	/// Sets the raw value of the first header with the given field name, or appends a new header.
	/// \param field The field name.
	/// \param value The raw value. If empty, all headers with the given field name are removed.
	/// \throws std::invalid_argument if the field name is invalid, or the value contains a line break
	/// that is not followed by whitespace.
	void set_header(std::string_view field, std::string_view value);

	/// Accesses the raw value of the first header with the given field name, appending an empty header if needed.
	///
	/// Values assigned this way are checked when the message is saved.
	/// \param field The field name.
	/// \return A reference to the raw value.
	/// \throws std::invalid_argument if the field name is invalid.
	std::string &operator[](std::string_view field);

	/// Accesses the raw value of the first header with the given field name.
	/// \param field The field name.
	/// \return A reference to the raw value, or to an empty string if there is no such header.
	const std::string &operator[](std::string_view field) const;

	/// Appends a header, even if there already is one with the same field name.
	/// \param field The field name.
	/// \param value The raw value.
	/// \throws std::invalid_argument for the same reasons as set_header().
	void append_header(std::string_view field, std::string_view value);

	/// Prepends a header, even if there already is one with the same field name.
	/// \param field The field name.
	/// \param value The raw value.
	/// \throws std::invalid_argument for the same reasons as set_header().
	void prepend_header(std::string_view field, std::string_view value);

	/// Removes all headers with the given field name.
	/// \param field The field name.
	void erase_header(std::string_view field);

	/// Removes all headers.
	void clear_headers();

	/// @}

	/// \name Decoded header access
	/// @{

	/// Gets the decoded value of the first header with the given field name.
	///
	/// For Content-Type and Content-Disposition, this is the value in front of the parameters, for
	/// example "text/plain". For other fields, it is the whole value with RFC 2047 encoded words
	/// decoded to UTF-8.
	/// \param field The field name.
	/// \return The decoded value, or an empty string if there is no such header.
	std::string get_header_value(std::string_view field) const;

	/// Sets the value of the first header with the given field name, or appends a new header.
	///
	/// For Content-Type and Content-Disposition, only the value in front of the parameters is replaced.
	/// For other fields, words that are not ASCII are encoded using RFC 2047, so addresses stay readable.
	/// \param field The field name.
	/// \param value The value, in UTF-8.
	/// \throws std::invalid_argument for the same reasons as set_header().
	void set_header_value(std::string_view field, std::string_view value);

	/// Gets the decoded value of a parameter, for example the charset of the Content-Type header.
	///
	/// RFC 2231 encoded and continued parameters are supported, as are RFC 2047 encoded words in
	/// quoted values.
	/// \param field The field name of the header.
	/// \param parameter The parameter name.
	/// \return The decoded value, or an empty string if the header or parameter does not exist.
	std::string get_header_parameter(std::string_view field, std::string_view parameter) const;

	/// Sets the value of a parameter, quoting or RFC 2231 encoding it as needed.
	///
	/// Adds the header if it does not exist yet.
	/// \param field The field name of the header.
	/// \param parameter The parameter name.
	/// \param value The value, in UTF-8.
	void set_header_parameter(std::string_view field, std::string_view parameter, std::string_view value);

	/// Gets the subtype of a multipart part.
	/// \return The subtype, for example "mixed", or an empty string if this is not multipart.
	std::string get_multipart_type() const;

	/// Prepends a Received header.
	/// \param text The text in front of the date, for example "from host.example.org by example.org".
	/// \param date The time the message was received.
	void add_received(std::string_view text, std::chrono::system_clock::time_point date = std::chrono::system_clock::now());

	/// Sets the Message-ID header to a random, unique identifier.
	/// \param domain The domain name to use in the identifier.
	void generate_msgid(std::string_view domain);

	/// Sets the Date header. The date is written in UTC.
	/// \param date The date of the message.
	void set_date(std::chrono::system_clock::time_point date = std::chrono::system_clock::now());

	/// @}

	/// \name Part manipulation
	/// @{

	/// Appends an empty child part. This part should be multipart, see make_multipart().
	/// \return The new child part.
	Part &append_part();

	/// Appends a copy of a child part. This part should be multipart, see make_multipart().
	/// \param part The part to copy.
	/// \return The new child part.
	Part &append_part(const Part &part);

	/// Prepends an empty child part. This part should be multipart, see make_multipart().
	/// \return The new child part.
	Part &prepend_part();

	/// Prepends a copy of a child part. This part should be multipart, see make_multipart().
	/// \param part The part to copy.
	/// \return The new child part.
	Part &prepend_part(const Part &part);

	/// Removes all child parts.
	void clear_parts();

	/// Turns this into a multipart part with the given subtype.
	///
	/// A non-empty body becomes the first child part, together with the Content-* headers. If this is
	/// already multipart with another subtype, the current parts move into a new child part. Nothing
	/// happens if it already has the given subtype.
	/// \param subtype The multipart subtype, for example "mixed".
	/// \param boundary The boundary to use. If empty, a random boundary is generated.
	void make_multipart(std::string_view subtype, std::string_view boundary = {});

	/// Replaces a multipart part that has a single child part by the content of that child part.
	///
	/// A multipart part without child parts becomes an empty single part.
	/// \return false if there is more than one child part, true otherwise.
	bool flatten();

	/// Gets the MIME type from the Content-Type header.
	/// \return The MIME type without its parameters, for example "text/plain".
	std::string get_mime_type() const;

	/// Sets the MIME type in the Content-Type header, keeping its parameters.
	/// \param type The MIME type, for example "text/plain".
	void set_mime_type(std::string_view type);

	/// Checks whether this part has the given MIME type.
	/// \param type A full MIME type like "text/plain", or only the top-level type like "text".
	/// \return Whether the MIME type matches.
	bool is_mime_type(std::string_view type) const;

	/// Checks whether this part has a MIME type.
	/// \return Whether this part has a Content-Type header.
	bool has_mime_type() const;

	/// @}

	/// \name Body and attachments
	/// @{

	/// Sets the text for the given subtype of text, replacing existing text of that subtype.
	///
	/// The text is added as an alternative to other text, and in front of attachments, creating
	/// multipart/alternative and multipart/mixed parts as needed. Line breaks are converted to the
	/// line ending style of the part, see set_crlf(). Text that is not ASCII or that has lines longer
	/// than 78 characters is encoded as quoted-printable or base64.
	/// \param subtype The subtype of text, for example "plain" or "html".
	/// \param text The text, in UTF-8.
	/// \return The part that holds the text.
	Part &set_alternative(std::string_view subtype, std::string_view text);

	/// Sets the plain text, see set_alternative().
	/// \param text The text, in UTF-8.
	void set_plain(std::string_view text);

	/// Sets the HTML text, see set_alternative().
	/// \param text The HTML, in UTF-8.
	void set_html(std::string_view text);

	/// Searches this part and its child parts, depth first, skipping attachments and empty parts.
	/// \param predicate The function that decides whether a part matches.
	/// \return The first matching part, or nullptr.
	const Part *get_first_matching_part(const std::function<bool(const Part &)> &predicate) const;

	/// Searches this part and its child parts, depth first, skipping attachments and empty parts.
	/// \param predicate The function that decides whether a part matches.
	/// \return The first matching part, or nullptr.
	Part *get_first_matching_part(const std::function<bool(const Part &)> &predicate);

	/// Searches for the first part with the given MIME type, see get_first_matching_part(const std::function<bool(const Part &)> &) const.
	///
	/// Parts without a Content-Type header are considered to be text/plain.
	/// \param type A full MIME type like "text/plain", or only the top-level type like "text".
	/// \return The first matching part, or nullptr.
	const Part *get_first_matching_part(std::string_view type) const;

	/// Searches for the first part with the given MIME type, see get_first_matching_part(const std::function<bool(const Part &)> &).
	///
	/// Parts without a Content-Type header are considered to be text/plain.
	/// \param type A full MIME type like "text/plain", or only the top-level type like "text".
	/// \return The first matching part, or nullptr.
	Part *get_first_matching_part(std::string_view type);

	/// Gets the body of the first part with the given MIME type, see get_first_matching_part(std::string_view) const.
	/// \param type A full MIME type like "text/plain", or only the top-level type like "text".
	/// \return The decoded body, or an empty string if there is no such part.
	std::string get_first_matching_body(std::string_view type) const;

	/// Gets the first text that is not an attachment.
	/// \return The text in UTF-8, or an empty string if there is none.
	std::string get_text() const;

	/// Gets the plain text.
	/// \return The text in UTF-8, or an empty string if there is none.
	std::string get_plain() const;

	/// Gets the HTML text.
	/// \return The HTML in UTF-8, or an empty string if there is none.
	std::string get_html() const;

	/// Attaches a copy of a part. A Message is attached as message/rfc822.
	///
	/// If this part is empty, it becomes the attachment itself. Otherwise it is made multipart/mixed.
	/// \param attachment The part to attach.
	/// \return The attachment.
	Part &attach(const Part &attachment);

	/// Attaches data, encoding it as base64 if necessary.
	///
	/// The data is kept exactly. Only text that already has the line ending style of the part,
	/// consists of ASCII lines of at most 78 characters, and ends with a line break is not encoded.
	/// If this part is empty, it becomes the attachment itself. Otherwise it is made multipart/mixed.
	/// \param data The content of the attachment.
	/// \param mime_type The MIME type of the data. If empty, text/plain is used.
	/// \param filename The file name to suggest to the recipient, if not empty.
	/// \return The attachment.
	Part &attach(std::string_view data, std::string_view mime_type, std::string_view filename = {});

	/// Attaches all data read from a stream, see attach(std::string_view, std::string_view, std::string_view).
	/// \param in The stream to read the content of the attachment from.
	/// \param mime_type The MIME type of the data. If empty, text/plain is used.
	/// \param filename The file name to suggest to the recipient, if not empty.
	/// \return The attachment.
	Part &attach(std::istream &in, std::string_view mime_type, std::string_view filename = {});

	/// Gets all attachments.
	/// \return The parts with an attachment Content-Disposition.
	std::vector<const Part *> get_attachments() const;

	/// Removes all text of the given MIME type that is not an attachment, then calls simplify().
	/// \param type A full MIME type like "text/plain", or only the top-level type like "text".
	void clear_alternative(std::string_view type);

	/// Removes all text that is not an attachment.
	void clear_text();

	/// Removes the plain text.
	void clear_plain();

	/// Removes the HTML text.
	void clear_html();

	/// Removes all attachments, then calls simplify().
	void clear_attachments();

	/// Removes empty child parts, and flattens multipart parts with only one child part.
	void simplify();

	/// Checks whether there is text.
	/// \return Whether there is text that is not an attachment.
	bool has_text() const;

	/// Checks whether there is plain text.
	/// \return Whether there is plain text that is not an attachment.
	bool has_plain() const;

	/// Checks whether there is HTML text.
	/// \return Whether there is HTML text that is not an attachment.
	bool has_html() const;

	/// Checks whether there are attachments.
	/// \return Whether this part or any of its child parts is an attachment.
	bool has_attachments() const;

	/// Checks whether this part is an attachment.
	/// \return Whether the Content-Disposition of this part is attachment.
	bool is_attachment() const;

	/// Checks whether this part is inline.
	/// \return Whether the Content-Disposition of this part is inline.
	bool is_inline() const;

	/// @}

	/// \name Line endings
	/// @{

	/// Sets the line endings used when saving.
	///
	/// This affects the headers and boundaries of this part, and bodies that are encoded afterwards.
	/// Child parts and existing bodies are not changed.
	/// \param value true for CRLF line endings, the default, or false for LF line endings.
	void set_crlf(bool value = true);

	/// @}
};

/// A complete message.
///
/// Unlike a Part, a Message must have headers to be saved, gets a MIME-Version header when the
/// high-level functions add MIME content, and is attached to other parts as message/rfc822.
class MIMESIS_EXPORT Message: public Part {
	public:
	/// Creates an empty message.
	Message();
};

/// Writes a part to a stream, see Part::save(std::ostream &).
/// \param out The stream to write to.
/// \param part The part to write.
/// \return The stream.
MIMESIS_EXPORT std::ostream &operator<<(std::ostream &out, const Part &part);

/// Reads a part from a stream, see Part::load(std::istream &).
/// \param in The stream to read from.
/// \param part The part to read into.
/// \return The stream.
MIMESIS_EXPORT std::istream &operator>>(std::istream &in, Part &part);
}
