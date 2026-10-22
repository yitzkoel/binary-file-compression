//
// Created by yitzk on 8/3/2026.
//

#include "../include/BinaryIO.h"

#include <filesystem>


namespace BinaryIO
{
    FileReader::FileReader(const std::string& file_name)
    {
        file = std::ifstream(file_name, std::ios::binary);

        if (!file) throw std::runtime_error("Failed to open file");
    }

    bool FileReader::read_bytes(uint8_t* dest, size_t count)
    {
        if (file.eof()) return false;

        file.read(reinterpret_cast<char*>(dest),count);

        num_bytes_read += file.gcount();

        return true;
    }

    uint64_t FileReader::get_num_bytes_read() const
    {
        return num_bytes_read;
    }

    uint64_t FileReader::get_file_size()
    {
        auto cur = file.tellg();
        file.seekg(0, std::ios::end);
        auto size = file.tellg();    // or just f.tellg() for total size
        file.seekg(cur);
        return size;
    }

    FileReader::~FileReader()
    {
        file.close();
    }

    FileWriter::FileWriter(const std::string& file_path)
    {
        file = std::ofstream(file_path, std::ios::binary);
    }

    void FileWriter::flush_buffer_to_file(const uint8_t* buffer, uint64_t num_bytes_to_flush)
    {
        file.write(reinterpret_cast<const char*>(buffer), (long)num_bytes_to_flush);
    }

    void FileWriter::write(const uint8_t* buffer, uint64_t num_bytes)
    {
        file.write(reinterpret_cast<const char*>(buffer), (long)num_bytes);
    }

    FileWriter::~FileWriter()
    {
        file.close();
    }
}
