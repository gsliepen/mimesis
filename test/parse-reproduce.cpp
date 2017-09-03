#include <iostream>
#include <mimesis.hpp>

int main(int argc, char *argv[]) {
	Mimesis::MIMEPart msg;
	msg.load(std::cin);
	msg.save(std::cout);
}
