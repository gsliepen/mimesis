#include <iostream>
#include <fstream>

#include <mimesis.hpp>

using namespace std;

int main() {
	// 01-simple
	Mimesis::Message msg;
	msg["From"] = "Some One <some.one@example.org>";
	msg["To"] = "Someone Else <someone.else@example.org>";
	msg["Subject"] = "Test";
	msg.set_plain("Hello!\r\n");
	msg.save("01-simple");

	// 02-multipart-alternative
	msg.clear();
	msg["From"] = "Some One <some.one@example.org>";
	msg["To"] = "Someone Else <someone.else@example.org>";
	msg["Subject"] = "Test";
	msg.set_plain("Hello!\r\n");
	msg.set_html("<p>Hello!</p>\r\n");
	msg.save("02-multipart-alternative");

	// 03-multipart-mixed
	msg.clear();
	msg["From"] = "Some One <some.one@example.org>";
	msg["To"] = "Someone Else <someone.else@example.org>";
	msg["Subject"] = "Test";
	msg.set_plain("Hello!\r\n");
	msg.attach("This is the attachment.\r\n", "text/plain");
	msg.save("03-multipart-mixed");

	// 04-nested-multipart
	msg.clear();
	msg["From"] = "Some One <some.one@example.org>";
	msg["To"] = "Someone Else <someone.else@example.org>";
	msg["Subject"] = "Test";
	msg.set_plain("Hello!\r\n");
	msg.set_html("<p>Hello!</p>\r\n");
	msg.attach("This is the attachment.\r\n", "text/plain");
	msg.save("04-nested-multipart");

	// 05-nested-message-rfc822
	Mimesis::Part outer;
	outer["From"] = "Some One <some.one@example.org>";
	outer["To"] = "Someone Else <someone.else@example.org>";
	outer["Subject"] = "Test";
	msg["Subject"] = "Nested test";
	outer.attach(msg);
	outer.save("05-nested-message-rfc822");
}

