#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <stdexcept>
#include <string>

#include <mimesis.hpp>

TEST_CASE("Raw header access") {
	Mimesis::Part part;
	CHECK(part.get_headers().empty());

	// Set using []
	part["foo"] = "bar";
	CHECK(part["foo"] == "bar");
	CHECK(part.get_header("foo") == "bar");
	CHECK(part.get_header_value("foo") == "bar");
	CHECK(part.get_header_parameter("foo", "").empty());
	CHECK(part.get_header_parameter("foo", "foo").empty());
	CHECK(part.to_string() == "foo: bar\r\n\r\n");

	// Set using set_header
	part.set_header("baz", "quux");
	CHECK(part["baz"] == "quux");
	CHECK(part.get_header("baz") == "quux");
	CHECK(part.to_string() == "foo: bar\r\nbaz: quux\r\n\r\n");

	// Change existing
	part.set_header("foo", "bar2");
	CHECK(part["foo"] == "bar2");
	CHECK(part.get_header("FOO") == "bar2");

	part.append_header("insert", "back");
	CHECK(part.get_header("insert") == "back");
	part.prepend_header("insert", "front");
	CHECK(part.get_header("insert") == "front");

	auto headers = part.get_headers();
	REQUIRE(headers.size() == 4);
	CHECK(headers[0] == Mimesis::Header{"insert", "front"});
	CHECK(headers[1].first == "foo");
	CHECK(headers[2].first == "baz");
	CHECK(headers[3] == Mimesis::Header{"insert", "back"});

	// Setting an empty value deletes the header
	part.set_header("foo", {});
	CHECK(part.get_header("foo").empty());
	CHECK(part.to_string() ==
			"insert: front\r\n"
			"baz: quux\r\n"
			"insert: back\r\n"
			"\r\n");

	// Erase multiple headers
	part.erase_header("insert");
	CHECK(part.to_string() == "baz: quux\r\n\r\n");

	// Const header access
	const auto &const_part = part;
	CHECK(const_part["baz"] == "quux");
	CHECK(const_part["insert"].empty());
	CHECK(const_part.get_headers().size() == 1);

	part.clear_headers();
	CHECK(part.get_headers().empty());
	CHECK(part.to_string() == "\r\n");

	// Transplant headers
	part.set_header("From", "me");
	{
		Mimesis::Part other;
		other.set_header("To", "you");
		other.set_header("From", "us");
		part.set_headers(other.get_headers());
	}
	CHECK(part.get_headers().size() == 2);
	CHECK(part.get_header("From") == "us");
	CHECK(part.get_header("To") == "you");
}

TEST_CASE("Values and parameters of MIME headers") {
	Mimesis::Part part;
	part["Content-Type"] = "text/plain";

	part.set_header_parameter("Content-Type", "charset", "us-ascii");
	CHECK(part.get_header("Content-Type") == "text/plain; charset=us-ascii");
	CHECK(part.get_header_value("Content-Type") == "text/plain");
	CHECK(part.get_header_parameter("Content-Type", "charset") == "us-ascii");

	part.set_header_parameter("Content-Type", "charset", "utf-8");
	part.set_header_parameter("Content-Type", "format", "flowed");
	CHECK(part.get_header("Content-Type") == "text/plain; charset=utf-8; format=flowed");

	part.set_header_value("Content-Type", "text/html");
	CHECK(part.get_header("Content-Type") == "text/html; charset=utf-8; format=flowed");

	// Parameter on a missing header
	part.set_header_parameter("Content-Disposition", "filename", "foo");
	CHECK(part.get_header("Content-Disposition") == "; filename=foo");
	CHECK(part.get_header_value("Content-Disposition").empty());

	part.set_header_value("Content-Disposition", "attachment");
	CHECK(part.get_header("Content-Disposition") == "attachment; filename=foo");

	part.set_header_value("Content-Language", "en");
	CHECK(part.get_header("Content-Language") == "en");
}

TEST_CASE("Values of other headers are not split at semicolons") {
	Mimesis::Part part;
	part["Subject"] = "Re: a; b";
	CHECK(part.get_header_value("Subject") == "Re: a; b");

	part.set_header_value("Subject", "x; y");
	CHECK(part.get_header("Subject") == "x; y");
}

TEST_CASE("erase_header is case-insensitive") {
	Mimesis::Part part;
	part["content-disposition"] = "inline";
	part["Other"] = "kept";
	part.erase_header("Content-Disposition");
	CHECK(part.to_string() == "Other: kept\r\n\r\n");
}

TEST_CASE("RFC 2047 encoded header values are decoded") {
	Mimesis::Part part;
	auto decode = [&](std::string raw) {
		part.set_header("Subject", raw);
		return part.get_header_value("Subject");
	};

	CHECK(decode("=?utf-8?q?caf=C3=A9?=") == "café");
	CHECK(decode("=?ISO-8859-1?Q?caf=E9?=") == "café");
	CHECK(decode("=?UTF-8?B?Y2Fmw6k=?=") == "café");
	CHECK(decode("=?utf-8*en?q?caf=C3=A9?=") == "café");
	CHECK(decode("=?utf-8?q?Hello_World?=") == "Hello World");
	CHECK(decode("Re: =?utf-8?b?Y2Fmw6k=?= ok") == "Re: café ok");

	SECTION("whitespace between adjacent encoded words is dropped") {
		CHECK(decode("=?utf-8?q?a?= =?utf-8?q?b?=") == "ab");
		CHECK(decode("=?utf-8?q?a?=\t =?utf-8?q?b?=") == "ab");
		CHECK(decode("=?utf-8?q?a?= x =?utf-8?q?b?=") == "a x b");
	}

	SECTION("malformed encoded words are kept literally") {
		CHECK(decode("=?utf-8?x?abc?= rest") == "=?utf-8?x?abc?= rest");
		CHECK(decode("=? foo") == "=? foo");
		CHECK(decode("a =?x-no-such-charset?q?x?= b") == "a =?x-no-such-charset?q?x?= b");
		CHECK(decode("=?utf-8?q?a b?=") == "=?utf-8?q?a b?=");
		CHECK(decode("=?utf-8?q?ok?= =?bad") == "ok =?bad");
	}
}

TEST_CASE("Non-ASCII header values are encoded using RFC 2047") {
	Mimesis::Part part;

	part.set_header_value("Subject", "Hello");
	CHECK(part.get_header("Subject") == "Hello");

	part.set_header_value("From", "Jürgen Müller <j@example.org>");
	auto raw = part.get_header("From");
	CHECK(raw.starts_with("=?utf-8?b?"));
	CHECK(raw.ends_with("?= <j@example.org>"));
	CHECK(part.get_header_value("From") == "Jürgen Müller <j@example.org>");

	std::string long_text;
	for (int i = 0; i < 100; i++)
		long_text += "é";
	part.set_header_value("Subject", "Long " + long_text + " text");
	raw = part.get_header("Subject");
	std::istringstream words(raw);
	for (std::string word; words >> word;)
		CHECK(word.size() <= 75);
	CHECK(part.get_header_value("Subject") == "Long " + long_text + " text");

	part.set_header_value("Subject", "looks =?like?= an encoded word");
	CHECK(part.get_header_value("Subject") == "looks =?like?= an encoded word");
}

TEST_CASE("Header injection is rejected") {
	Mimesis::Part part;
	CHECK_THROWS_AS(part.set_header("Subject", "a\r\nBcc: x@example.org"), std::invalid_argument);
	CHECK_THROWS_AS(part.set_header("Subject", "a\nb"), std::invalid_argument);
	CHECK_THROWS_AS(part.set_header("Subject", "a\rb"), std::invalid_argument);
	CHECK_THROWS_AS(part.set_header("Subject", "a\r\n"), std::invalid_argument);
	CHECK_THROWS_AS(part.set_header("Bad Field", "x"), std::invalid_argument);
	CHECK_THROWS_AS(part.set_header("Bad:Field", "x"), std::invalid_argument);
	CHECK_THROWS_AS(part.append_header("", "x"), std::invalid_argument);
	CHECK_THROWS_AS(part.prepend_header("X", "a\nb"), std::invalid_argument);
	CHECK_THROWS_AS(part["Bad Field"], std::invalid_argument);
	CHECK(part.get_headers().empty());

	// Folded lines are allowed
	CHECK_NOTHROW(part.set_header("Subject", "a\r\n b"));
	CHECK_NOTHROW(part.set_header("Subject", "a\n\tb"));

	// Values written directly are checked when saving
	part["Subject"] = "a\r\nBcc: x@example.org";
	CHECK_THROWS_AS(part.to_string(), std::invalid_argument);
}

TEST_CASE("Empty header values are preserved") {
	Mimesis::Part part;
	std::string message = "X-Empty:\r\nA: b\r\n\r\nbody\r\n";
	part.from_string(message);
	CHECK(part.get_headers().size() == 2);
	CHECK(part.to_string() == message);
}
