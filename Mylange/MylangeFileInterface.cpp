#include <iostream>
#include <string>
#include <fstream>

#include "MylangeFileInterface.h"

std::string read_file_into_string(const std::string& filename) {
    // Open the file. Using the constructor handles the open call.
    std::ifstream ifs(filename);

    // Check if the file opened successfully
    if (!ifs.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return ""; // Or throw an exception
    }

    // Use iterators to read the entire stream into the string
    std::string content(std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>());

    // The ifstream destructor automatically closes the file when it goes out of scope.
    return content;
}




int MylangeFileInterface::ReadFile(const std::string& filePath)
{
    std::cout << "Running Mylange script: " << filePath << std::endl;

    std::ifstream file(filePath);
    std::ostringstream content_stream;
    content_stream << file.rdbuf(); // Read the entire buffer into the stream
    std::string entireFileContent = content_stream.str();

    return 0;
}
