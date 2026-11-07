#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include <mimesis.hpp>

using namespace std;

TEST_CASE("Multipart manipulation") {
	Mimesis::Message msg;

	// Set simple body
	msg.set_header("From", "me");
	msg.set_body("body\r\n");
	CHECK(msg.is_multipart() == false);
	CHECK(msg.get_body() == "body\r\n");
	CHECK(msg.to_string() == "From: me\r\n\r\nbody\r\n");

	// Clear body
	msg.clear_body();
	CHECK(msg.get_body().empty());
	CHECK(!msg.get_headers().empty());

	// Clear everything
	msg.set_body("body\r\n");
	msg.clear();
	CHECK(msg.get_header("From").empty());
	CHECK(msg.get_body().empty());

	// Make multipart
	msg.set_header("From", "me");
	msg.set_body("body\r\n");
	msg.make_multipart("mixed");
	CHECK(msg.is_multipart() == true);
	CHECK(msg.get_body().empty());
	CHECK(msg.get_header("MIME-Version") == "1.0");
	CHECK(msg.get_header_value("Content-Type") == "multipart/mixed");
	CHECK(!msg.get_header_parameter("Content-Type", "boundary").empty());
	CHECK(msg.get_boundary().size() == 32);
	CHECK(msg.get_parts()[0].get_body() == "body\r\n");

	{
		auto &part = msg.append_part();
		part.set_mime_type("text/plain");
		part.set_body("second body\r\n");
	}

	CHECK(msg.get_parts().size() == 2);
	CHECK(msg.get_parts()[1].get_body() == "second body\r\n");

	// Set preamble and epilogue
	msg.set_preamble("preamble\r\n");
	msg.set_epilogue("epilogue\r\n");

	// Intermediate result
	msg.set_boundary("-");
	CHECK(msg.to_string() ==
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
	msg.make_multipart("mixed");
	CHECK(msg.get_parts().size() == 2);

	// Make different multipart
	msg.make_multipart("parallel", "=");
	CHECK(msg.is_multipart("parallel"));
	CHECK(msg.get_preamble().empty());
	CHECK(msg.get_epilogue().empty());
	CHECK(msg.get_parts().size() == 1);
	{
		auto &part = msg.get_parts()[0];
		CHECK(part.is_multipart("mixed"));
		CHECK(part.get_preamble() == "preamble\r\n");
		CHECK(part.get_epilogue() == "epilogue\r\n");
	}

	// Constant part access
	const auto &cmsg = msg;
	CHECK(cmsg.get_parts().size() == 1);

	// Clear parts
	msg.clear_parts();
	CHECK(msg.is_multipart() == true);
	CHECK(msg.get_parts().empty());

	// Make singlepart
	{
		auto &part = msg.append_part();
		part.set_mime_type("foo/bar");
		part.set_body("third body\r\n");
	}
	CHECK(!msg.get_parts().empty());
	CHECK(msg.flatten());
	CHECK(msg.is_singlepart("foo"));
	CHECK(!msg.is_singlepart("bar"));
	CHECK(msg.is_singlepart("foo/bar"));
	CHECK(msg.get_parts().empty());
	CHECK(msg.get_body() == "third body\r\n");

	// High-level functions
	msg.clear();
	msg.set_plain("plain body\r\n");
	CHECK(msg.is_singlepart("text/plain"));

	msg.set_html("html body\r\n");
	CHECK(msg.is_multipart("alternative"));
	CHECK(msg.get_parts().size() == 2);
	CHECK(msg.get_parts()[0].is_singlepart("text/plain"));
	CHECK(msg.get_parts()[0].get_body() == "plain body\r\n");
	CHECK(msg.get_parts()[1].is_singlepart("text/html"));
	CHECK(msg.get_parts()[1].get_body() == "html body\r\n");

	msg.attach("attachment\r\n", "text/plain", "foo");
	CHECK(msg.is_multipart("mixed"));
	CHECK(msg.get_parts().size() == 2);
	CHECK(msg.get_parts()[0].is_multipart("alternative"));
	CHECK(msg.get_parts()[0].get_header_value("Content-Disposition") != "attachment");
	CHECK(msg.get_parts()[0].is_attachment() == false);
	CHECK(msg.get_parts()[0].has_attachments() == false);
	CHECK(msg.get_parts()[1].is_singlepart("text/plain"));
	CHECK(msg.get_parts()[1].get_header_value("Content-Disposition") == "attachment");
	CHECK(msg.get_parts()[1].get_header_parameter("Content-Disposition", "filename") == "foo");
	CHECK(msg.get_parts()[1].is_attachment() == true);
	CHECK(msg.get_parts()[1].has_attachments() == true);

	CHECK(msg.get_text() == "plain body\r\n");
	CHECK(msg.get_plain() == "plain body\r\n");
	CHECK(msg.get_html() == "html body\r\n");
	CHECK(msg.has_attachments());
	CHECK(msg.get_attachments().size() == 1);
	CHECK(msg.get_attachments()[0]->is_singlepart("text/plain"));
	CHECK(msg.get_attachments()[0]->get_body() == "attachment\r\n");

	// Const access
	CHECK(cmsg.get_first_matching_body("text/plain") == "plain body\r\n");
	CHECK(cmsg.get_first_matching_body("text/pdf").empty());

	// Delete parts
	msg.clear_text();
	CHECK(msg.has_text() == false);
	CHECK(msg.get_attachments().size() == 1);
	CHECK(msg.is_singlepart("text/plain"));

	msg.clear_attachments();
	CHECK(msg.has_attachments() == false);
	CHECK(msg.get_body().empty());
	CHECK(msg.get_header("Content-Type").empty());
	CHECK(msg.has_mime_type() == false);

	// Different order
	msg.clear();
	msg.attach("attachment\r\n", "text/plain", "foo");
	msg.set_html("html body\r\n");
	msg.set_plain("plain body\r\n");
	CHECK(msg.is_multipart("mixed"));
	CHECK(msg.get_parts().size() == 2);
	CHECK(msg.get_parts()[0].is_multipart("alternative"));
	CHECK(msg.get_parts()[0].is_attachment() == false);
	CHECK(msg.get_parts()[1].is_singlepart("text/plain"));
	CHECK(msg.get_parts()[1].is_attachment() == true);
	CHECK(msg.get_parts()[1].get_header_parameter("Content-Disposition", "filename") == "foo");
	{
		auto &alternative = msg.get_parts()[0];
		CHECK(alternative.get_parts().size() == 2);
		CHECK(alternative.get_parts()[0].is_singlepart("text/html"));
		CHECK(alternative.get_parts()[1].is_singlepart("text/plain"));
	}

	CHECK(msg.get_text() == "html body\r\n");
	CHECK(msg.get_plain() == "plain body\r\n");
	CHECK(msg.get_html() == "html body\r\n");
	CHECK(msg.get_attachments().size() == 1);
	CHECK(msg.get_attachments()[0]->is_singlepart("text/plain"));
	CHECK(msg.get_attachments()[0]->get_body() == "attachment\r\n");

	// Delete parts
	msg.clear_attachments();
	CHECK(msg.has_text() == true);
	CHECK(msg.has_attachments() == false);
	CHECK(msg.is_multipart("alternative"));

	msg.clear_plain();
	CHECK(msg.has_text() == true);
	CHECK(msg.has_plain() == false);
	CHECK(msg.has_html() == true);
	CHECK(msg.is_singlepart("text/html"));

	// Add to existing multipart/alternative
	msg.clear();
	msg.make_multipart("alternative");
	msg.set_html("html body\r\n");
	CHECK(msg.is_multipart("alternative"));
	CHECK(msg.get_parts().size() == 1);
	CHECK(msg.get_parts()[0].is_singlepart("text/html"));
	msg.clear_html();
	CHECK(msg.is_multipart() == false);
	CHECK(msg.get_body().empty());
	CHECK(msg.has_mime_type() == false);

	// Combine with existing text part
	msg.clear();
	msg.make_multipart("alternative");
	msg.set_plain("plain body\r\n");
	msg.make_multipart("mixed");
	msg.set_html("html body\r\n");
	CHECK(msg.get_header_value("Content-Type") == "multipart/mixed");
	CHECK(msg.get_parts().size() == 1);
	{
		auto &alternative = msg.get_parts()[0];
		CHECK(alternative.is_multipart("alternative"));
		CHECK(alternative.get_parts().size() == 2);
		CHECK(alternative.get_parts()[0].is_singlepart("text/plain"));
		CHECK(alternative.get_parts()[1].is_singlepart("text/html"));
	}

	// Simplify
	msg.simplify();
	CHECK(msg.is_multipart("alternative"));
	CHECK(msg.get_parts().size() == 2);
	CHECK(msg.get_parts()[0].is_singlepart("text/plain"));
	CHECK(msg.get_parts()[1].is_singlepart("text/html"));

	// Overwrite parts
	msg.set_html("html body 2\r\n");
	msg.set_plain("plain body 2\r\n");
	CHECK(msg.get_parts().size() == 2);
	CHECK(msg.get_parts()[0].get_body() == "plain body 2\r\n");
	CHECK(msg.get_parts()[1].get_body() == "html body 2\r\n");

	// Flatten
	CHECK(msg.flatten() == false);
	msg.clear_parts();
	CHECK(msg.flatten() == true);
	CHECK(msg.flatten() == true);

	// Attach from stream
	msg.clear();
	{
		istringstream in("Stream attachment\n");
		msg.attach(in, "text/plain", "attachment.txt");
	}
	CHECK(msg.get_attachments().size() == 1);
	{
		auto part = msg.get_attachments()[0];
		CHECK(part->is_singlepart("text/plain"));
		CHECK(part->get_header_parameter("Content-Disposition", "filename") == "attachment.txt");
		CHECK(part->get_body() == "Stream attachment\n");
	}

	// Attach a part
	msg.clear();
	{
		Mimesis::Part part;
		part.set_header("Content-Type", "text/plain");
		part.set_body("plain body\r\n");
		msg.attach(part);
	}
	CHECK(msg.is_singlepart("text/plain"));
	CHECK(msg.get_attachments().size() == 1);

	// Attach another part
	{
		Mimesis::Part part;
		part.set_header("Content-Type", "text/html");
		part.set_body("html body\r\n");
		msg.attach(part);
	}
	CHECK(msg.is_multipart("mixed"));
	CHECK(msg.get_attachments().size() == 2);
	CHECK(msg.get_attachments()[0]->is_singlepart("text/plain"));
	CHECK(msg.get_attachments()[1]->is_singlepart("text/html"));

	// Attach a message
	msg.clear();
	{
		Mimesis::Message msg2;
		msg2.set_header("From", "me");
		msg2.set_body("body\r\n");
		msg.attach(msg2);
	}
	CHECK(msg.is_multipart() == false);
	CHECK(msg.is_singlepart() == true);
	CHECK(msg.get_header_value("Content-Type") == "message/rfc822");
	CHECK(msg.get_attachments().size() == 1);

	// Attach another message
	{
		Mimesis::Message msg2;
		msg2.set_header("From", "me");
		msg2.set_body("body\r\n");
		msg.attach(msg2);
	}
	CHECK(msg.is_multipart("mixed"));
	CHECK(msg.get_attachments().size() == 2);
	CHECK(msg.get_attachments()[0]->is_singlepart("message/rfc822"));
	CHECK(msg.get_attachments()[1]->is_singlepart("message/rfc822"));

	// Transplant parts
	msg.clear();
	msg.attach("attachment\r\n", "text/plain", "foo");
	msg.attach("attachment\r\n", "text/plain", "bar");
	{
		Mimesis::Part other;
		other.set_plain("plain body\r\n");
		other.set_html("html body\r\n");
		msg.set_parts(other.get_parts());
		CHECK(other.get_parts().size() == 2);
	}
	CHECK(msg.get_parts().size() == 2);
	CHECK(msg.get_parts()[0].is_singlepart("text/plain"));
	CHECK(msg.get_parts()[1].is_singlepart("text/html"));

}
