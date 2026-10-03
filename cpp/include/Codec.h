//
// Created by yitzk on 10/1/2026.
//

#ifndef CODEC_H
#define CODEC_H
#include <string>
#include <BinaryIO.h>
#include <HuffmanCodec.h>
#include <LempelZivCodec.h>
#include <bits/regex.h>


class Codec
{
public:
    static void compress(std::string& file_path);

    void decompress(std::string& file_path);

private:

    static std::string remove_format_postfix(std::string& file_path);



    static constexpr size_t LEN_CODE_LEN_TABLE = HuffmanCodec::DISTANCE_NUM_SYMBOLS +
        HuffmanCodec::LITERAL_AND_LEN_NUM_SYMBOLS;

    static constexpr size_t COMPRESSION_UPPER_BOUND = BUFFER_SIZE - LEN_CODE_LEN_TABLE;

    static  const uint8_t RAW_CODE_SYMBOLE = UINT8_MAX;

    static constexpr std::string_view POSTFIX_FORMAT = ".ktn";
};


#endif //CODEC_H
