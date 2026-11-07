#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include <mimesis.hpp>

using namespace std;

TEST_CASE("Header manipulation") {
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
	CHECK(part.get_header_value("baz") == "quux");
	CHECK(part.to_string() == "foo: bar\r\nbaz: quux\r\n\r\n");

	// Change existing
	part.set_header("foo", "bar2");
	CHECK(part["foo"] == "bar2");
	CHECK(part.get_header("foo") == "bar2");
	CHECK(part.get_header_value("foo") == "bar2");

	// Set parameter on existing
	part.set_header_parameter("baz", "parameter", "value");
	CHECK(part["baz"] == "quux; parameter=value");
	CHECK(part.get_header("baz") == "quux; parameter=value");
	CHECK(part.get_header_value("baz") == "quux");
	CHECK(part.get_header_parameter("baz", "parameter") == "value");

	// Add parameter on existing
	part.set_header_parameter("baz", "parameter", "value2");
	CHECK(part.get_header("baz") == "quux; parameter=value2");
	CHECK(part.get_header_value("baz") == "quux");
	CHECK(part.get_header_parameter("baz", "parameter") == "value2");

	// Change parameter on existing
	part.set_header_parameter("baz", "foo", "bar");
	CHECK(part.get_header("baz") == "quux; parameter=value2; foo=bar");
	CHECK(part.get_header_value("baz") == "quux");
	CHECK(part.get_header_parameter("baz", "parameter") == "value2");
	CHECK(part.get_header_parameter("baz", "foo") == "bar");

	// Set parameter on empty
	part.set_header_parameter("aap", "noot", "mies");
	CHECK(part.get_header("aap") == "; noot=mies");
	CHECK(part.get_header_value("aap").empty());
	CHECK(part.get_header_parameter("aap", "noot") == "mies");

	// Change value but not parameters
	part.set_header_value("baz", "quux2");
	CHECK(part.get_header("baz") == "quux2; parameter=value2; foo=bar");

	part.set_header_value("a", "b");
	CHECK(part.get_header("a") == "b");

	// Erase header
	part.erase_header("a");
	CHECK(part.get_header("a").empty());

	// Appback header
	part.append_header("insert", "back");
	CHECK(part.get_header("insert") == "back");

	// Prepback header
	part.prepend_header("insert", "front");
	CHECK(part.get_header("insert") == "front");

	// Array access
	auto headers = part.get_headers();
	CHECK(headers.size() == 5);
	for (auto &header: headers) {
		CHECK(!header.first.empty());
		CHECK(!header.second.empty());
	}
	CHECK(headers.front().second == "front");
	CHECK(headers[1].first == "foo");
	CHECK(headers[2].first == "baz");
	CHECK(headers[3].first == "aap");
	CHECK(headers[3].second == "; noot=mies");
	CHECK(headers.back().second == "back");

	// Delete single header
	part.set_header("foo", {});
	CHECK(part.get_header("foo").empty());

	// Intermediate result
	CHECK(part.to_string() ==
			"insert: front\r\n"
			"baz: quux2; parameter=value2; foo=bar\r\n"
			"aap: ; noot=mies\r\n"
			"insert: back\r\n"
			"\r\n");

	// Erase multiple headers
	part.erase_header("insert");
	CHECK(part.get_header("insert").empty());

	// Intermediate result
	CHECK(part.to_string() ==
			"baz: quux2; parameter=value2; foo=bar\r\n"
			"aap: ; noot=mies\r\n"
			"\r\n");

	// Const header access
	const auto &cpart = part;
	CHECK(cpart.get_headers().size() >= 2);
	CHECK(cpart.get_header("baz") == "quux2; parameter=value2; foo=bar");
	CHECK(cpart["aap"] == "; noot=mies");
	CHECK(cpart["insert"].empty());

	// Delete headers
	part.clear_headers();
	CHECK(part.get_headers().empty());
	CHECK(part.get_header("foo").empty());
	CHECK(part.to_string() == "\r\n");

	// Transplant headers
	part.clear();
	part.set_header("From", "me");
	{
		Mimesis::Part other;
		other.set_header("To", "you");
		other.set_header("From", "us");
		part.set_headers(other.get_headers());
		CHECK(other.get_headers().size() == 2);
	}
	CHECK(part.get_headers().size() == 2);
	CHECK(part.get_header("From") == "us");
	CHECK(part.get_header("To") == "you");
}
