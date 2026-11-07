#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <clocale>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <mimesis.hpp>

#include "parameters.hpp"

using namespace std::chrono;

TEST_CASE("Dates are formatted in UTC without depending on the locale") {
	auto date = sys_days{year{2024} / 3 / 5} + 13h + 4min + 5s + 678ms;

	Mimesis::Message message;
	message.set_date(date);
	CHECK(message.get_header("Date") == "Tue, 05 Mar 2024 13:04:05 +0000");

	message.add_received("from example.org", date);
	CHECK(message.get_headers().front() == Mimesis::Header{"Received", "from example.org; Tue, 05 Mar 2024 13:04:05 +0000"});
}

TEST_CASE("Setting the date leaves the locale alone") {
	std::string before = std::setlocale(LC_TIME, nullptr);
	if (std::setlocale(LC_TIME, "C.UTF-8"))
		before = "C.UTF-8";

	Mimesis::Message message;
	message.set_date();
	CHECK(std::string(std::setlocale(LC_TIME, nullptr)) == before);

	std::setlocale(LC_TIME, "C");
}

TEST_CASE("Message IDs are unique") {
	std::set<std::string> ids;
	Mimesis::Message message;
	for (int i = 0; i < 1000; i++) {
		message.generate_msgid("example.org");
		auto id = message.get_header("Message-ID");
		CHECK(id.starts_with('<'));
		CHECK(id.ends_with("@example.org>"));
		ids.insert(id);
	}
	CHECK(ids.size() == 1000);
}

TEST_CASE("Generated boundaries are valid tokens") {
	for (int i = 0; i < 1000; i++) {
		Mimesis::Part part;
		part.make_multipart("mixed");
		auto boundary = part.get_boundary();
		REQUIRE(boundary.size() == 32);
		REQUIRE(Mimesis::quote_token(boundary) == boundary);
		REQUIRE(part.get_header_parameter("Content-Type", "boundary") == boundary);
	}
}

TEST_CASE("Boundaries can be generated from multiple threads") {
	std::vector<std::thread> threads;
	std::vector<std::string> boundaries(8);
	for (std::size_t i = 0; i < boundaries.size(); i++) {
		threads.emplace_back([&boundaries, i] {
			for (int j = 0; j < 100; j++) {
				Mimesis::Part part;
				part.make_multipart("mixed");
				boundaries[i] = part.get_boundary();
			}
		});
	}
	for (auto &thread: threads)
		thread.join();
	CHECK(std::set<std::string>(boundaries.begin(), boundaries.end()).size() == boundaries.size());
}

TEST_CASE("Comparison includes whether a part is a message") {
	Mimesis::Part part;
	Mimesis::Message message;
	part["Subject"] = "x";
	message["Subject"] = "x";
	CHECK(part != message);
	CHECK(part == Mimesis::Part(part));
}

TEST_CASE("MIME type and disposition checks ignore case and whitespace") {
	Mimesis::Part part;
	part.from_string(
		"Content-Type: Multipart/Alternative ; boundary=b\n"
		"\n"
		"--b\n"
		"Content-Type:  TEXT/Plain \n"
		"Content-Disposition: Attachment ; filename=x\n"
		"\n"
		"body\n"
		"--b\n"
		"Content-Type: text/html\n"
		"Content-Disposition: INLINE\n"
		"\n"
		"html\n"
		"--b--\n");
	CHECK(part.is_multipart("alternative"));
	REQUIRE(part.get_parts().size() == 2);
	CHECK(part.get_parts()[0].is_singlepart("text/plain"));
	CHECK(part.get_parts()[0].is_mime_type("text"));
	CHECK(part.get_parts()[0].is_attachment());
	CHECK(part.get_parts()[1].is_inline());
	CHECK(part.get_attachments().size() == 1);
}
