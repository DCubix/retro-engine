#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

int main(int argc, char** argv) {
	if (argc < 3) {
		std::cerr << "Usage: shader_header_gen <output_file> <shader_file>" << std::endl;
		return 1;
	}

	std::string outputFile = argv[1];
	std::string inputFile = argv[2];

	std::ifstream input(inputFile);
	std::ofstream output(outputFile);
	if (!input.is_open()) {
		std::cerr << "Error: Could not open file " << inputFile << std::endl;
		return 1;
	}

	auto path = std::filesystem::path(inputFile);
	std::string fileNameWE = path.filename().replace_extension("").string();
	std::string ext = path.extension().string();

	// replace spaces and special chars with _
	for (char& c : fileNameWE) {
		if (!isalnum(c)) {
			c = '_';
		}
	}

	std::transform(fileNameWE.begin(), fileNameWE.end(), fileNameWE.begin(), ::tolower);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	std::string shaderName = fileNameWE + "_" + ext.substr(1);

	output << "#pragma once\n\n";
	output << "#include <string>\n";
	output << "const std::string " << shaderName << "_src = R\"(";
	std::string line;
	while (std::getline(input, line)) {
		output << line << "\n";
	}
	output << ")\";\n";
	input.close();
	output.close();

	return 0;
}