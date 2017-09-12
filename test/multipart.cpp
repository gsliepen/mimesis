#include <cassert>
#include <iostream>
#include <fstream>
#include <stdexcept>

#include <mimesis.hpp>

using namespace std;

int main(int argc, char *argv[]) {
	Mimesis::Message msg;

	// Set simple body
	msg.set_header("From", "me");
	msg.set_body("body\r\n");
	assert(msg.is_multipart() == false);
	assert(msg.get_body() == "body\r\n");
	assert(msg.to_string() == "From: me\r\n\r\nbody\r\n");

	// Clear body
	msg.clear_body();
	assert(msg.get_body().empty());
	assert(!msg.get_headers().empty());

	// Clear everything
	msg.set_body("body\r\n");
	msg.clear();
	assert(msg.get_header("From").empty());
	assert(msg.get_body().empty());

	// Make multipart
	msg.set_header("From", "me");
	msg.set_body("body\r\n");
	msg.make_multipart("mixed");
	assert(msg.is_multipart() == true);
	assert(msg.get_body().empty());
	assert(msg.get_header("MIME-Version") == "1.0");
	assert(msg.get_header_value("Content-Type") == "multipart/mixed");
	assert(!msg.get_header_parameter("Content-Type", "boundary").empty());
	assert(msg.get_boundary().size() == 32);
	assert(msg.get_parts()[0].get_body() == "body\r\n");

	{
		auto &part = msg.append_part();
		part.set_header("Content-Type", "text/plain");
		part.set_body("second body\r\n");
	}

	assert(msg.get_parts().size() == 2);
	assert(msg.get_parts()[1].get_body() == "second body\r\n");

	// Set preamble and epilogue
	msg.set_preamble("preamble\r\n");
	msg.set_epilogue("epilogue\r\n");

	// Intermediate result
	msg.set_boundary("-");
	assert(msg.to_string() ==
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
	assert(msg.get_parts().size() == 2);

	// Make different multipart
	msg.make_multipart("parallel", "=");
	assert(msg.is_multipart() == true);
	assert(msg.get_preamble().empty());
	assert(msg.get_epilogue().empty());
	assert(msg.get_parts().size() == 1);
	assert(msg.get_header_value("Content-Type") == "multipart/parallel");
	{
		auto &part = msg.get_parts()[0];
		assert(part.get_header_value("Content-Type") == "multipart/mixed");
		assert(part.get_preamble() == "preamble\r\n");
		assert(part.get_epilogue() == "epilogue\r\n");
	}

	// Constant part access
	const auto &cmsg = msg;
	assert(cmsg.get_parts().size() == 1);

	// Clear parts
	msg.clear_parts();
	assert(msg.is_multipart() == true);
	assert(msg.get_parts().empty());

	// Make singlepart
	{
		auto &part = msg.append_part();
		part.set_header("Content-Type", "foo/bar");
		part.set_body("third body\r\n");
	}
	assert(!msg.get_parts().empty());
	assert(msg.flatten());
	assert(msg.is_multipart() == false);
	assert(msg.get_parts().empty());
	assert(msg.get_header("Content-Type") == "foo/bar");
	assert(msg.get_body() == "third body\r\n");

	// High-level functions
	msg.clear();
	msg.set_plain("plain body\r\n");
	assert(msg.is_multipart() == false);
	assert(msg.get_header_value("Content-Type") == "text/plain");

	msg.set_html("html body\r\n");
	assert(msg.is_multipart() == true);
	assert(msg.get_header_value("Content-Type") == "multipart/alternative");
	assert(msg.get_parts().size() == 2);
	assert(msg.get_parts()[0].get_header_value("Content-Type") == "text/plain");
	assert(msg.get_parts()[0].get_body() == "plain body\r\n");
	assert(msg.get_parts()[1].get_header_value("Content-Type") == "text/html");
	assert(msg.get_parts()[1].get_body() == "html body\r\n");

	msg.attach("attachment\r\n", "text/plain", "foo");
	assert(msg.is_multipart() == true);
	assert(msg.get_header_value("Content-Type") == "multipart/mixed");
	assert(msg.get_parts().size() == 2);
	assert(msg.get_parts()[0].get_header_value("Content-Type") == "multipart/alternative");
	assert(msg.get_parts()[0].get_header_value("Content-Disposition") != "attachment");
	assert(msg.get_parts()[1].get_header_value("Content-Type") == "text/plain");
	assert(msg.get_parts()[1].get_header_value("Content-Disposition") == "attachment");
	assert(msg.get_parts()[1].get_header_parameter("Content-Disposition", "filename") == "foo");

	assert(msg.get_text() == "plain body\r\n");
	assert(msg.get_plain() == "plain body\r\n");
	assert(msg.get_html() == "html body\r\n");
	assert(msg.get_attachments().size() == 1);
	assert(msg.get_attachments()[0]->get_header_value("Content-Type") == "text/plain");
	assert(msg.get_attachments()[0]->get_body() == "attachment\r\n");


	// Const access
	assert(cmsg.get_first_matching_body("text/plain") == "plain body\r\n");
	assert(cmsg.get_first_matching_body("text/pdf").empty());

	// Delete parts
	msg.clear_text();
	assert(msg.has_text() == false);
	assert(msg.get_attachments().size() == 1);
	assert(msg.get_header_value("Content-Type") == "text/plain");

	msg.clear_attachments();
	assert(msg.has_attachments() == false);
	assert(msg.get_body().empty());
	assert(msg.get_header("Content-Type").empty());

	// Different order
	msg.clear();
	msg.attach("attachment\r\n", "text/plain", "foo");
	msg.set_html("html body\r\n");
	msg.set_plain("plain body\r\n");
	assert(msg.is_multipart() == true);
	assert(msg.get_header_value("Content-Type") == "multipart/mixed");
	assert(msg.get_parts().size() == 2);
	assert(msg.get_parts()[0].get_header_value("Content-Type") == "multipart/alternative");
	assert(msg.get_parts()[0].get_header_value("Content-Disposition") != "attachment");
	assert(msg.get_parts()[1].get_header_value("Content-Type") == "text/plain");
	assert(msg.get_parts()[1].get_header_value("Content-Disposition") == "attachment");
	assert(msg.get_parts()[1].get_header_parameter("Content-Disposition", "filename") == "foo");
	{
		auto &alternative = msg.get_parts()[0];
		assert(alternative.get_parts().size() == 2);
		assert(alternative.get_parts()[0].get_header_value("Content-Type") == "text/html");
		assert(alternative.get_parts()[1].get_header_value("Content-Type") == "text/plain");
	}

	assert(msg.get_text() == "html body\r\n");
	assert(msg.get_plain() == "plain body\r\n");
	assert(msg.get_html() == "html body\r\n");
	assert(msg.get_attachments().size() == 1);
	assert(msg.get_attachments()[0]->get_header_value("Content-Type") == "text/plain");
	assert(msg.get_attachments()[0]->get_body() == "attachment\r\n");

	// Delete parts
	msg.clear_attachments();
	assert(msg.has_text() == true);
	assert(msg.has_attachments() == false);
	assert(msg.get_header_value("Content-Type") == "multipart/alternative");

	msg.clear_plain();
	assert(msg.has_text() == true);
	assert(msg.has_plain() == false);
	assert(msg.has_html() == true);
	assert(msg.get_header_value("Content-Type") == "text/html");

	// Add to existing multipart/alternative
	msg.clear();
	msg.make_multipart("alternative");
	msg.set_html("html body\r\n");
	assert(msg.is_multipart() == true);
	assert(msg.get_parts().size() == 1);
	assert(msg.get_parts()[0].get_header_value("Content-Type") == "text/html");
	msg.clear_html();
	assert(msg.is_multipart() == false);
	assert(msg.get_body().empty());
	assert(msg.get_header_value("Content-Type").empty());

	// Combine with existing text part
	msg.clear();
	msg.make_multipart("alternative");
	msg.set_plain("plain body\r\n");
	msg.make_multipart("mixed");
	msg.set_html("html body\r\n");
	assert(msg.get_header_value("Content-Type") == "multipart/mixed");
	assert(msg.get_parts().size() == 1);
	{
		auto &alternative = msg.get_parts()[0];
		assert(alternative.get_header_value("Content-Type") == "multipart/alternative");
		assert(alternative.get_parts().size() == 2);
		assert(alternative.get_parts()[0].get_header_value("Content-Type") == "text/plain");
		assert(alternative.get_parts()[1].get_header_value("Content-Type") == "text/html");
	}

	// Simplify
	msg.simplify();
	assert(msg.get_header_value("Content-Type") == "multipart/alternative");
	assert(msg.get_parts().size() == 2);
	assert(msg.get_parts()[0].get_header_value("Content-Type") == "text/plain");
	assert(msg.get_parts()[1].get_header_value("Content-Type") == "text/html");

	// Overwrite parts
	msg.set_html("html body 2\r\n");
	msg.set_plain("plain body 2\r\n");
	assert(msg.get_parts().size() == 2);
	assert(msg.get_parts()[0].get_body() == "plain body 2\r\n");
	assert(msg.get_parts()[1].get_body() == "html body 2\r\n");

	// Flatten
	assert(msg.flatten() == false);
	msg.clear_parts();
	assert(msg.flatten() == true);
	assert(msg.flatten() == true);

	// Attach from stream
	msg.clear();
	{
		ifstream in("multipart.cpp");
		msg.attach(in, "text/plain", "attachment.txt");
	}
	assert(msg.get_attachments().size() == 1);
	{
		auto part = msg.get_attachments()[0];
		assert(part->get_header_value("Content-Type") == "text/plain");
		assert(part->get_header_parameter("Content-Disposition", "filename") == "attachment.txt");
		assert(part->get_body().size() > 4096);
	}

	// Attach a part
	msg.clear();
	{
		Mimesis::Part part;
		part.set_header("Content-Type", "text/plain");
		part.set_body("plain body\r\n");
		msg.attach(part);
	}
	assert(msg.is_multipart() == false);
	assert(msg.get_header_value("Content-Type") == "text/plain");
	assert(msg.get_attachments().size() == 1);

	// Attach another part
	{
		Mimesis::Part part;
		part.set_header("Content-Type", "text/html");
		part.set_body("html body\r\n");
		msg.attach(part);
	}
	assert(msg.is_multipart() == true);
	assert(msg.get_header_value("Content-Type") == "multipart/mixed");
	assert(msg.get_attachments().size() == 2);
	assert(msg.get_attachments()[0]->get_header_value("Content-Type") == "text/plain");
	assert(msg.get_attachments()[1]->get_header_value("Content-Type") == "text/html");

	// Attach a message
	msg.clear();
	{
		Mimesis::Message msg2;
		msg2.set_header("From", "me");
		msg2.set_body("body\r\n");
		msg.attach(msg2);
	}
	assert(msg.is_multipart() == false);
	assert(msg.get_header_value("Content-Type") == "message/rfc822");
	assert(msg.get_attachments().size() == 1);

	// Attach another message
	{
		Mimesis::Message msg2;
		msg2.set_header("From", "me");
		msg2.set_body("body\r\n");
		msg.attach(msg2);
	}
	assert(msg.is_multipart() == true);
	assert(msg.get_header_value("Content-Type") == "multipart/mixed");
	assert(msg.get_attachments().size() == 2);
	assert(msg.get_attachments()[0]->get_header_value("Content-Type") == "message/rfc822");
	assert(msg.get_attachments()[1]->get_header_value("Content-Type") == "message/rfc822");

	// Transplant parts
	msg.clear();
	msg.attach("attachment\r\n", "text/plain", "foo");
	msg.attach("attachment\r\n", "text/plain", "bar");
	{
		Mimesis::Part other;
		other.set_plain("plain body\r\n");
		other.set_html("html body\r\n");
		msg.set_parts(other.get_parts());
		assert(other.get_parts().size() == 2);
	}
	assert(msg.get_parts().size() == 2);
	assert(msg.get_parts()[0].get_mime_type() == "text/plain");
	assert(msg.get_parts()[1].get_mime_type() == "text/html");

}
