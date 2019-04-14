/* This test reads each input file in a string,
 * converts the string to a Mimesis::Part,
 * then converts the Mimesis::Part to a string.
 * The test succeeds if the input and output strings are identical.
 */

#include <iostream>
#include <sstream>
#include <fstream>

#include <mimesis.hpp>

using namespace std;

static bool parse_reproduce(istream &in, ostream &out) {
	stringstream sin;
	stringstream sout;
	sin << in.rdbuf();

	Mimesis::Part msg;
	try {
		msg.load(sin);
	} catch(runtime_error &e) {
		return false;
	}
	msg.save(sout);
	out << sout.rdbuf();

	return sin.str() == sout.str();
}

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		return parse_reproduce(cin, cout) ? 0 : 1;
	}

	for (int i = 1; i < argc; i++) {
		ifstream in(argv[i]);
		if (!parse_reproduce(in, cout))
			return 1;

	}

	return 0;
}
