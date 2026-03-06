import argparse
import time
import os
from lempel_ziv77 import LempelZiv77
from huffman_code import HuffmanCode

def compress(input_path: str, output_path: str, window_size: int = 1 << 15) -> None:
    start_time = time.time()
    print(f"{input_path} is compressing....")
    # compression logic
    lempel = LempelZiv77(window_size=window_size, file_to_compress_name=input_path)
    lempel.compress_file()
    lempel_vecs = lempel.get_vecs()
    huffman = HuffmanCode(vec_list=lempel_vecs)
    huffman.encode(name_file_to_code_to=output_path)

    end_time = time.time()
    print(f"Compression Complete!!!")

    # performance metrics
    original_size = os.path.getsize(input_path)
    compressed_size = os.path.getsize(output_path)

    ratio = original_size / compressed_size if compressed_size > 0 else 0
    print(f"\n------metrics------")
    print(f"Time taken: {end_time - start_time:.2f} seconds")
    print(f"Original size: {original_size} Bytes")
    print(f"Compressed size: {compressed_size} Bytes ({(compressed_size / original_size) * 100:.1f}%)")
    print(f"Compression Ratio: {ratio:.2f}x\n")




def decompress(input_path: str, output_path: str, window_size: int = 1 << 15) ->None:
    start_time = time.time()
    print(f"{input_path} is decompressing....")

    # decompression logic
    huffman = HuffmanCode(huffman_code_file_name=input_path)
    huffman.decode()
    lempel_vecs = huffman.get_vec_list()
    lempel = LempelZiv77(window_size=window_size,window_vec=lempel_vecs[0],
                         literal_vec=lempel_vecs[1],past_start_index_vec=lempel_vecs[2])
    lempel.decompress_file(dest_file_name=output_path)

    end_time = time.time()
    print(f"Decompression Complete!!!!")

    # performance metrics
    original_size = os.path.getsize(input_path)
    decompressed_size = os.path.getsize(output_path)

    ratio = decompressed_size / original_size if decompressed_size > 0 else 0
    print(f"\n------metrics------")
    print(f"Time taken: {end_time - start_time:.2f} seconds")
    print(f"Original size: {original_size} Bytes")
    print(f"Decompressed size: {decompressed_size} Bytes ({(decompressed_size / original_size) * 100:.1f}%)")
    print(f"Expansion Ratio: {ratio:.2f}x\n")


def main():
    # create the parse obj
    parser = argparse.ArgumentParser(description="A binary file compression tool")

    # Add the compression commands
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-c', '--compress', action='store_true', help="Compress a file using LZ77 and Huffman")
    group.add_argument('-d', '--decompress', action='store_true', help="Decompress a file using LZ77 and Huffman")

    # Add the file paths
    parser.add_argument('-i', '--input', type=str, required=True, help='Path to the input file')
    parser.add_argument('-o', '--output', type=str, required=True, help='Path to the output file')

    # activate parser
    args = parser.parse_args()

    WINDOW_SIZE = 1 << 15  # 32KB

    # run the compression methods
    if args.compress:
        compress(args.input, args.output, WINDOW_SIZE)
    elif args.decompress:
        decompress(args.input, args.output, WINDOW_SIZE)

if __name__ == "__main__":
    main()
