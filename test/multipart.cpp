#include <cassert>
#include <iostream>
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
	assert(msg.get_parts()[0].get_body() == "body\r\n");

	{
		auto &part = msg.append_part();
		part.set_header("Content-Type", "text/plain");
		part.set_body("second body\r\n");
	}

	assert(msg.get_parts().size() == 2);
	assert(msg.get_parts()[1].get_body() == "second body\r\n");
	
	// Intermediate result
	msg.set_boundary("-");
	assert(msg.to_string() ==
			"From: me\r\n"
			"MIME-Version: 1.0\r\n"
			"Content-Type: multipart/mixed; boundary=-\r\n"
			"\r\n"
			"---\r\n"
			"\r\n"
			"body\r\n"
			"---\r\n"
			"Content-Type: text/plain\r\n"
			"\r\n"
			"second body\r\n"
			"-----\r\n");

	// Idempotent make multipart
	msg.make_multipart("mixed");
	assert(msg.get_parts().size() == 2);
	
	// Make different multipart
	msg.make_multipart("parallel");
	assert(msg.is_multipart() == true);
	assert(msg.get_parts().size() == 1);
	assert(msg.get_header_value("Content-Type") == "multipart/parallel");
	assert(msg.get_parts()[0].get_header_value("Content-Type") == "multipart/mixed");

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
	assert(msg.make_singlepart());
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
}	
