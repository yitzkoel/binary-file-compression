//
// Created by yitzk on 8/3/2026.
//

#ifndef BINARY_IO_H
#define BINARY_IO_H
#include <string>
#include<fstream>
#include <memory>

# define BUFFER_SIZE 2<<22 // 4MB

namespace binary_io
{
    /*
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
         * slides the window buffer 1MB forward..
         * @return true if there is more to slide and false otherwise.
         */
        bool slide_window();


        /**
         * getter of the past window.
         * @return the array that holds the past window of the file
         */
        std::shared_ptr<std::array<uint8_t,BUFFER_SIZE>> get_buffer();

        /**
         * Gets the number of bytes that where writin into the buffer (from index 0 to the returned value).
         * @return the number of bytes read from the file into the buffer
         */
        uint64_t get_num_bytes_read() const;

        ~FileReader();

    private:
        std::shared_ptr<std::array<uint8_t, BUFFER_SIZE>> buffer =
         std::make_shared<std::array<uint8_t,BUFFER_SIZE>>();
        uint64_t num_bytes_read = 0;
        std::ifstream file;
    };


    class FileWriter
    {
    public:
        /**
         *  The constructor to a file to write into
         * @param file_path the path of the file to write into
         */
        explicit FileWriter(const std::string& file_path);

        /**
         *  This method writes 'num_bytes_to_flush' bytes from 'buffer' into the file.
         * @param buffer the buffer to write from into the file
         * @param num_bytes_to_flush the number of bytes to read from the file
         */
        void flush_buffer_to_file(const std::shared_ptr<std::array<uint8_t,BUFFER_SIZE>>& buffer,
                                  uint64_t num_bytes_to_flush);

        /**
         * Closes the file.
         */
        ~FileWriter();

    private:
        std::ofstream file;
    };
}


#endif //BINARY_IO_H
