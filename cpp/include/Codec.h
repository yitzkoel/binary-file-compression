//
// Created by yitzk on 10/1/2026.
//

#ifndef CODEC_H
#define CODEC_H
#include <string>
#include <BinaryIO.h>
#include <HuffmanCodec.h>
#include <LempelZivCodec.h>
#include <regex>

#ifdef __linux__

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#endif


/**
* =========================================================================================
* 1. THE CODING FORMAT
* =========================================================================================
* The file is structured as a Global Header followed by one or more Compressed Blocks.
*
* [Global Header]
* <Format name> -> 4 bytes holding - 0xFF + ASCII(ktn)
* <Format version> -> 1 byte (currently version 1 is the only one)
* <original_file_size>  -> 8 bytes (uint64_t). The total uncompressed size of the original file.
*
* [Block i] (Repeats until End Of File)
* <Block header>
*      <compressed size> -> 4 bytes holding the number of bytes of the compressed file
*      <raw data(optional)> -> 1 byte holding if the blocks data is raw uncompressed data,
*                              used to when entropy is large and we can't achieve compression.
* <Tree 1 Definition (Literals & Lengths)>
*      <bit_lengths_1>  -> Array of 1 byte lengths used to reconstruct the Canonical Huffman tree.
*                          (Symbol values:
                                            0-255:   Literals,
                                            256:     EOF(end of file)
                                            256-284: Length Ranges from ). // TODO add the discription of the ranges
*
* <Tree 2 Definition (Distances)>
*      <bit_lengths_2>  -> Array of lengths used to reconstruct the Canonical Huffman tree.
*                          (Symbol values represent Distance Ranges).
*                          // TODO add the discription of the ranges
*
* <Bitstream (The Encoded LZSS Data)>
*      The bitstream contains a sequence of encoded elements. For each element:
*      a. 'literal' (< 256):     Encode <Huffman Code of literal from Tree 1>
*      b. 'length' (length+diff): Encode <Huffman Code of length Symbol from Tree 1> + <Extra Bits for diff>
*                                Encode <Huffman Code of Distance Symbol from Tree 2> + <Extra Bits for diff>
*      c. 'end of block':        Encode <Huffman Code of 256 from Tree 1>
*
*/


class Codec
{
public:
    /**
     * This function recieves a file and compresses it into the file "file_path.ktn".
     * If the file was previously compressed by this compressor it will do nothing.
     *
     * @param file_path the file to compress
     */
    static void compress(std::string& file_path);

    /**
     * This function get's a file in the ktn format and decompresses it.
     * If the file is not in the format or was corrupted it will throw a runtime_error that needs to be catched.
     *
     * @param file_path the file to decompress
     */
    static void decompress(std::string& file_path);

private:
    /**
     * This function only works on linux, and informes the kernal to optimise writing into that file by telling the
     * kernal the file size.
     * @param file_path the file to be writtin to
     * @param estimated_file_size the estimated number of bytes that will be written into the file
     */
    static void optymize_writing_to_file(std::string& file_path, uint64_t estimated_file_size);

    /**
     * This function checks if the file is already compressed by cheching its format (first 4 Bytes).
     * @param file the file to check
     * @return true if the file was already compressed, false otherwise
     */
    static bool file_already_compressed(BinaryIO::FileReader& file);

    /**
     * this function returns a file_path without the .ktn extention.
     * @param file_path the file with the .ktn extention
     * @return the file path withou the .ktn extention
     */
    static std::string remove_format_extension(std::string& file_path);

    /**
     * This function extracts from 'file' the information it needs and encodes into 'compressed_file' the global
     * compression header (can look bellow to see what is in it).
     *
     * @param file the file we want to compress
     * @param compressed_file the file we write the compressed data into
     */
    static void write_global_header(BinaryIO::FileReader& file, BinaryIO::FileWriter& compressed_file);

    /**
     * check if the file actully is a compressed ktn format file by looking into the header.
     * @param header_data the header data array
     */
    static void check_valid_global_header(uint8_t* header_data);

    /**
     * this function checks if the file path is a compressed file path (with the .ktn extention).
     * @param file_path the path to examine
     */
    static void check_valid_path(std::string& file_path);


    // the size of the encoded len code table in the head of a compressed block
    static constexpr size_t LEN_CODE_LEN_TABLE = HuffmanCodec::DISTANCE_NUM_SYMBOLS +
        HuffmanCodec::LITERAL_AND_LEN_NUM_SYMBOLS;

    static constexpr size_t READ_BUFFER_SIZE = 1 << 22; // 4MB
    static constexpr size_t WRITE_BUFFER_SIZE = 1 << 22 + 1 << 29; // 4.5MB

    // the buufers used to encode and decode
    static std::array<uint8_t, READ_BUFFER_SIZE> read_buffer;
    static std::array<uint8_t, WRITE_BUFFER_SIZE> write_buffer;

    // the symbol that indicates if a block is raw data(no compression was done) or is compressed data
    static const uint8_t RAW_CODE_SYMBOLE = UINT8_MAX;

    // the format file path extention
    static constexpr std::string_view EXTENTION_FORMAT = ".ktn";

    // the coded indicator at the begining of every knt format file to indicate the format
    static constexpr std::array<std::uint8_t, 4> FORMAT_INDICATOR{0xFF, 0x6B, 0x74, 0x6E}; // 0xFF,"ktn"

    // the current version of the compression, used to make sure we dont try to decompress a unsupported verssion
    static constexpr uint8_t VERSION = 1;

    // the number of bytes at the begining of the file that encode the global header
    static constexpr uint8_t NUM_BYTES_IN_GLOBAL_HEADER = 13;
};


#endif //CODEC_H
