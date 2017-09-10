#include <cassert>
#include <iostream>
#include <stdexcept>

#include <mimesis.hpp>

using namespace std;

int main(int argc, char *argv[]) {
	Mimesis::Part part;

	assert(part.get_headers().empty());

	// Set using []
	part["foo"] = "bar";
	assert(part["foo"] == "bar");
	assert(part.get_header("foo") == "bar");
	assert(part.get_header_value("foo") == "bar");
	assert(part.get_header_parameter("foo", "").empty());
	assert(part.get_header_parameter("foo", "foo").empty());
	assert(part.to_string() == "foo: bar\r\n\r\n");

	// Set using set_header
	part.set_header("baz", "quux");
	assert(part["baz"] == "quux");
	assert(part.get_header("baz") == "quux");
	assert(part.get_header_value("baz") == "quux");
	assert(part.to_string() == "foo: bar\r\nbaz: quux\r\n\r\n");

	// Change existing
	part.set_header("foo", "bar2");
	assert(part["foo"] == "bar2");
	assert(part.get_header("foo") == "bar2");
	assert(part.get_header_value("foo") == "bar2");

	// Set parameter on existing
	part.set_header_parameter("baz", "parameter", "value");
	assert(part["baz"] == "quux; parameter=value");
	assert(part.get_header("baz") == "quux; parameter=value");
	assert(part.get_header_value("baz") == "quux");
	assert(part.get_header_parameter("baz", "parameter") == "value");

	// Add parameter on existing
	part.set_header_parameter("baz", "parameter", "value2");
	assert(part.get_header("baz") == "quux; parameter=value2");
	assert(part.get_header_value("baz") == "quux");
	assert(part.get_header_parameter("baz", "parameter") == "value2");

	// Change parameter on existing
	part.set_header_parameter("baz", "foo", "bar");
	assert(part.get_header("baz") == "quux; parameter=value2; foo=bar");
	assert(part.get_header_value("baz") == "quux");
	assert(part.get_header_parameter("baz", "parameter") == "value2");
	assert(part.get_header_parameter("baz", "foo") == "bar");

	// Set parameter on empty
	part.set_header_parameter("aap", "noot", "mies");
	assert(part.get_header("aap") == "; noot=mies");
	assert(part.get_header_value("aap").empty());
	assert(part.get_header_parameter("aap", "noot") == "mies");

	// Change value but not parameters
	part.set_header_value("baz", "quux2");
	assert(part.get_header("baz") == "quux2; parameter=value2; foo=bar");

	part.set_header_value("a", "b");
	assert(part.get_header("a") == "b");

	// Erase header
	part.erase_header("a");
	assert(part.get_header("a").empty());

	// Appback header
	part.append_header("insert", "back");
	assert(part.get_header("insert") == "back");

	// Prepback header
	part.prepend_header("insert", "front");
	assert(part.get_header("insert") == "front");

	// Array access
	auto headers = part.get_headers();
	assert(headers.size() == 5);
	for (auto &header: headers) {
		assert(!header.first.empty());
		assert(!header.second.empty());
	}
	assert(headers.front().second == "front");
	assert(headers[1].first == "foo");
	assert(headers[2].first == "baz");
	assert(headers[3].first == "aap");
	assert(headers[3].second == "; noot=mies");
	assert(headers.back().second == "back");

	// Delete single header
	part.set_header("foo", {});
	assert(part.get_header("foo").empty());

	// Intermediate result
	assert(part.to_string() ==
			"insert: front\r\n"
			"baz: quux2; parameter=value2; foo=bar\r\n"
			"aap: ; noot=mies\r\n"
			"insert: back\r\n"
			"\r\n");

	// Erase multiple headers
	part.erase_header("insert");
	assert(part.get_header("insert").empty());

	// Intermediate result
	assert(part.to_string() ==
			"baz: quux2; parameter=value2; foo=bar\r\n"
			"aap: ; noot=mies\r\n"
			"\r\n");

	// Const header access
	const auto &cpart = part;
	assert(cpart.get_headers().size() >= 2);
	assert(cpart.get_header("baz") == "quux2; parameter=value2; foo=bar");
	assert(cpart["aap"] == "; noot=mies");
	assert(cpart["insert"].empty());

	// Delete headers
	part.clear_headers();
	assert(part.get_headers().empty());
	assert(part.get_header("foo").empty());
	assert(part.to_string() == "\r\n");
}
