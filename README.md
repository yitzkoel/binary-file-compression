# Binary Compression project

## About

This project is a python implementation of the lempel ziv 77 and huffman code compression algorithms.  
It is implemented using python for clear algorithmic exposition.


## Installation
Simply clone or download the repository to your local machine:
```bash
git clone https://github.com/yitzkoel/binary-file-compression.git
```

## Usage
The project uses a simple shell interface implemented using argparse.  
open terminal in the directory of the main.py file.  
**To compress a file:**  
```bash
python main.py -c -i "input_file_to_compress" -o "output_file_to_compress"  
```

**To decompress a file:**  
```bash
python main.py -d -i "input_file_to_decompress" -o "output_file_to_decompress"
```
**Note**  
This project does not actively validate if a provided compressed binary file is in the correct format before decompression.  
Providing an improperly formatted or uncompressed file to the -d flag will result in undefined behavior

## Key Features
-  **Lempel Ziv 77 algorithm:** Dynamic sliding window compression.  
-  **Canonical Huffman Coding:** Optimal prefix code generation based on symbol frequencies.
-  **Bit & Byte Level I/O Management:** Custom buffer handling to minimize overhead.

## Implementation Details
### The Lempel Ziv 77 algorithm
Is implemented with a search window of 32KB (with an option to easily modify the window size).   
The window search is implemented using binary search that searches for the largest window match in the past.   
It is implemented with Python's 'memoryview' to reduce overhead and memory duplication during the search process.

### The huffman code algorithm
The code is implemented using code lengths and with the canonical huffman coding.
Canonical huffman code is implemented with this algo:  
sort the symbols by length of huffman code.  
From the smallest huffman code to the largest we do:
1. pre_huffman_code << (cur_len - pre_len), perform a bitwise left shift to the previous code by the difference in depth.
2. we add 1 to the code.

#### Binary File Coding Format:  
-  **num of symbols**  
-  **symbols dict:** (symbol_1,len), (symbol_2,len),....  
-  **vec 1**  
-  **vec 2**  
-  more vectors....  
#### Header Structure:
**num of symbols**   
Coded using 2 bytes (we assume that there is maximum 257 symbols)  
**symbols dict**  
each *symbol* and *len* is coded separately.  
**symbol**  
coded using 2 bytes.   
All regular symbols in the huffman code are defined to be int in the range 0 - 255,
In addition, we have 2 more symbols:
1. vec separator symbol (= 256) that notes a seperator between vectors.
2. end of file symbol (= 257) that notes that we reached the end of the file.

**len**  
Coded using 2 bytes (we assume that there are at most 257 symbols, therefore the max depth of the tree is 257 that can be represented with 2 bytes)  
**vec i:**  
For each 'literal' in the vec_i there are 3 cases:
1. 'literal' is smaller than 256: encode the bits '0' + 'huffman code[literal]'
2. 'literal' is bigger than 255: represent 'literal' is base 256
(we assume that there are no literals bigger than 256^2 -1) so we will get
'literal' = ab, such that a and b are smaller than 256.
Then we will encode '1' + 'huffman code[a]' + 'huffman code[b]'
3. We reach the end of the vector, so we encode '0' + 'huffman code[256]'

## Example Performance
```plaintext
python main.py -c -i test_files/Samp1.bin -o test_files/Samp1.compressed   
test_files/Samp1.bin is compressing....   
Compression Complete!!!

------metrics------   
Time taken: 1.72 seconds   
Original size: 65728 Bytes   
Compressed size: 30890 Bytes (47.0%)  
Compression Ratio: 2.13x

python main.py -d -i test_files/Samp1.compressed -o test_files/Samp1.decompressed   
test_files/Samp1.compressed is decompressing....

Decompression Complete!!!!
  
------metrics------   
Time taken: 0.08 seconds   
Original size: 30890 Bytes   
Decompressed size: 65728 Bytes (212.8%)  
Expansion Ratio: 2.13x
```

## Contact

If you have any questions or suggestions, feel free to reach out.

-   **Email**: yitzkoel@gmail.com
-   **LinkedIn**: www.linkedin.com/in/yitzhak-koelewyn

