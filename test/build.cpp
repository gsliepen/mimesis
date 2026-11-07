#include <catch2/catch_test_macros.hpp>

#include <string>

#include <mimesis.hpp>

#include "test-utils.hpp"

static void add_addresses(Mimesis::Part &part, const std::string &subject = "Test") {
	part["From"] = "Some One <some.one@example.org>";
	part["To"] = "Someone Else <someone.else@example.org>";
	part["Subject"] = subject;
}

TEST_CASE("Low-level builders reproduce the reference messages") {
	Mimesis::Message message;
	add_addresses(message);
	message.set_body("Hello!\r\n");
	CHECK(message.to_string() == read_file(test_data_dir / "01-simple"));

	message.clear();
	add_addresses(message);
	message.make_multipart("alternative", "zxnrbl");
	message.set_preamble("This is the preamble.\r\n");
	{
		Mimesis::Part plain;
		plain["Content-Type"] = "text/plain";
		plain.set_body("Hello!\r\n");
		message.append_part(plain);

		Mimesis::Part html;
		html["Content-Type"] = "text/html";
		html.set_body("<p>Hello!</p>\r\n");
		message.append_part(html);
	}
	message.set_epilogue("This is the epilogue.\r\n");
	CHECK(message.to_string() == read_file(test_data_dir / "02-multipart-alternative"));

	Mimesis::Part attachment;
	attachment["Content-Type"] = "text/plain";
	attachment["Content-Disposition"] = "attachment; filename=\"attachment.txt\"";
	attachment.set_body("This is the attachment.\r\n");

	message.clear();
	add_addresses(message);
	message.make_multipart("mixed", "zxnrbl");
	message.set_preamble("This is the preamble.\r\n");
	{
		Mimesis::Part plain;
		plain["Content-Type"] = "text/plain";
		plain.set_body("Hello!\r\n");
		message.append_part(plain);
	}
	message.append_part(attachment);
	message.set_epilogue("This is the epilogue.\r\n");
	CHECK(message.to_string() == read_file(test_data_dir / "03-multipart-mixed"));

	message.clear();
	add_addresses(message);
	message.make_multipart("mixed", "zxnrbl");
	message.set_preamble("This is the preamble.\r\n");
	{
		Mimesis::Part nested;
		nested.make_multipart("alternative", "xyzzy");
		nested.set_preamble("This is the nested preamble.\r\n");
		Mimesis::Part plain;
		plain["Content-Type"] = "text/plain";
		plain.set_body("Hello!\r\n");
		nested.append_part(plain);
		Mimesis::Part html;
		html["Content-Type"] = "text/html";
		html.set_body("<p>Hello!</p>\r\n");
		nested.append_part(html);
		nested.set_epilogue("This is the nested epilogue.\r\n");
		message.append_part(nested);
	}
	message.append_part(attachment);
	message.set_epilogue("This is the epilogue.\r\n");
	CHECK(message.to_string() == read_file(test_data_dir / "04-nested-multipart"));

	Mimesis::Part outer;
	add_addresses(outer);
	outer["MIME-Version"] = "1.0";
	outer["Content-Type"] = "message/rfc822";
	message["Subject"] = "Nested test";
	outer.set_body(message.to_string());
	CHECK(outer.to_string() == read_file(test_data_dir / "05-nested-message-rfc822"));
}

TEST_CASE("High-level builders produce messages that parse back") {
	Mimesis::Message message;
	add_addresses(message);
	message.set_plain("Hello!\r\n");
	message.set_html("<p>Hello!</p>\r\n");
	message.attach("This is the attachment.\r\n", "text/plain");

	Mimesis::Message reparsed;
	reparsed.from_string(message.to_string());
	CHECK(reparsed.get_plain() == "Hello!\r\n");
	CHECK(reparsed.get_html() == "<p>Hello!</p>\r\n");
	CHECK(reparsed.get_attachments().size() == 1);
}
