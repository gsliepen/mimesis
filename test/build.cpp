#include <catch2/catch_test_macros.hpp>

#include <string>

#include <mimesis.hpp>

#include "test-utils.hpp"

static void add_addresses(Mimesis::Part &part, const std::string &subject = "Test") {
	part["From"] = "Some One <some.one@example.org>";
	part["To"] = "Someone Else <someone.else@example.org>";
	part["Subject"] = subject;
}

static const std::string address_headers =
	"From: Some One <some.one@example.org>\r\n"
	"To: Someone Else <someone.else@example.org>\r\n"
	"Subject: Test\r\n";

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

TEST_CASE("High-level builders") {
	Mimesis::Message message;
	add_addresses(message);
	message.set_plain("Hello!\r\n");
	CHECK(message.to_string() ==
			address_headers +
			"MIME-Version: 1.0\r\n"
			"Content-Type: text/plain; charset=utf-8\r\n"
			"\r\n"
			"Hello!\r\n");

	message.set_html("<p>Hello!</p>\r\n");
	message.set_boundary("zxnrbl");
	CHECK(message.to_string() ==
			address_headers +
			"MIME-Version: 1.0\r\n"
			"Content-Type: multipart/alternative; boundary=zxnrbl\r\n"
			"\r\n"
			"--zxnrbl\r\n"
			"Content-Type: text/plain; charset=utf-8\r\n"
			"\r\n"
			"Hello!\r\n"
			"--zxnrbl\r\n"
			"Content-Type: text/html; charset=utf-8\r\n"
			"\r\n"
			"<p>Hello!</p>\r\n"
			"--zxnrbl--\r\n");

	message.attach("This is the attachment.\r\n", "text/plain", "attachment.txt");
	message.set_boundary("zxnrbl");
	message.get_parts()[0].set_boundary("xyzzy");
	CHECK(message.to_string() ==
			address_headers +
			"MIME-Version: 1.0\r\n"
			"Content-Type: multipart/mixed; boundary=zxnrbl\r\n"
			"\r\n"
			"--zxnrbl\r\n"
			"Content-Type: multipart/alternative; boundary=xyzzy\r\n"
			"\r\n"
			"--xyzzy\r\n"
			"Content-Type: text/plain; charset=utf-8\r\n"
			"\r\n"
			"Hello!\r\n"
			"--xyzzy\r\n"
			"Content-Type: text/html; charset=utf-8\r\n"
			"\r\n"
			"<p>Hello!</p>\r\n"
			"--xyzzy--\r\n"
			"--zxnrbl\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Disposition: attachment; filename=attachment.txt\r\n"
			"\r\n"
			"This is the attachment.\r\n"
			"--zxnrbl--\r\n");

	Mimesis::Part outer;
	add_addresses(outer);
	message["Subject"] = "Nested test";
	outer.attach(message);
	CHECK(outer.get_header("MIME-Version").empty());
	CHECK(outer.get_header("Content-Type") == "message/rfc822");
	CHECK(outer.get_header("Content-Disposition") == "attachment");
	CHECK(outer.get_body() == message.to_string());
}

TEST_CASE("Text is encoded as needed") {
	Mimesis::Message message;
	message["Subject"] = "x";

	SECTION("ASCII text is sent as is") {
		message.set_plain("Hello!\r\n");
		CHECK(message.get_header("Content-Transfer-Encoding").empty());
	}

	SECTION("mostly ASCII text uses quoted-printable") {
		message.set_plain("Café au lait\r\n");
		CHECK(message.get_header("Content-Transfer-Encoding") == "quoted-printable");
		CHECK(message.get_header_parameter("Content-Type", "charset") == "utf-8");
		CHECK(message.to_string().ends_with("\r\n\r\nCaf=C3=A9 au lait\r\n"));
		CHECK(message.get_plain() == "Café au lait\r\n");
	}

	SECTION("mostly non-ASCII text uses base64") {
		message.set_plain("日本語のテキスト\r\n");
		CHECK(message.get_header("Content-Transfer-Encoding") == "base64");
		CHECK(message.get_plain() == "日本語のテキスト\r\n");
	}

	SECTION("lines that are too long use quoted-printable") {
		auto text = std::string(2000, 'x') + "\r\n";
		message.set_plain(text);
		CHECK(message.get_header("Content-Transfer-Encoding") == "quoted-printable");
		CHECK(all_lines_at_most(message.to_string(), 76));
		CHECK(message.get_plain() == text);
	}

	Mimesis::Message reparsed;
	reparsed.from_string(message.to_string());
	CHECK(reparsed.get_plain() == message.get_plain());
}

TEST_CASE("Binary attachments are base64 encoded") {
	std::string data;
	for (int i = 0; i < 1000; i++)
		data.push_back(static_cast<char>(i * 7));

	Mimesis::Message message;
	message["Subject"] = "x";
	message.set_plain("See attachment.\r\n");
	auto &attachment = message.attach(data, "application/octet-stream", "data.bin");
	CHECK(attachment.get_header("Content-Transfer-Encoding") == "base64");
	CHECK(attachment.get_body() == data);
	CHECK(all_lines_at_most(message.to_string(), 76));

	Mimesis::Message reparsed;
	reparsed.from_string(message.to_string());
	REQUIRE(reparsed.get_attachments().size() == 1);
	CHECK(reparsed.get_attachments()[0]->get_body() == data);
	CHECK(reparsed.get_attachments()[0]->get_header_parameter("Content-Disposition", "filename") == "data.bin");
}

TEST_CASE("Attachments that are not text keep their exact content") {
	for (std::string data: {"data", "data\n", "line\nendings\r\n"}) {
		INFO(data);
		Mimesis::Message message;
		message["Subject"] = "x";
		message.set_plain("See attachment.\r\n");
		message.attach(data, "application/json", "data.json");
		CHECK(message.get_attachments()[0]->get_header("Content-Transfer-Encoding") == "base64");

		Mimesis::Message reparsed;
		reparsed.from_string(message.to_string());
		REQUIRE(reparsed.get_attachments().size() == 1);
		CHECK(reparsed.get_attachments()[0]->get_body() == data);
	}
}

TEST_CASE("Attachments with non-ASCII filenames use RFC 2231") {
	Mimesis::Message message;
	message["Subject"] = "x";
	message.attach("data", "application/octet-stream", "résumé.pdf");
	CHECK(message.get_header("Content-Disposition") == "attachment; filename*=utf-8''r%C3%A9sum%C3%A9.pdf");
	CHECK(message.get_header_parameter("Content-Disposition", "filename") == "résumé.pdf");
}

TEST_CASE("Messages built with high-level functions get a MIME-Version header") {
	Mimesis::Message message;
	message["Subject"] = "x";
	message.attach("data", "application/octet-stream");
	CHECK(message.get_header("MIME-Version") == "1.0");

	Mimesis::Part part;
	part.set_plain("text");
	CHECK(part.get_header("MIME-Version").empty());
}

TEST_CASE("Replacing text resets the transfer encoding") {
	Mimesis::Message message;
	message.from_string("Subject: x\nContent-Type: text/plain\nContent-Transfer-Encoding: base64\n\naGVsbG8=\n");
	message.set_plain("new text\n");
	CHECK(message.get_header("Content-Transfer-Encoding").empty());
	CHECK(message.get_plain() == "new text\n");
}

TEST_CASE("Making a body multipart moves its transfer encoding along") {
	Mimesis::Message message;
	message.from_string("Subject: x\nContent-Type: text/plain\nContent-Transfer-Encoding: base64\n\naGVsbG8=\n");
	message.attach("data", "application/octet-stream");
	CHECK(message.get_header("Content-Transfer-Encoding").empty());
	REQUIRE(message.get_parts().size() == 2);
	CHECK(message.get_parts()[0].get_header("Content-Transfer-Encoding") == "base64");
	CHECK(message.get_plain() == "hello");
}

TEST_CASE("Attaching a part keeps its encoding, parameters and parts") {
	std::string data("\x00\x01\xff\n", 4);
	Mimesis::Part binary;
	binary.attach(data, "application/octet-stream", "a.bin");

	Mimesis::Message message;
	message["Subject"] = "x";
	message.set_plain("text\r\n");
	message.attach(binary);
	REQUIRE(message.get_attachments().size() == 1);
	auto attachment = message.get_attachments()[0];
	CHECK(attachment->get_body() == data);
	CHECK(attachment->get_header_parameter("Content-Disposition", "filename") == "a.bin");

	Mimesis::Part multipart;
	multipart.set_plain("plain\r\n");
	multipart.set_html("html\r\n");
	message.attach(multipart);
	REQUIRE(message.get_parts().size() == 3);
	auto &attached = message.get_parts()[2];
	CHECK(attached.is_multipart("alternative"));
	CHECK(attached.is_attachment());
	CHECK(attached.get_parts().size() == 2);
}
