from lempel_ziv77 import LempelZiv77
import time
import os
from huffman_code import HuffmanCode

"""
Tests for code implementing LempelZiv77 and huffman compression algorithms
"""

def test_huffman_code(file_name,output_dir):
    print(f"****Testing huffman code for {file_name}****")


    # file to lempel ziv
    WINDOW_SIZE = 1 << 15
    lempel_ziv = LempelZiv77(WINDOW_SIZE, file_name)
    vec_list_before = lempel_ziv.get_vecs()

    # lempel ziv vecs to huffman code
    start_comp = time.time()
    huffman_code = HuffmanCode(vec_list_before)
    base_name = os.path.basename(file_name)
    compressed_file_path = os.path.join(output_dir, f"{base_name}.huffman_coded")
    huffman_code.encode(compressed_file_path)
    end_comp = time.time()

    # huffman code to vecs
    start_decomp = time.time()
    huffman_code.set_coded_huffman_file_name(compressed_file_path)
    huffman_code.decode()
    end_decomp = time.time()
    vec_list_after = huffman_code.get_vec_list()

    # compare vecs
    if vec_list_before == vec_list_after:
        result = "success!!!"
    else:
        result = "fail :("

    print(f"trying to compress and decompress {file_name} was a: {result}")
    print(f"Time - Compress: {end_comp - start_comp:.5f}s | Decompress: {end_decomp - start_decomp:.5f}s")

    # --- Compression ratio calculation ---
    original_size = os.path.getsize(file_name)
    compressed_size = os.path.getsize(compressed_file_path)

    if original_size > 0:
        ratio = original_size / compressed_size if compressed_size > 0 else 0
        percent = (compressed_size / original_size) * 100
        status = "Reduced" if compressed_size < original_size else "Increased/Equal"
        print(f"Stats - Original: {original_size}B | Compressed: {compressed_size}B")
        print(f"Compression: {status} to {percent:.1f}% of original size (Ratio: {ratio:.2f}x)\n")

def run_huffman_compression_test():
    files = ['test_files/Samp1.bin', 'test_files/Samp2.bin', 'test_files/Samp3.bin',
             'test_files/Samp4.bin', 'test_files/Samp5.bin.amr']
    output_dir = "huffman_test_output_files"
    os.makedirs(output_dir, exist_ok=True)
    print()
    print(f"-------------Testing huffman compression-------------")
    print()
    for file_name in files:
        test_huffman_code(file_name,output_dir)

def test_LempelZiv77(file_name, window_size, output_dir):
    print(f"****Testing Lempel Ziv 77 for {file_name}****")

    # 1. Measure compression time
    start_comp = time.time()
    lz = LempelZiv77(window_size, file_name)
    end_comp = time.time()
    base_name = os.path.basename(file_name)

    # 2. Measure decompression time
    decompressed_file_name = "test_decompressed_file_" + base_name

    decompressed_file_path = os.path.join(output_dir, decompressed_file_name)

    start_decomp = time.time()
    lz.decompress_file(decompressed_file_path)
    end_decomp = time.time()

    # 3. Correctness check (compare file contents)
    with open(decompressed_file_path, "rb") as result_file, open(file_name, "rb") as start_file:
        if result_file.read() == start_file.read():
            result = "success"
        else:
            result = "fail"

        print(f"trying to compress and decompress {file_name} was a: {result}")
        print(f"Time - Compress: {end_comp - start_comp:.5f}s | Decompress: {end_decomp - start_decomp:.5f}s")

        # --- Compression ratio calculation ---
        original_size = os.path.getsize(file_name)
        # Theoretical estimation: each triplet (length, distance, literal) takes about 3 bytes without Huffman coding
        compressed_estimate = len(lz.window_vec) * 3

        if original_size > 0:
            ratio = original_size / compressed_estimate if compressed_estimate > 0 else 0
            percent = (compressed_estimate / original_size) * 100
            status = "Reduced" if compressed_estimate < original_size else "Increased/Equal"
            print(f"Stats - Original: {original_size}B | Est. Compressed: {compressed_estimate}B")
            print(f"Compression: {status} to {percent:.1f}% of original size (Ratio: {ratio:.2f}x)\n")

def run_LempelZiv77_advance_test(window_size):
    # Assumption: These files exist in the directory
    files = ['test_files/Samp1.bin', 'test_files/Samp2.bin', 'test_files/Samp3.bin',
             'test_files/Samp4.bin']

    output_dir = "advance_Lempel_ziv_test_output_files"
    os.makedirs(output_dir, exist_ok=True)

    print()
    print(f"-------------Testing advance Lempel ziv compression with window size: {window_size}-------------")
    print()
    for f in files:
        try:
            test_LempelZiv77(f, window_size, output_dir)
        except FileNotFoundError:
            print(f"Skipping {f} - File not found")

def basic_Lempel_ziv_test(data, num):
    output_dir = "basic_Lempel_ziv_test_output_files"
    os.makedirs(output_dir, exist_ok=True)

    print(f"-------------Testing basic Lempel ziv compression-------------")
    MAX_WINDOW_SIZE = 17

    file_name = f"basic_Lempel_ziv_test_data_{num}"
    file_path = os.path.join(output_dir, file_name)

    print(f"\n--- Test #{num} ---")
    print(f"Original data:", list(data))

    with open(file_path, 'wb') as data_f:
        data_f.write(data)

    # Compression
    lz = LempelZiv77(MAX_WINDOW_SIZE, file_path)

    print(f"Compression vectors:")
    window_vec = lz.window_vec
    past_start_index_vec = lz.past_start_index_vec
    literal_vec = lz.literal_vec
    print(f"(Window len, j, Literal)")
    for w, j, liter in zip(window_vec, past_start_index_vec, literal_vec):
        print(f"({w}, {j}, {liter})")

    print(f"Decompressing...")
    # Decompression - passing the vectors explicitly (Keyword Arguments)
    lz = LempelZiv77(MAX_WINDOW_SIZE, window_vec=window_vec, literal_vec=literal_vec,
                     past_start_index_vec=past_start_index_vec)

    output_filename = f"test_decompressed_file_{file_name}"
    output_file_path = os.path.join(output_dir, output_filename)

    lz.decompress_file(output_file_path)

    # Check the result
    with open(output_file_path, 'rb') as data_f:
        result_content = data_f.read()
        print(f"Result data:   {list(result_content)}")

        if result_content == data:
            print("Status: SUCCESS")
        else:
            print("Status: FAIL")

        # Compression ratio calculation ---
        original_size = len(data)
        compressed_estimate = len(window_vec) * 3
        if original_size > 0:
            ratio = original_size / compressed_estimate if compressed_estimate > 0 else 0
            percent = (compressed_estimate / original_size) * 100
            status = "Reduced" if compressed_estimate < original_size else "Increased"
            print(f"Compression Stats: {status} from {original_size}B to ~{compressed_estimate}B ({percent:.1f}%)")

def run_basic_LempelZiv77_tests():
    basic_Lempel_ziv_test(bytes([2, 1, 1, 1, 3, 1, 1, 1, 4, 5, 5, 6, 1, 1, 1, 3, 7]), 1)
    basic_Lempel_ziv_test(bytes([2, 1, 1, 1, 3, 1, 1, 1, 4, 5, 5, 6, 1, 1, 1, 3, 1]), 2)


if __name__ == '__main__':
     WINDOW_SIZE = [1 << 15]  # 16KB, 32KB, 64KB, 128KB
     run_basic_LempelZiv77_tests()
     print()

     for window_size in WINDOW_SIZE:
         run_LempelZiv77_advance_test(window_size)
         print()

     run_huffman_compression_test()