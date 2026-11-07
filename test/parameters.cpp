#include <catch2/catch_test_macros.hpp>

#include "parameters.hpp"

using namespace Mimesis;

TEST_CASE("Parameters without a value don't cause out-of-bounds reads") {
	CHECK(get_parameter("text/plain; flowed", "charset") == "");
	CHECK(get_parameter("text/plain; flowed;", "charset") == "");
	CHECK(get_parameter("text/plain;", "charset") == "");
	CHECK(get_parameter("text/plain; charset=", "charset") == "");
	CHECK(get_parameter("text/plain; charset", "charset") == "");
	CHECK(get_parameter("text/plain; charset=\"unterminated", "charset") == "unterminated");
	CHECK(get_parameter("text/plain; charset=\"escape\\", "charset") == "escape\\");
}

TEST_CASE("Parameters after a valueless parameter are found") {
	CHECK(get_parameter("text/plain; flowed; charset=utf-8", "charset") == "utf-8");
	CHECK(get_parameter("text/plain; format=flowed; charset=utf-8", "charset") == "utf-8");
	CHECK(get_parameter("multipart/mixed; foo; boundary=abc", "boundary") == "abc");
}

TEST_CASE("Quoted parameter values are skipped as a whole") {
	auto header = "attachment; filename=\"a;b=c\"; size=3";
	CHECK(get_parameter(header, "size") == "3");
	CHECK(get_parameter(header, "filename") == "a;b=c");
	CHECK(get_parameter("attachment; filename=\"a\\\"b\"; size=3", "filename") == "a\"b");
	CHECK(get_parameter("attachment; filename=\"a\\\"b\"; size=3", "size") == "3");
}

TEST_CASE("Parameter names are matched exactly and case-insensitively") {
	CHECK(get_parameter("text/plain; xcharset=a; charset=b", "charset") == "b");
	CHECK(get_parameter("text/plain; charsetx=a; charset=b", "charset") == "b");
	CHECK(get_parameter("text/plain; CHARSET=b", "charset") == "b");
	CHECK(get_parameter("text/plain ;  charset = \"utf-8\" ", "charset") == "utf-8");
}

TEST_CASE("Setting parameters") {
	CHECK(set_parameter("text/plain", "charset", "utf-8") == "text/plain; charset=utf-8");
	CHECK(set_parameter("", "charset", "utf-8") == "; charset=utf-8");
	CHECK(set_parameter("text/plain; charset=ascii; format=flowed", "charset", "utf-8") == "text/plain; charset=utf-8; format=flowed");
	CHECK(set_parameter("text/plain; flowed", "charset", "utf-8") == "text/plain; flowed; charset=utf-8");
	CHECK(set_parameter("a; filename*0=x; filename*1=y; size=1", "filename", "z") == "a; filename=z; size=1");
	CHECK(set_parameter("a; filename=\"x;y\"; size=1", "filename", "z") == "a; filename=z; size=1");
}

TEST_CASE("Parameter values are quoted using the RFC 2045 token rules") {
	CHECK(quote_token("abc-123") == "abc-123");
	CHECK(quote_token("a/b") == "\"a/b\"");
	CHECK(quote_token("a=b") == "\"a=b\"");
	CHECK(quote_token("a b") == "\"a b\"");
	CHECK(quote_token("") == "\"\"");
	CHECK(quote_token("a\"b\\") == "\"a\\\"b\\\\\"");
	CHECK(set_parameter("multipart/mixed", "boundary", "a/b") == "multipart/mixed; boundary=\"a/b\"");
}

TEST_CASE("RFC 2231 parameters are decoded") {
	CHECK(get_parameter("attachment; filename*=utf-8''caf%C3%A9.txt", "filename") == "café.txt");
	CHECK(get_parameter("attachment; filename*=iso-8859-1'en'caf%E9.txt", "filename") == "café.txt");
	CHECK(get_parameter("attachment; filename*0=\"long \"; filename*1=name.txt", "filename") == "long name.txt");
	CHECK(get_parameter("attachment; filename*0*=utf-8''caf%C3; filename*1*=%A9; filename*2=.txt", "filename") == "café.txt");
	CHECK(get_parameter("attachment; filename=fallback.txt; filename*=utf-8''caf%C3%A9.txt", "filename") == "café.txt");
}

TEST_CASE("RFC 2047 encoded words in quoted parameters are decoded") {
	CHECK(get_parameter("attachment; filename=\"=?utf-8?q?caf=C3=A9.txt?=\"", "filename") == "café.txt");
}

TEST_CASE("Non-ASCII parameter values are encoded using RFC 2231") {
	auto header = set_parameter("attachment", "filename", "café.txt");
	CHECK(header == "attachment; filename*=utf-8''caf%C3%A9.txt");
	CHECK(get_parameter(header, "filename") == "café.txt");
	CHECK(get_parameter(set_parameter("attachment", "filename", "a\r\nb"), "filename") == "a\r\nb");
}

TEST_CASE("The value in front of the parameters") {
	CHECK(get_value(" text/plain ; charset=x") == "text/plain");
	CHECK(get_value("text/plain") == "text/plain");
	CHECK(set_value("a; b=c", "d") == "d; b=c");
	CHECK(set_value("a", "d") == "d");
}
