#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include <mimesis.hpp>

TEST_CASE("Low-level multipart manipulation") {
	Mimesis::Message message;

	message.set_header("From", "me");
	message.set_body("body\r\n");
	CHECK_FALSE(message.is_multipart());
	CHECK(message.get_body() == "body\r\n");
	CHECK(message.to_string() == "From: me\r\n\r\nbody\r\n");

	message.clear_body();
	CHECK(message.get_body().empty());
	CHECK_FALSE(message.get_headers().empty());

	message.set_body("body\r\n");
	message.clear();
	CHECK(message.get_header("From").empty());
	CHECK(message.get_body().empty());

	// Make multipart
	message.set_header("From", "me");
	message.set_body("body\r\n");
	message.make_multipart("mixed");
	CHECK(message.is_multipart());
	CHECK(message.get_multipart_type() == "mixed");
	CHECK(message.get_body().empty());
	CHECK(message.get_header("MIME-Version") == "1.0");
	CHECK(message.get_header_value("Content-Type") == "multipart/mixed");
	CHECK_FALSE(message.get_header_parameter("Content-Type", "boundary").empty());
	CHECK(message.get_boundary().size() == 32);
	CHECK(message.get_parts()[0].get_body() == "body\r\n");

	{
		auto &part = message.append_part();
		part.set_mime_type("text/plain");
		part.set_body("second body\r\n");
	}
	REQUIRE(message.get_parts().size() == 2);
	CHECK(message.get_parts()[1].get_body() == "second body\r\n");

	message.set_preamble("preamble\r\n");
	message.set_epilogue("epilogue\r\n");

	message.set_boundary("-");
	CHECK(message.to_string() ==
			"From: me\r\n"
			"MIME-Version: 1.0\r\n"
			"Content-Type: multipart/mixed; boundary=-\r\n"
			"\r\n"
			"preamble\r\n"
			"---\r\n"
			"\r\n"
			"body\r\n"
			"---\r\n"
			"Content-Type: text/plain\r\n"
			"\r\n"
			"second body\r\n"
			"-----\r\n"
			"epilogue\r\n");

	// Idempotent make multipart
	message.make_multipart("mixed");
	CHECK(message.get_parts().size() == 2);

	// Make different multipart
	message.make_multipart("parallel", "=");
	CHECK(message.is_multipart("parallel"));
	CHECK(message.get_header("Content-Type") == "multipart/parallel; boundary=\"=\"");
	CHECK(message.get_preamble().empty());
	CHECK(message.get_epilogue().empty());
	REQUIRE(message.get_parts().size() == 1);
	{
		auto &part = message.get_parts()[0];
		CHECK(part.is_multipart("mixed"));
		CHECK(part.get_boundary() == "-");
		CHECK(part.get_preamble() == "preamble\r\n");
		CHECK(part.get_epilogue() == "epilogue\r\n");
	}

	const auto &const_message = message;
	CHECK(const_message.get_parts().size() == 1);

	message.clear_parts();
	CHECK(message.is_multipart());
	CHECK(message.get_parts().empty());

	// Make singlepart
	{
		auto &part = message.append_part();
		part.set_mime_type("foo/bar");
		part.set_body("third body\r\n");
	}
	CHECK(message.flatten());
	CHECK(message.is_singlepart("foo"));
	CHECK_FALSE(message.is_singlepart("bar"));
	CHECK(message.is_singlepart("foo/bar"));
	CHECK(message.get_parts().empty());
	CHECK(message.get_body() == "third body\r\n");
}

TEST_CASE("High-level multipart manipulation") {
	Mimesis::Message message;
	const auto &const_message = message;

	message.set_plain("plain body\r\n");
	CHECK(message.is_singlepart("text/plain"));

	message.set_html("html body\r\n");
	CHECK(message.is_multipart("alternative"));
	REQUIRE(message.get_parts().size() == 2);
	CHECK(message.get_parts()[0].is_singlepart("text/plain"));
	CHECK(message.get_parts()[0].get_body() == "plain body\r\n");
	CHECK(message.get_parts()[1].is_singlepart("text/html"));
	CHECK(message.get_parts()[1].get_body() == "html body\r\n");

	message.attach("attachment\r\n", "text/plain", "foo");
	CHECK(message.is_multipart("mixed"));
	REQUIRE(message.get_parts().size() == 2);
	CHECK(message.get_parts()[0].is_multipart("alternative"));
	CHECK_FALSE(message.get_parts()[0].is_attachment());
	CHECK_FALSE(message.get_parts()[0].has_attachments());
	CHECK(message.get_parts()[1].is_singlepart("text/plain"));
	CHECK(message.get_parts()[1].get_header_value("Content-Disposition") == "attachment");
	CHECK(message.get_parts()[1].get_header_parameter("Content-Disposition", "filename") == "foo");
	CHECK(message.get_parts()[1].is_attachment());
	CHECK(message.get_parts()[1].has_attachments());

	CHECK(message.get_text() == "plain body\r\n");
	CHECK(message.get_plain() == "plain body\r\n");
	CHECK(message.get_html() == "html body\r\n");
	CHECK(message.has_attachments());
	REQUIRE(message.get_attachments().size() == 1);
	CHECK(message.get_attachments()[0]->is_singlepart("text/plain"));
	CHECK(message.get_attachments()[0]->get_body() == "attachment\r\n");

	CHECK(const_message.get_first_matching_body("text/plain") == "plain body\r\n");
	CHECK(const_message.get_first_matching_body("text/pdf").empty());

	// Delete parts
	message.clear_text();
	CHECK_FALSE(message.has_text());
	CHECK(message.get_attachments().size() == 1);
	CHECK(message.is_singlepart("text/plain"));

	message.clear_attachments();
	CHECK_FALSE(message.has_attachments());
	CHECK(message.get_body().empty());
	CHECK(message.get_header("Content-Type").empty());
	CHECK_FALSE(message.has_mime_type());

	// Different order
	message.clear();
	message.attach("attachment\r\n", "text/plain", "foo");
	message.set_html("html body\r\n");
	message.set_plain("plain body\r\n");
	CHECK(message.is_multipart("mixed"));
	REQUIRE(message.get_parts().size() == 2);
	CHECK(message.get_parts()[0].is_multipart("alternative"));
	CHECK_FALSE(message.get_parts()[0].is_attachment());
	CHECK(message.get_parts()[1].is_singlepart("text/plain"));
	CHECK(message.get_parts()[1].is_attachment());
	CHECK(message.get_parts()[1].get_header_parameter("Content-Disposition", "filename") == "foo");
	{
		auto &alternative = message.get_parts()[0];
		REQUIRE(alternative.get_parts().size() == 2);
		CHECK(alternative.get_parts()[0].is_singlepart("text/html"));
		CHECK(alternative.get_parts()[1].is_singlepart("text/plain"));
	}

	CHECK(message.get_text() == "html body\r\n");
	CHECK(message.get_plain() == "plain body\r\n");
	CHECK(message.get_html() == "html body\r\n");
	REQUIRE(message.get_attachments().size() == 1);
	CHECK(message.get_attachments()[0]->get_body() == "attachment\r\n");

	message.clear_attachments();
	CHECK(message.has_text());
	CHECK_FALSE(message.has_attachments());
	CHECK(message.is_multipart("alternative"));

	message.clear_plain();
	CHECK(message.has_text());
	CHECK_FALSE(message.has_plain());
	CHECK(message.has_html());
	CHECK(message.is_singlepart("text/html"));

	// Add to existing multipart/alternative
	message.clear();
	message.make_multipart("alternative");
	message.set_html("html body\r\n");
	CHECK(message.is_multipart("alternative"));
	REQUIRE(message.get_parts().size() == 1);
	CHECK(message.get_parts()[0].is_singlepart("text/html"));
	message.clear_html();
	CHECK_FALSE(message.is_multipart());
	CHECK(message.get_body().empty());
	CHECK_FALSE(message.has_mime_type());

	// Combine with existing text part
	message.clear();
	message.make_multipart("alternative");
	message.set_plain("plain body\r\n");
	message.make_multipart("mixed");
	message.set_html("html body\r\n");
	CHECK(message.get_header_value("Content-Type") == "multipart/mixed");
	REQUIRE(message.get_parts().size() == 1);
	{
		auto &alternative = message.get_parts()[0];
		CHECK(alternative.is_multipart("alternative"));
		REQUIRE(alternative.get_parts().size() == 2);
		CHECK(alternative.get_parts()[0].is_singlepart("text/plain"));
		CHECK(alternative.get_parts()[1].is_singlepart("text/html"));
	}

	message.simplify();
	CHECK(message.is_multipart("alternative"));
	REQUIRE(message.get_parts().size() == 2);
	CHECK(message.get_parts()[0].is_singlepart("text/plain"));
	CHECK(message.get_parts()[1].is_singlepart("text/html"));

	// Overwrite parts
	message.set_html("html body 2\r\n");
	message.set_plain("plain body 2\r\n");
	REQUIRE(message.get_parts().size() == 2);
	CHECK(message.get_parts()[0].get_body() == "plain body 2\r\n");
	CHECK(message.get_parts()[1].get_body() == "html body 2\r\n");

	CHECK_FALSE(message.flatten());
	message.clear_parts();
	CHECK(message.flatten());
	CHECK(message.flatten());
	CHECK_FALSE(message.has_mime_type());
}

TEST_CASE("Attaching data, parts and messages") {
	Mimesis::Message message;

	SECTION("from a stream") {
		std::istringstream in("Stream attachment\n");
		message.attach(in, "text/plain", "attachment.txt");
		REQUIRE(message.get_attachments().size() == 1);
		auto part = message.get_attachments()[0];
		CHECK(part->is_singlepart("text/plain"));
		CHECK(part->get_header_parameter("Content-Disposition", "filename") == "attachment.txt");
		CHECK(part->get_body() == "Stream attachment\n");
	}

	SECTION("parts") {
		Mimesis::Part plain;
		plain.set_header("Content-Type", "text/plain");
		plain.set_body("plain body\r\n");
		message.attach(plain);
		CHECK(message.is_singlepart("text/plain"));
		CHECK(message.get_attachments().size() == 1);

		Mimesis::Part html;
		html.set_header("Content-Type", "text/html");
		html.set_body("html body\r\n");
		message.attach(html);
		CHECK(message.is_multipart("mixed"));
		REQUIRE(message.get_attachments().size() == 2);
		CHECK(message.get_attachments()[0]->is_singlepart("text/plain"));
		CHECK(message.get_attachments()[1]->is_singlepart("text/html"));
	}

	SECTION("messages") {
		Mimesis::Message attachment;
		attachment.set_header("From", "me");
		attachment.set_body("body\r\n");
		message.attach(attachment);
		CHECK(message.is_singlepart());
		CHECK(message.get_header_value("Content-Type") == "message/rfc822");
		CHECK(message.get_body() == "From: me\r\n\r\nbody\r\n");
		CHECK(message.get_attachments().size() == 1);

		message.attach(attachment);
		CHECK(message.is_multipart("mixed"));
		REQUIRE(message.get_attachments().size() == 2);
		CHECK(message.get_attachments()[0]->is_singlepart("message/rfc822"));
		CHECK(message.get_attachments()[1]->is_singlepart("message/rfc822"));
	}

	SECTION("transplanting parts") {
		message.attach("attachment\r\n", "text/plain", "foo");
		message.attach("attachment\r\n", "text/plain", "bar");
		{
			Mimesis::Part other;
			other.set_plain("plain body\r\n");
			other.set_html("html body\r\n");
			message.set_parts(other.get_parts());
		}
		REQUIRE(message.get_parts().size() == 2);
		CHECK(message.get_parts()[0].is_singlepart("text/plain"));
		CHECK(message.get_parts()[1].is_singlepart("text/html"));
	}
}

TEST_CASE("Flattening a nested multipart keeps boundary and delimiters consistent") {
	Mimesis::Message message;
	message["Subject"] = "x";
	message.make_multipart("mixed", "outer");
	message.set_preamble("outer preamble\r\n");

	Mimesis::Part inner;
	inner.make_multipart("alternative", "inner");
	inner.set_preamble("inner preamble\r\n");
	inner.set_epilogue("inner epilogue\r\n");
	inner.set_plain("a\r\n");
	inner.set_html("<b>a</b>\r\n");
	message.append_part(inner);

	CHECK(message.flatten());
	CHECK(message.is_multipart("alternative"));
	CHECK(message.get_boundary() == "inner");
	CHECK(message.get_header_parameter("Content-Type", "boundary") == "inner");
	CHECK(message.get_preamble() == "inner preamble\r\n");
	CHECK(message.get_epilogue() == "inner epilogue\r\n");

	Mimesis::Message reparsed;
	reparsed.from_string(message.to_string());
	CHECK(reparsed == message);
	CHECK(reparsed.get_plain() == "a\r\n");
	CHECK(reparsed.get_html() == "<b>a</b>\r\n");
}

TEST_CASE("Flattening a single encoded part keeps its transfer encoding") {
	Mimesis::Message message;
	message["Subject"] = "x";
	message.make_multipart("mixed");
	message.append_part().attach(std::string("\x00\xff", 2), "application/octet-stream", "x.bin");

	CHECK(message.flatten());
	CHECK(message.get_header("Content-Transfer-Encoding") == "base64");
	CHECK(message.get_body() == std::string("\x00\xff", 2));
	CHECK(message.get_header_parameter("Content-Disposition", "filename") == "x.bin");
}

TEST_CASE("Clearing the text of a single-part message keeps its other headers") {
	Mimesis::Message message;
	message["From"] = "me";
	message.set_plain("text\r\n");
	message.clear_text();
	CHECK(message.get_header("From") == "me");
	CHECK_FALSE(message.has_text());
	CHECK_FALSE(message.has_mime_type());
}

TEST_CASE("Body parts always end before the next boundary line") {
	Mimesis::Part part;
	part.make_multipart("mixed", "b");
	part.append_part().set_body("no newline");
	part.append_part().set_body("");
	CHECK(part.to_string() ==
			"Content-Type: multipart/mixed; boundary=b\r\n"
			"\r\n"
			"--b\r\n"
			"\r\n"
			"no newline\r\n"
			"--b\r\n"
			"\r\n"
			"--b--\r\n");
}
