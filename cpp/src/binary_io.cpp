//
// Created by yitzk on 8/3/2026.
//

#include "binary_io.h"


namespace binary_io
{
    FileReader::FileReader(const std::string& file_name )
    {
        file = std::ifstream(file_name, std::ios::binary);

        if(!file) throw std::runtime_error("Failed to open file");

        file.read(reinterpret_cast<char*> (future.data()),BUFFER_SIZE);
    }
    void FileReader::slide_window()
    {
        if(file.eof()) return;
        past = future;
        file.read(reinterpret_cast<char*> (future.data()),BUFFER_SIZE);
    }
    std::array<uint8_t,BUFFER_SIZE>&  FileReader::get_past()
    {
        return past;
    }

    std::array<uint8_t,BUFFER_SIZE>&  FileReader::get_future()
    {
        return future;
    }

    FileReader::~FileReader()
    {
        file.close();
    }
}
