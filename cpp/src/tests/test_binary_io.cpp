//
// Created by yitzk on 8/5/2026.
//

#include <iostream>
#include <iterator>

#include "../binary_io.h"
#include <string>

void test_reading_file(std::string& file_path)
{
    // binary_io::FileReader file;
    try
    {
        binary_io::FileReader file = binary_io::FileReader(file_path);
        do
        {
            auto& past = file.get_past();
            auto& future = file.get_future();


            std::cout << "***********   PAST   ************\n";
            for (unsigned char byte : past) {
                std::cout << byte;
            }
            std::cout << std::endl;
            std::cout << "***********   FUTURE   ************\n";
            for (unsigned char byte : past) {
                std::cout << byte;
            }
            std::cout << std::endl;
        }
        while (file.slide_window());
    }
    catch (std::runtime_error& err)
    {
        std::cout << err.what() << std::endl;
    }
}

void run_test_reading_from_file()
{
    std::string file_path_prefix = "../test_files/Samp";
    std::string file_path_postfix = ".bin";

    for (int i = 1; i < 2; i++)
    {
        std::string file_path = file_path_prefix + std::to_string(i) + file_path_postfix;
        test_reading_file(file_path);
    }
}
