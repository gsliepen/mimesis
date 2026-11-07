#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include <mimesis.hpp>

// Uses the installed header and the shared library only, to check that the public API is exported.
TEST_CASE("The public API is usable from the shared library") {
	Mimesis::Message message;
	message.set_header_value("Subject", "Café");
	message.set_plain("Hello!\r\n");
	message.attach("data\r\n", "application/octet-stream", "data.bin");

	std::stringstream stream;
	stream << message;
	Mimesis::Message reparsed;
	stream >> reparsed;

	CHECK(reparsed == message);
	CHECK(reparsed.get_header_value("Subject") == "Café");
	CHECK(reparsed.get_plain() == "Hello!\r\n");
	CHECK(reparsed.get_attachments().size() == 1);
}
