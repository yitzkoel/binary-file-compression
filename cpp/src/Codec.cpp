//
// Created by yitzk on 10/1/2026.
//

#include "Codec.h"
#include <filesystem>
#include <iostream>


void Codec::compress(std::string& file_path)
{
    BinaryIO::FileReader file(file_path);
    std::string compressed_file_path = file_path + POSTFIX_FORMAT.data();
    BinaryIO::FileWriter compressed_file(compressed_file_path);

    LempelZivCodec lempel_ziv_codec;
    HuffmanCodec huffman_codec{};

    // TODO add global header to the compressed file
    // TODO 1. the format (why?) 2. the number of bytes the original file took

    while (file.slide_window())
    {
        auto lempel_ziv_vec = lempel_ziv_codec.compress(file.get_buffer(), file.get_num_bytes_read());

        auto num_bytes_in_compressed_file = huffman_codec.compress(compressed_file.get_buffer(), lempel_ziv_vec);

        // if the compression didnt compress then write the raw data without compressing adding a flag to indicate it
        if (num_bytes_in_compressed_file >= COMPRESSION_UPPER_BOUND)
        {
            // write the symbol to indicate we copied the data raw without compression
            compressed_file.flush_buffer_to_file(&RAW_CODE_SYMBOLE, 1);

            // write the data
            compressed_file.flush_buffer_to_file(file.get_buffer(), file.get_num_bytes_read());
        }
        else
        {
            compressed_file.flush_buffer_to_file(num_bytes_in_compressed_file);
        }
    }
}

void Codec::decompress(std::string& file_path)
{
    // prepare the file
    BinaryIO::FileReader file(file_path);

    // extract the file path without the format .ktn extention
    std::string decompressed_file_path =remove_format_postfix(file_path);

    // prepare the decompressed file
    BinaryIO::FileWriter decompressed_file(decompressed_file_path);

    // read the first 8 bytes to read the number of bytes the original file took

    // tell the Kernal to prepare a file of that size

    // decompress each block devide the case in wich the first byte is 255 and not(raw code or huffman code)

    // please note that we will need try catch since we might have got a corrupted file that is not in the format
    // please think if there are ways for me here to catch wrong format(probobly not)
}

std::string Codec::remove_format_postfix(std::string& file_path)
{
    std::filesystem::path p(file_path);

    if (p.extension() != POSTFIX_FORMAT) {
        throw std::invalid_argument("Unsupported file format: expected a .ktn file, got: " + file_path);
    }

    return p.replace_extension("").string();
}
