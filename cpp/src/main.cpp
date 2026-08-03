//
// Created by yitzk on 7/29/2026.
//

#include <iostream>
#include <sys/stat.h> // POSIX standard for file information

int main() {
    struct stat fileInfo;

    // Ask the OS for metadata about this specific file
    if (stat("../test_files/Samp1.bin", &fileInfo) == 0) {

        // st_blksize is the OS's recommended I/O chunk size for performance
        std::cout << "Optimal I/O Block Size: " << fileInfo.st_blksize << " bytes\n";
        std::cout << "Total File Size: " << fileInfo.st_size << " bytes\n";

    } else {
        std::cerr << "Failed to get file stats.\n";
    }

    return 0;
}