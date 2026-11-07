#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <sstream>

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
		part.load((test_data_dir / name).string());
		CHECK(part.to_string() == read_file(test_data_dir / name));
	}
}

TEST_CASE("Messages survive save and load round-trips") {
	auto temporary = (std::filesystem::temp_directory_path() / "mimesis-load-save.tmp").string();

	for (auto name: clean_inputs) {
		INFO(name);
		Mimesis::Message message;
		message.load((test_data_dir / name).string());

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
	message.load((test_data_dir / "04-nested-multipart").string());
	CHECK(message.is_multipart("mixed"));
	REQUIRE(message.get_parts().size() == 2);
	CHECK(message.get_parts()[0].is_multipart("alternative"));
	CHECK(message.get_parts()[0].get_parts().size() == 2);
	CHECK(message.get_parts()[1].is_attachment());

	message.clear();
	message.load((test_data_dir / "05-nested-message-rfc822").string());
	CHECK(message.is_singlepart("message/rfc822"));
}
