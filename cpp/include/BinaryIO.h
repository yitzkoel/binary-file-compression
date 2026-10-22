//
// Created by yitzk on 8/3/2026.
//

#ifndef BINARY_IO_H
#define BINARY_IO_H
#include <string>
#include<fstream>
#include <memory>


namespace BinaryIO
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
         * This function reads 'count' bytes into the address dest, from the file.
         * If the file has less than count bytes in it it will read only what is left.
         *
         * @param dest the buffer to read into the data from the file
         * @param count the number of bytes to read from the file
         * @return true if successful (we havent reached the eof), and false if there is no bytes to read (eof).
         */
        bool read_bytes(uint8_t* dest, size_t count);

        /**
         * Gets the number of bytes that where writin into the buffer (from index 0 to the returned value).
         * @return the number of bytes read from the file into the buffer
         */
        uint64_t get_num_bytes_read() const;

        uint64_t get_file_size();

        ~FileReader();

    private:
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
         * This method writes 'num_bytes_to_flush' bytes from the buffer into the file.
         * @param buffer the buffer to write into the file
         * @param num_bytes_to_flush the number of bytes to write
         */
        void flush_buffer_to_file(const uint8_t* buffer, uint64_t num_bytes_to_flush);

        void write(const uint8_t* buffer, uint64_t num_bytes);

        /**
         * Closes the file.
         */
        ~FileWriter();

    private:
        std::ofstream file;
    };
}


#endif //BINARY_IO_H
