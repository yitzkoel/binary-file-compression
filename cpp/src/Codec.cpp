//
// Created by yitzk on 10/1/2026.
//

#include "Codec.h"
#include <filesystem>
#include <iostream>


void Codec::compress(std::string& file_path)
{
    BinaryIO::FileReader file(file_path);

    // if the file is alredy compressed we will do nothing
    if (file_already_compressed(file)) return;

    std::string compressed_file_path = file_path + EXTENTION_FORMAT.data();
    BinaryIO::FileWriter compressed_file(compressed_file_path);

    LempelZivCodec lempel_ziv_codec;
    HuffmanCodec huffman_codec{};

    // write global header (compression format, version, number of uncompressed file's bytes).
    write_global_header(file, compressed_file);

    while (file.read_bytes(read_buffer.data(), READ_BUFFER_SIZE))
    {
        auto lempel_ziv_vec = lempel_ziv_codec.compress(read_buffer.data(), file.get_num_bytes_read());

        auto num_bytes_in_compressed_file = huffman_codec.compress(write_buffer.data(), lempel_ziv_vec);

        // if the compression ratio isn't good then write the raw data without compressing adding a flag to indicate it
        if (num_bytes_in_compressed_file > file.get_num_bytes_read())
        {
            // write block's header (number of bytes in the block)
            size_t num_bytes_of_raw_block = file.get_num_bytes_read();
            compressed_file.write((const uint8_t*)&num_bytes_of_raw_block, sizeof(size_t));

            // write the symbol to indicate we copied the data raw without compression
            compressed_file.flush_buffer_to_file(&RAW_CODE_SYMBOLE, 1);

            // write the raw data
            compressed_file.flush_buffer_to_file(read_buffer.data(), file.get_num_bytes_read());
        }
        else
        {
            // write global block's header (number of bytes)
            compressed_file.write((const uint8_t*)&num_bytes_in_compressed_file, sizeof(size_t));

            compressed_file.flush_buffer_to_file(write_buffer.data(), num_bytes_in_compressed_file);
        }
    }
}

void Codec::decompress(std::string& file_path)
{
    // check if the file_path have the extention .ktn
    check_valid_path(file_path);

    // open the file to decompress
    BinaryIO::FileReader file(file_path);

    // get the header data
    uint8_t header_data[NUM_BYTES_IN_GLOBAL_HEADER];
    file.read_bytes(header_data, NUM_BYTES_IN_GLOBAL_HEADER);

    // check if the header is valid
    check_valid_global_header(header_data);

    //---------Extract the data from the header---------//

    // extract the file path without the format .ktn extention
    std::string decompressed_file_path = remove_format_extension(file_path);

    // extract the file uncompressed size from the header
    uint64_t uncompressed_size = *(uint64_t*)(header_data + (NUM_BYTES_IN_GLOBAL_HEADER - 8));


    // optimize the writing into the decompressed_file by telling the kernal the size of the expected file (linux only)
    optymize_writing_to_file(decompressed_file_path, uncompressed_size);

    // open the file to write the decompression into
    BinaryIO::FileWriter decompressed_file(decompressed_file_path);

    // prepare the decompression engines
    LempelZivCodec lempel_ziv_codec;


    size_t block_size = 0; // holds the block size
    size_t num_bytes_read = 0;

    while (num_bytes_read < uncompressed_size)
    {
        //-------------- Read the next block---------------//

        file.read_bytes((uint8_t*)&block_size, sizeof(size_t)); // read the block size in bytes
        file.read_bytes(read_buffer.data(), block_size); // read the block data into the read buffer
        num_bytes_read += block_size;

        // if the block wasn't compressed we just dump the data into the decompressed_file
        if (read_buffer[0] == RAW_CODE_SYMBOLE)
        {
            decompressed_file.flush_buffer_to_file(read_buffer.data(), block_size);
        }

        // else the data is compressed and we use the decompression engines to decompress
        else
        {
            auto lz_vec = HuffmanCodec::decompress(read_buffer.data(), block_size, READ_BUFFER_SIZE);
            auto num_bytes_decompressed = lempel_ziv_codec.decompress(write_buffer.data(),
                                                                      WRITE_BUFFER_SIZE, lz_vec);

            // write the decompressed data ito the file
            decompressed_file.flush_buffer_to_file(write_buffer.data(), num_bytes_decompressed);
        }
    }
}

void Codec::write_global_header(BinaryIO::FileReader& file, BinaryIO::FileWriter& compressed_file)
{
    // add to global header the format
    compressed_file.write(FORMAT_INDICATOR.data(), 4);

    // add to global header the version
    compressed_file.write(&VERSION, 1);

    // add to global header the size of the uncompressed file
    uint64_t size_original_file = file.get_file_size();
    compressed_file.write((uint8_t*)&size_original_file, 8);
}

void Codec::check_valid_global_header(uint8_t* header_data)
{
    int i = 0;
    for (; i < FORMAT_INDICATOR.size(); i++)
    {
        if (header_data[i] != FORMAT_INDICATOR[i]) throw std::runtime_error("Unsupported format");
    }

    if (header_data[i] != VERSION)
    {
        throw std::runtime_error(
            std::string("Unsupported version: file has ")
            + std::to_string(static_cast<unsigned>(header_data[i]))
            + ", expected " + std::to_string(static_cast<unsigned>(VERSION)));
    }
}

void Codec::check_valid_path(std::string& file_path)
{
    std::filesystem::path p(file_path);
    if (p.extension() != EXTENTION_FORMAT)
    {
        throw std::invalid_argument("Unsupported file format: expected a .ktn file, got: " + file_path);
    }
}

void Codec::optymize_writing_to_file(std::string& file_path, uint64_t estimated_file_size)
{
#ifdef __linux__
    // tell OS to reserve that size of file for the decompression
    int fd = ::open(file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ::fallocate(fd, 0, 0, static_cast<off_t>(estimated_file_size));
    ::close(fd);
#endif
}

bool Codec::file_already_compressed(BinaryIO::FileReader& file)
{
    // get the header data
    uint8_t header_data[NUM_BYTES_IN_GLOBAL_HEADER];

    file.read_bytes(header_data, NUM_BYTES_IN_GLOBAL_HEADER);

    // if the file doesnt have enough to hold a header (less than NUM_BYTES_IN_GLOBAL_HEADER) then it could not have been compressed
    if (file.get_num_bytes_read() < NUM_BYTES_IN_GLOBAL_HEADER) return false;

    // if the FORMAT_INDICATOR in the header is wrong then we did not compress it
    for (int i = 0; i < FORMAT_INDICATOR.size(); i++)
    {
        if (header_data[i] != FORMAT_INDICATOR[i]) return false;
    }

    return true;
}

std::string Codec::remove_format_extension(std::string& file_path)
{
    std::filesystem::path p(file_path);

    return p.replace_extension("").string();
}
