#include <iostream>
#include <string>

#include <mimesis.hpp>

using namespace std;

static string indent(int level) {
	return string(level, '\t');
}

static void describe_part(Mimesis::Part &part, int level) {
	string content_type = part.get_header("Content-Type");
	string from = part.get_header("From");

	if (!from.empty())
		cout << indent(level) << "From: " << from << "\n";

	cout << indent(level) << content_type << "\n";

	if (part.is_multipart())
		for (auto &child: part.get_parts())
			describe_part(child, level + 1);
}

int main() {
	Mimesis::Part msg;
	try {
		msg.load(cin);
	} catch (runtime_error &e) {
		cout << "Invalid message\n";
		return 1;
	}
	describe_part(msg, 0);
}
