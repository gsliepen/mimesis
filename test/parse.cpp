#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <sstream>
#include <stdexcept>

#include <mimesis.hpp>

#include "test-utils.hpp"

static const char *clean_inputs[] = {
	"01-simple",
	"02-multipart-alternative",
	"03-multipart-mixed",
	"04-nested-multipart",
	"05-nested-message-rfc822",
};

TEST_CASE("Clean messages are reproduced exactly") {
	for (auto name: clean_inputs) {
		INFO(name);
		Mimesis::Part part;
		part.load(test_data_dir / name);
		CHECK(part.to_string() == read_file(test_data_dir / name));
	}
}

TEST_CASE("Messages survive save and load round-trips") {
	auto temporary = std::filesystem::temp_directory_path() / "mimesis-load-save.tmp";

	for (auto name: clean_inputs) {
		INFO(name);
		Mimesis::Message message;
		message.load(test_data_dir / name);

		message.save(temporary);
		Mimesis::Message from_file;
		from_file.load(temporary);
		CHECK(from_file == message);

		Mimesis::Message from_string;
		from_string.from_string(message.to_string());
		CHECK(from_string == message);

		std::stringstream stream;
		stream << message;
		Mimesis::Message from_stream;
		stream >> from_stream;
		CHECK(from_stream == message);
	}

	std::filesystem::remove(temporary);
}

TEST_CASE("Message structure is parsed") {
	Mimesis::Message message;
	message.load(test_data_dir / "04-nested-multipart");
	CHECK(message.is_multipart("mixed"));
	CHECK(message.get_preamble() == "This is the preamble.\r\n");
	CHECK(message.get_epilogue() == "This is the epilogue.\r\n");
	REQUIRE(message.get_parts().size() == 2);

	auto &alternative = message.get_parts()[0];
	CHECK(alternative.is_multipart("alternative"));
	CHECK(alternative.get_boundary() == "xyzzy");
	REQUIRE(alternative.get_parts().size() == 2);
	CHECK(alternative.get_parts()[0].get_body() == "Hello!\r\n");

	auto &attachment = message.get_parts()[1];
	CHECK(attachment.is_attachment());
	CHECK(attachment.get_header_parameter("Content-Disposition", "filename") == "attachment.txt");

	message.load(test_data_dir / "05-nested-message-rfc822");
	CHECK(message.is_singlepart("message/rfc822"));
	Mimesis::Message nested;
	nested.from_string(message.get_body());
	CHECK(nested.get_header("Subject") == "Nested test");
}

TEST_CASE("Loading replaces the previous content") {
	Mimesis::Part part;
	part.from_string("A: 1\n\nfirst\n");
	part.from_string("B: 2\n\nsecond\n");
	CHECK(part.to_string() == "B: 2\n\nsecond\n");
}

TEST_CASE("Quoted-printable bodies are decoded") {
	Mimesis::Part part;
	part.from_string("Content-Type: text/plain\nContent-Transfer-Encoding: quoted-printable\n\nA=3DB soft=\nbreak\n");
	CHECK(part.get_body() == "A=B softbreak\n");
}

TEST_CASE("Base64 bodies are decoded") {
	Mimesis::Part part;
	part.from_string("Content-Type: application/octet-stream\nContent-Transfer-Encoding: Base64\n\naGVs\nbG8=\n");
	CHECK(part.get_body() == "hello");
}

TEST_CASE("Text bodies in other character sets are converted to UTF-8") {
	Mimesis::Part part;
	part.from_string("Content-Type: text/plain; charset=iso-8859-1\n\ncaf\xe9\n");
	CHECK(part.get_body() == "café\n");
}

TEST_CASE("A missing final boundary keeps the parts read so far") {
	Mimesis::Part part;
	part.from_string("Content-Type: multipart/mixed; boundary=b\n\npreamble\n--b\n\nfirst\n--b\n\nsecond\n");
	REQUIRE(part.get_parts().size() == 2);
	CHECK(part.get_parts()[0].get_body() == "first\n");
	CHECK(part.get_parts()[1].get_body() == "second\n");

	part.from_string("Content-Type: multipart/mixed; boundary=b\n\npreamble only\n");
	CHECK(part.is_multipart());
	CHECK(part.get_parts().empty());
	CHECK(part.get_preamble() == "preamble only\n");
}

TEST_CASE("Boundaries don't match longer boundaries") {
	Mimesis::Part part;
	part.from_string("Content-Type: multipart/mixed; boundary=abc\n\n--abc\n\n--abcdef\ntext\n--abc--\n");
	REQUIRE(part.get_parts().size() == 1);
	CHECK(part.get_parts()[0].get_body() == "--abcdef\ntext\n");
}

TEST_CASE("Boundary lines may have trailing whitespace") {
	Mimesis::Part part;
	part.from_string("Content-Type: multipart/mixed; boundary=b\n\n--b  \n\nfirst\n--b-- \t\nepilogue\n");
	REQUIRE(part.get_parts().size() == 1);
	CHECK(part.get_parts()[0].get_body() == "first\n");
	CHECK(part.get_epilogue() == "epilogue\n");
}

TEST_CASE("Nested parts end at the boundary of an enclosing part") {
	Mimesis::Part part;
	part.from_string(
		"Content-Type: multipart/mixed; boundary=outer\n"
		"\n"
		"--outer\n"
		"Content-Type: multipart/alternative; boundary=inner\n"
		"\n"
		"--inner\n"
		"\n"
		"inner\n"
		"--outer\n"
		"\n"
		"second\n"
		"--outer--\n");
	REQUIRE(part.get_parts().size() == 2);
	REQUIRE(part.get_parts()[0].get_parts().size() == 1);
	CHECK(part.get_parts()[0].get_parts()[0].get_body() == "inner\n");
	CHECK(part.get_parts()[1].get_body() == "second\n");
}

TEST_CASE("A multipart type without a boundary is treated as a single part") {
	Mimesis::Part part;
	part.from_string("Content-Type: multipart/mixed\n\n--x\nbody\n");
	CHECK(part.is_singlepart());
	CHECK(part.get_body() == "--x\nbody\n");
}

TEST_CASE("An mbox From line in front of the headers is skipped") {
	Mimesis::Part part;
	part.from_string("From someone@example.org Sat Jan  1 00:00:00 2000\nSubject: x\n\nbody\n");
	CHECK(part.get_headers().size() == 1);
	CHECK(part.get_header("Subject") == "x");
}

TEST_CASE("Folded headers are unfolded") {
	Mimesis::Part part;
	part.from_string("Subject: a\n b\n\n");
	CHECK(part.get_header("Subject") == "a b");
}

TEST_CASE("Invalid header lines are rejected") {
	Mimesis::Part part;
	CHECK_THROWS_AS(part.from_string("Invalid header\n\nbody\n"), std::runtime_error);
	CHECK_THROWS_AS(part.from_string(" continuation\n\nbody\n"), std::runtime_error);
	CHECK_THROWS_AS(part.from_string(": no name\n\nbody\n"), std::runtime_error);
}

TEST_CASE("A missing final newline is reproduced") {
	Mimesis::Part part;
	part.from_string("A: b\n\nbody");
	CHECK(part.get_body() == "body");
	CHECK(part.to_string() == "A: b\n\nbody");
}
