#include "ISOReader.h"
#include <iostream>
#include <string>
#include <algorithm>

void printUsage(const char* programName) {
    std::cout << "ISO Reader - A tool to read and list files in ISO images\n\n"
              << "Usage:\n"
              << "  " << programName << " <iso_file>         - List all files in the ISO\n"
              << "  " << programName << " <iso_file> <pattern> - Search for files matching pattern\n\n"
              << "Examples:\n"
              << "  " << programName << " myfile.iso         - List all files\n"
              << "  " << programName << " myfile.iso .txt    - Find all text files\n"
              << "  " << programName << " myfile.iso readme  - Find files containing 'readme'\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    try {
        ISOReader reader(argv[1]);
        
        if (!reader.open()) {
            std::cerr << "Error: Failed to open ISO file: " << reader.getLastError() << std::endl;
            return 1;
        }

        std::cout << "Reading ISO file: " << argv[1] << std::endl;
        auto files = reader.listFiles();
        
        if (files.empty()) {
            std::cout << "No files found in ISO image." << std::endl;
            return 0;
        }

        if (argc > 2) {
            // Search mode
            std::string pattern = argv[2];
            std::cout << "\nSearching for files matching '" << pattern << "':\n";
            bool found = false;

            for (const auto& file : files) {
                if (file.find(pattern) != std::string::npos) {
                    std::cout << "\nFile: " << file << std::endl;
                    found = true;

                    std::vector<char> content;
                    if (reader.readFile(file, content)) {
                        std::cout << "  Content preview (first 100 bytes):\n  ";
                        size_t preview_size = std::min(content.size(), size_t(100));
                        for (size_t i = 0; i < preview_size; ++i) {
                            char c = content[i];
                            if (isprint(static_cast<unsigned char>(c))) {
                                std::cout << c;
                            } else {
                                std::cout << '.';
                            }
                        }
                        std::cout << std::endl;
                    } else {
                        std::cerr << "  Failed to read file: " << reader.getLastError() << std::endl;
                    }
                }
            }

            if (!found) {
                std::cout << "No files found matching the pattern '" << pattern << "'" << std::endl;
            }
        } else {
            // List mode
            std::cout << "\nFiles in ISO (" << files.size() << " files found):\n";
            for (const auto& file : files) {
                std::cout << "  " << file << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}