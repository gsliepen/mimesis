#pragma once

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

inline const std::filesystem::path test_data_dir = TEST_DATA_DIR;

inline std::string read_file(const std::filesystem::path &path) {
	std::ifstream in(path, std::ios::binary);
	return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

inline bool all_lines_at_most(const std::string &text, std::size_t length) {
	std::size_t start = 0;
	while (start < text.size()) {
		auto end = text.find("\r\n", start);
		if (end == std::string::npos)
			end = text.size();
		if (end - start > length)
			return false;
		start = end + 2;
	}
	return true;
}
