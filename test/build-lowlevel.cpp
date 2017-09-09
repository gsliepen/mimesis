#include <iostream>
#include <fstream>

#include <mimesis.hpp>

using namespace std;

int main(int argc, char *argv[]) {
	// 01-simple
	Mimesis::MIMEPart msg;
	msg["From"] = "Some One <some.one@example.org>";
	msg["To"] = "Someone Else <someone.else@example.org>";
	msg["Subject"] = "Test";
	msg.set_body("Hello!\r\n");
	msg.save("01-simple");

	// 02-multipart-alternative
	msg.clear();
	msg["From"] = "Some One <some.one@example.org>";
	msg["To"] = "Someone Else <someone.else@example.org>";
	msg["Subject"] = "Test";
	msg.make_multipart("alternative");
	msg.set_boundary("zxnrbl");
	msg.set_preamble("This is the preamble.\r\n");
	{
		Mimesis::MIMEPart plain_hello;
		plain_hello["Content-Type"] = "text/plain";
		plain_hello.set_body("Hello!\r\n");
		msg.append_part(plain_hello);
	}
	{
		Mimesis::MIMEPart html_hello;
		html_hello["Content-Type"] = "text/html";
		html_hello.set_body("<p>Hello!</p>\r\n");
		msg.append_part(html_hello);
	}
	msg.set_epilogue("This is the epilogue.\r\n");
	msg.save("02-multipart-alternative");

	// 03-multipart-mixed
	msg.clear();
	msg["From"] = "Some One <some.one@example.org>";
	msg["To"] = "Someone Else <someone.else@example.org>";
	msg["Subject"] = "Test";
	msg.make_multipart("mixed");
	msg.set_boundary("zxnrbl");
	msg.set_preamble("This is the preamble.\r\n");
	{
		Mimesis::MIMEPart plain_hello;
		plain_hello["Content-Type"] = "text/plain";
		plain_hello.set_body("Hello!\r\n");
		msg.append_part(plain_hello);
	}
	{
		Mimesis::MIMEPart plain_attachment;
		plain_attachment["Content-Type"] = "text/plain";
		plain_attachment["Content-Disposition"] = "attachment; filename=\"attachment.txt\"";
		plain_attachment.set_body("This is the attachment.\r\n");
		msg.append_part(plain_attachment);
	}
	msg.set_epilogue("This is the epilogue.\r\n");
	msg.save("03-multipart-alternative");

	// 04-nested-multipart
	msg.clear();
	msg["From"] = "Some One <some.one@example.org>";
	msg["To"] = "Someone Else <someone.else@example.org>";
	msg["Subject"] = "Test";
	msg.make_multipart("mixed");
	msg.set_boundary("zxnrbl");
	msg.set_preamble("This is the preamble.\r\n");
	{
		Mimesis::MIMEPart nested;
		nested.make_multipart("alternative");
		nested.set_boundary("xyzzy");
		nested.set_preamble("This is the nested preamble.\r\n");
		{
			Mimesis::MIMEPart plain_hello;
			plain_hello["Content-Type"] = "text/plain";
			plain_hello.set_body("Hello!\r\n");
			nested.append_part(plain_hello);
		}
		{
			Mimesis::MIMEPart html_hello;
			html_hello["Content-Type"] = "text/html";
			html_hello.set_body("<p>Hello!</p>\r\n");
			nested.append_part(html_hello);
		}
		nested.set_epilogue("This is the nested epilogue.\r\n");
		msg.append_part(nested);
	}
	{
		Mimesis::MIMEPart plain_attachment;
		plain_attachment["Content-Type"] = "text/plain";
		plain_attachment["Content-Disposition"] = "attachment; filename=\"attachment.txt\"";
		plain_attachment.set_body("This is the attachment.\r\n");
		msg.append_part(plain_attachment);
	}
	msg.set_epilogue("This is the epilogue.\r\n");
	msg.save("04-nested-multipart");

	// 05-nested-message-rfc822
	Mimesis::MIMEPart outer;
	outer["From"] = "Some One <some.one@example.org>";
	outer["To"] = "Someone Else <someone.else@example.org>";
	outer["Subject"] = "Test";
	outer["MIME-Version"] = "1.0";
	outer["Content-Type"] = "message/rfc822";
	msg["Subject"] = "Nested test";
	outer.set_body(msg.to_string());
	outer.save("05-nested-message-rfc822");
}

