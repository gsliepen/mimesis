#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>

#include <mimesis.hpp>

using namespace std;

int main(int argc, char *argv[]) {
	assert(argc == 3);

	// Load from file
	Mimesis::Message msg;
	msg.load(argv[1]);

	// Save to file
	msg.save(argv[2]);

	// Save to and load from string
	{
		string str = msg.to_string();
		Mimesis::Message msg2;
		msg2.from_string(str);
		assert(msg2 == msg);
	}

	// Save to and load from stream using operators
	{
		stringstream ss;
		ss << msg;
		Mimesis::Message msg2;
		ss >> msg2;
		assert(!(msg2 != msg));
	}
}

