#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>

#include "base64.hpp"
#include "charset.hpp"
#include "quoted-printable.hpp"
#include "test-utils.hpp"

using namespace Mimesis;
using namespace std::string_literals;

static std::string all_bytes() {
	std::string bytes;
	for (int i = 0; i < 256; i++)
		bytes.push_back(static_cast<char>(i));
	return bytes;
}

TEST_CASE("base64 encodes the RFC 4648 test vectors, including partial groups") {
	CHECK(base64_encode("") == "");
	CHECK(base64_encode("f") == "Zg==");
	CHECK(base64_encode("fo") == "Zm8=");
	CHECK(base64_encode("foo") == "Zm9v");
	CHECK(base64_encode("foob") == "Zm9vYg==");
	CHECK(base64_encode("fooba") == "Zm9vYmE=");
	CHECK(base64_encode("foobar") == "Zm9vYmFy");
}

TEST_CASE("base64 decodes the RFC 4648 test vectors") {
	CHECK(base64_decode("") == "");
	CHECK(base64_decode("Zg==") == "f");
	CHECK(base64_decode("Zm8=") == "fo");
	CHECK(base64_decode("Zm9v") == "foo");
	CHECK(base64_decode("Zm9vYg==") == "foob");
	CHECK(base64_decode("Zm9vYmE=") == "fooba");
	CHECK(base64_decode("Zm9vYmFy") == "foobar");
	CHECK(base64_decode("Zm9v\r\nYmFy\r\n") == "foobar");
}

TEST_CASE("base64 round-trips all byte values at every length") {
	auto bytes = all_bytes();
	for (std::size_t length = 0; length <= bytes.size(); length++)
		REQUIRE(base64_decode(base64_encode(bytes.substr(0, length))) == bytes.substr(0, length));
}

TEST_CASE("base64 wraps lines when asked") {
	auto data = std::string(100, 'x');
	auto encoded = base64_encode(data, 76, "\r\n");
	CHECK(encoded.find("\r\n") == 76);
	CHECK(encoded.ends_with("\r\n"));
	CHECK(all_lines_at_most(encoded, 76));
	CHECK(base64_decode(encoded) == data);
	CHECK(base64_encode("", 76) == "");
}

TEST_CASE("quoted-printable decodes soft line breaks with LF and CRLF") {
	CHECK(quoted_printable_decode("foo=\r\nbar") == "foobar");
	CHECK(quoted_printable_decode("foo=\nbar") == "foobar");
	CHECK(quoted_printable_decode("foo= \t\r\nbar") == "foobar");
	CHECK(quoted_printable_decode("foo\r\nbar\r\n") == "foo\r\nbar\r\n");
	CHECK(quoted_printable_decode("foo=") == "foo");
}

TEST_CASE("quoted-printable accepts lowercase hexadecimal digits") {
	CHECK(quoted_printable_decode("caf=C3=A9") == "caf\xc3\xa9");
	CHECK(quoted_printable_decode("caf=c3=a9") == "caf\xc3\xa9");
}

TEST_CASE("quoted-printable keeps invalid escape sequences literally") {
	CHECK(quoted_printable_decode("a=ZZb") == "a=ZZb");
	CHECK(quoted_printable_decode("a=4Gb") == "a=4Gb");
	CHECK(quoted_printable_decode("a=4") == "a=4");
}

TEST_CASE("quoted-printable encoding") {
	CHECK(quoted_printable_encode("a=b") == "a=3Db");
	CHECK(quoted_printable_encode("caf\xc3\xa9\n", "\n") == "caf=C3=A9\n");
	CHECK(quoted_printable_encode("trailing space \r\nnext\t\r\n") == "trailing space=20\r\nnext=09\r\n");
	CHECK(quoted_printable_encode("lone\rcarriage return") == "lone=0Dcarriage return");

	SECTION("long lines are wrapped with soft line breaks") {
		auto text = std::string(200, 'x') + "\r\n" + std::string(100, '\xe9');
		auto encoded = quoted_printable_encode(text);
		CHECK(all_lines_at_most(encoded, 76));
		CHECK(quoted_printable_decode(encoded) == text);
	}

	SECTION("all byte values except line breaks round-trip") {
		auto bytes = all_bytes();
		std::erase_if(bytes, [](char c) { return c == '\r' || c == '\n'; });
		CHECK(quoted_printable_decode(quoted_printable_encode(bytes)) == bytes);
	}

	SECTION("line breaks are normalized, since it is meant for text") {
		CHECK(quoted_printable_encode("a\nb\r\nc", "\r\n") == "a\r\nb\r\nc");
		CHECK(quoted_printable_encode("a\nb\r\nc", "\n") == "a\nb\nc");
	}
}

TEST_CASE("Q encoding decodes underscores as spaces") {
	CHECK(q_decode("Hello_World=21") == "Hello World!");
	CHECK(q_decode("a=5Fb") == "a_b");
}

TEST_CASE("charset_decode converts input larger than its buffer") {
	auto latin1 = std::string(5000, '\xe9');
	std::string expected;
	for (int i = 0; i < 5000; i++)
		expected += "\xc3\xa9";
	CHECK(charset_decode("iso-8859-1", latin1) == expected);
}

TEST_CASE("charset_decode handles stateful character sets") {
	CHECK(charset_decode("ISO-2022-JP", "\x1b$B$3$s$K$A$O\x1b(B") == "こんにちは");
}

TEST_CASE("charset_decode rejects unknown character sets and invalid input") {
	CHECK_THROWS_AS(charset_decode("x-no-such-charset", "a"), std::runtime_error);
	CHECK_THROWS_AS(charset_decode("utf-8", "\xff"), std::runtime_error);
	CHECK_THROWS_AS(charset_decode("utf-8", "\xc3"), std::runtime_error);
}
