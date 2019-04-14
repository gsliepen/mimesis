#include <iostream>
#include <fstream>
#include <string>

#include <mimesis.hpp>

using namespace std;

static string indent(int level) {
	return string(level, '\t');
}

static void describe_part(Mimesis::Part &part, int level, ostream &out) {
	string content_type = part.get_header("Content-Type");
	string from = part.get_header("From");

	if (!from.empty())
		out << indent(level) << "From: " << from << "\n";

	out << indent(level) << content_type << "\n";

	if (part.is_multipart())
		for (auto &child: part.get_parts())
			describe_part(child, level + 1, out);
}

static bool describe(istream &in, ostream &out) {
	Mimesis::Part msg;
	try {
		msg.load(in);
	} catch (runtime_error &e) {
		out << "Invalid message\n";
		return false;
	}
	describe_part(msg, 0, out);
	return true;
}

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		return describe(cin, cout) ? 0 : 1;
	}

	for (int i = 1; i < argc; i++) {
		ifstream in(argv[i]);
		if (!describe(in, cout))
			return 1;

	}

	return 0;
}
