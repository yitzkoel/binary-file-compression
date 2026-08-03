//
// Created by yitzk on 8/3/2026.
//

#ifndef BINARY_IO_H
#define BINARY_IO_H
#include <string>
#include<fstream>
#include <memory>

# define BUFFER_SIZE 2<<20 // 1MB

namespace binary_io
{   /*
    * This class provides an abstraction to reading from a binary file.
    * It is designed for the lempel ziv77 algo.
    */
    class FileReader
    {
    public:
        /**
         * opens the file in binary mode for reading.
         * @param file_name the name of the file to be read.
         */
        explicit FileReader(const std::string& file_name);

        /**
         * slides the window BUFFER_SIZE(1MB) into the future of the file and saves it in the future var
         * The past var is updated to what was the future before the slide.
         */
        void slide_window();
        /**
         * getter of the past window.
         * @return the array that holds the past window of the file
         */
        std::array<uint8_t,BUFFER_SIZE>& get_past();

        /**
         * getter of the future window.
         * @return the array that holds the future window of the file
         */
        std::array<uint8_t,BUFFER_SIZE>& get_future();

        ~FileReader();

    private:
        std::array<uint8_t,BUFFER_SIZE> past{};
        std::array<uint8_t,BUFFER_SIZE> future{};
        std::ifstream file;
    };

    class FileWriter
    {
    };
}


#endif //BINARY_IO_H
