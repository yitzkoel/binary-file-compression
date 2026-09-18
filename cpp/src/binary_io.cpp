//
// Created by yitzk on 8/3/2026.
//

#include "../include/binary_io.h"


namespace binary_io
{
    FileReader::FileReader(const std::string& file_name)
    {
        file = std::ifstream(file_name, std::ios::binary);

        if (!file) throw std::runtime_error("Failed to open file");
    }

    bool FileReader::slide_window()
    {
        if (file.eof()) return false;

        file.read(reinterpret_cast<char*>(buffer->data()),BUFFER_SIZE);

        num_bytes_read += file.gcount();

        return true;
    }

    bool FileReader::read_bytes(uint8_t* dest, size_t count)
    {
        if (file.eof()) return false;

        file.read(reinterpret_cast<char*>(dest),count);

        num_bytes_read += file.gcount();

        return true;
    }

    uint8_t* FileReader::get_buffer()
    {
        return buffer->data();
    }

    uint64_t FileReader::get_num_bytes_read() const
    {
        return num_bytes_read;
    }

    FileReader::~FileReader()
    {
        file.close();
    }

    FileWriter::FileWriter(const std::string& file_path)
    {
        file = std::ofstream(file_path, std::ios::binary);
    }

    void FileWriter::flush_buffer_to_file(uint64_t num_bytes_to_flush)
    {
        file.write(reinterpret_cast<const char*>(buffer->data()), (long)num_bytes_to_flush);
    }

    uint8_t* FileWriter::get_buffer()
    {
        return buffer->data();
    }

    FileWriter::~FileWriter()
    {
        file.close();
    }
}
