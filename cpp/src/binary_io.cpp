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

    std::shared_ptr<std::array<uint8_t,BUFFER_SIZE>> FileReader::get_buffer()
    {
        return buffer;
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

    void FileWriter::flush_buffer_to_file(const std::shared_ptr<std::array<uint8_t,BUFFER_SIZE>>& buffer,
                                          uint64_t num_bytes_to_flush)
    {
        file.write(reinterpret_cast<const char*>(buffer->data()), (long)num_bytes_to_flush);
    }

    FileWriter::~FileWriter()
    {
        file.close();
    }
}
