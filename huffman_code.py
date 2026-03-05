from __future__ import annotations

import heapq
from collections import Counter
import itertools
import math

from bitarray import bitarray

from binary_io import FileBitWriter
from binary_io import FileByteReader


class _Node:
    """
    Represents a node in a graph
    """
    def __init__(self, symbol: int = None, frequency: int = None, left: _Node = None, right: _Node = None) -> None:
        self.frequency = frequency
        self.symbol = symbol
        self.left = left
        self.right = right


    def __lt__(self, other):
        return self.frequency < other.frequency





class HuffmanCode:
    """
    This class holds all the logic to encode vectors into a file using huffman code and to decode a file that is coded
    with huffman into vectors.

    The coding format is:
    Order of code : <num of symbols> <symbols dict: <symbol_1,len>, <symbol_2,len>,...<symbol_k,len>>   <vec_1> <vec_2> etc....

                    <num of symbols> -> Coded using 2 bytes (we assume that there is maximum 257 symbols)
                    <symbols dict> -> each <symbol,len> is coded:
                                    <symbol> -> coded using 2 bytes
                                                All regular symbols in the huffman code are defined to be int in the range 0 - 255,
                                                In addition, we have 2 more symbols:
                                                    1. VEC_SPACE_SYMBOL = 256 that notes a seperator between vectors.
                                                    2. EOF_SYMBOL = 257 that notes that we reached the end of the file.
                                    <len> -> Coded using 2 bytes (we assume that there are at most 257 symbols
                                             therefor the max depth of the tree is 257 that can be represende with 2 bytes)
                    <vec_i> -> For each 'literal' in the vec_i there are 3 cases:
                                       a. 'literal' is smaller than 256 -> encode the bits '0' + 'huffman code[literal]'
                                       b. 'literal' is bigger than 255 -> represent 'literal' is base 256
                                           (we assume that there are no literals bigger than 265^2 -1) so we will get
                                           'literal' = ab, such that a and b are smaller than 256.
                                           Then we will encode '1' + 'huffman code[a]' + 'huffman code[b]'
                                       c. We reach the end of the vector so we encode '0' + 'huffman code[256]'

    Attributes:
         vec_list(list[str]) - vec list to code with huffman code.
         huffman_code_file_name(str) - name of a file with huffman coded vectrs according to the above format.
    """
    def __init__(self, vec_list: list[list[int]] = None, huffman_code_file_name: str = None) -> None:
        self._check_invalid_input(vec_list, huffman_code_file_name)

        if huffman_code_file_name is None:
            self.vec_list = vec_list

        else:
            self.huffman_code_file_name = huffman_code_file_name

        self.VEC_SPACE_SYMBOL = 256
        self.EOF_SYMBOL = 257

    def get_vector_list(self) -> list[list[int]]:
        """
        Returns the vector list to be encoded with Huffman code
        :return: a list of vectors of integers
        """
        return self.vec_list



    def decode(self) -> None:
        """
        Decode binary file (that is correctly encoded with huffman code) into vectors
        :return: None
        """

        # Check if the file exists
        if self.huffman_code_file_name is None:
            print("No file to decode, please input a file name.")
            return

        WINDOW_SIZE = 1 << 15 # 32KB
        file = FileByteReader(WINDOW_SIZE, self.huffman_code_file_name)
        byte_buffer = file.get_current()
        cur_bit_location_in_buffer = 0

        # create huffman code dict and update the cur_bit_location_in_buffer
        huffman_dict, num_bytes_read = HuffmanCode._get_huffman_dict_from_file(byte_buffer)
        max_len_huffman_code = max(len(code) for code in huffman_dict.keys())
        cur_bit_location_in_buffer += (num_bytes_read * 8)

        bit_buffer = bitarray(byte_buffer)
        len_buffer = len(bit_buffer)

        vec_list = []
        cur_literal = 0
        while(cur_literal != self.EOF_SYMBOL):
            vec =[]
            cur_literal = 0
            while(cur_literal != self.VEC_SPACE_SYMBOL and cur_literal != self.EOF_SYMBOL):

                # Check if there is enough in the buffer to read next literal if not we will update
                remaining_bits_in_buffer = len_buffer - cur_bit_location_in_buffer
                if remaining_bits_in_buffer < (max_len_huffman_code * 2) + 1:  # worst case is: '1'+code1+code2 s.t len(code1)=len(code2)=max_len_huffman_code
                    num_bytes_to_end_of_byte_buffer = math.ceil(remaining_bits_in_buffer / 8)

                    # Calculate how many bytes we read from buffer
                    num_consumed_bytes = len(byte_buffer) - num_bytes_to_end_of_byte_buffer

                    # Slide window buffer, erasing the consumed bytes and loading more from the file
                    file.slide_window(num_consumed_bytes)

                    # update buffers after window shift
                    byte_buffer = file.get_current()
                    bit_buffer = bitarray(byte_buffer)
                    cur_bit_location_in_buffer = (num_bytes_to_end_of_byte_buffer * 8) - remaining_bits_in_buffer
                    len_buffer = len(bit_buffer)

                if bit_buffer[cur_bit_location_in_buffer] == 0:
                    cur_bit_location_in_buffer += 1

                    code = HuffmanCode._get_next_code(bit_buffer, cur_bit_location_in_buffer, huffman_dict, max_len_huffman_code)
                    cur_bit_location_in_buffer += len(code)

                    literal = huffman_dict[code]
                    if literal != self.EOF_SYMBOL and literal != self.VEC_SPACE_SYMBOL:
                        vec.append(huffman_dict[code])

                    cur_literal = huffman_dict[code]
                else:
                    cur_bit_location_in_buffer += 1

                    code1 = HuffmanCode._get_next_code(bit_buffer, cur_bit_location_in_buffer, huffman_dict, max_len_huffman_code)
                    cur_bit_location_in_buffer += len(code1)
                    code2 = HuffmanCode._get_next_code(bit_buffer, cur_bit_location_in_buffer, huffman_dict, max_len_huffman_code)
                    cur_bit_location_in_buffer += len(code2)

                    literal = HuffmanCode._turn_from_base_256_to_decimal(huffman_dict[code1], huffman_dict[code2])
                    vec.append(literal)

            vec_list.append(vec)

        self.vec_list = vec_list

    def encode(self, name_file_to_code_to: str) -> None:
        """
        Encode the input vectors into file using binary code according to huffman tree protocol.
        :param name_file_to_code_to:
        :return: None
        """

        # Check if the vec list exists
        if self.vec_list is None:
            print("No vectors to incode, please insert vectors")
            return

        # create huffman tree from vecs literals
        huffman_tree_root: _Node = self.create_huffman_tree_from_vecs()

        # Create the huffman code dict using the huffman tree lengths
        dict_symbol_to_len: dict[int, int] ={}
        self._create_len_list_of_leaves(dict_symbol_to_len, huffman_tree_root, 0)
        huffman_code_dict: dict[int, str] = HuffmanCode._create_Canonical_huffman_code(dict_symbol_to_len)

        # create a file to write to
        file: FileBitWriter = FileBitWriter(name_file_to_code_to)

        # Add dict to file (look above to understand coding)
        HuffmanCode._add_hufman_dict_to_file(file, dict_symbol_to_len)

        # Add vecs to file (look above to understand coding)
        self._add_vec_to_file(file, huffman_code_dict)

        # Add end of file symbol code
        file.write_bits('0' + huffman_code_dict[self.EOF_SYMBOL])
        
        file.close()


    def set_vec_list(self, vec_list: list[list[int]]):
        self.vec_list = vec_list

    def set_coded_huffman_file_name(self, file_name: str) -> None:
        self.huffman_code_file_name = file_name

    def get_vec_list(self) -> list[list[int]]:
        return self.vec_list

    def get_coded_huffman_file_name(self) -> str:
        return self.huffman_code_file_name


    ############################
    ###### Helper methods ######
    ############################
    def create_huffman_tree_from_vecs(self) -> _Node:
        """
        Creates a huffman tree of the vectors given to the Hoffman obj
        The tree will contain at most 257 symbols (each symbol is its ASCII value)
        If a symbol ASCII val is above 255 then we will consider it as 2 symbols of the digits in base 256.
        We assume there is no symbol that is more than 256^2.
        :return: the root of the huffman tree
        """

        # Get a dict from each symbol to frequency of appearance in the vectors:  symbol->frequency
        freq_dict: Counter[int] = Counter(itertools.chain.from_iterable(self.vec_list))

        # We remove every symbol that has ASCII val above 255 and replace it with the symbols
        # of the digits of it representaion in base 256
        for symbol in list(freq_dict.keys()):
            if symbol >= 256:
                a, b = HuffmanCode._transform_256_base(symbol)  # ab equals to symbol in 256 base
                freq_dict[a] += freq_dict[symbol]
                freq_dict[b] += freq_dict[symbol]
                del freq_dict[symbol]

        # Create the heap that holds the nodes
        min_heap_nodes = [_Node(symbol=symbol, frequency=freq) for symbol, freq in freq_dict.items()]
        num_of_vec_separators = len(self.vec_list) - 1 # Add the vec separator_symbol that is the ASCII val of 256
        min_heap_nodes.append(_Node(self.VEC_SPACE_SYMBOL, num_of_vec_separators))
        min_heap_nodes.append(_Node(self.EOF_SYMBOL, 1))
        heapq.heapify(min_heap_nodes)

        # Create Huffman Tree Using the huffman build algorithm
        while len(min_heap_nodes) > 1:
            node1 = heapq.heappop(min_heap_nodes)
            node2 = heapq.heappop(min_heap_nodes)
            merge_node = _Node(symbol=-1, frequency=node1.frequency + node2.frequency, left=node1, right=node2)
            heapq.heappush(min_heap_nodes, merge_node)
        return min_heap_nodes[0]

    def _add_vec_to_file(self, file: FileBitWriter, huffman_code_dict: dict[int, str]) -> None:
        """
        Codes the vectors to the file using huffman code format.
        The format is: We code the vectors symbol by symbol in the fallwing method:
                       a. if symbol can be represended in base 256 with one digit (that is it holds only 1 byte in memory in ASCII format)
                          then we code 0 and  then the hufmman code of that symbol.
                       b. if symbol can be represended in base 256 with two digits (that is we need 2 bytes in memory in ASCII format)
                          then we code 1, then we code the first(msb) digit of that symbol in base 256, and then we code the second digit(lsb)
                       c. if we reached the end of the vector we encode it using the special hufmman code for end of vec that is 'self.VEC_SPACE_SYMBOL'
        :param file: the file to code into.
        :param huffman_code_dict: The dict that hold the huffman code symbol(int)->huffman code(str)
        """
        num_of_vec = len(self.vec_list)
        for i, vec in enumerate(self.vec_list):
            for symbol in vec:
                # Code symbol digit by digit in base 256
                if symbol >= 256:
                    a, b = HuffmanCode._transform_256_base(symbol)
                    huffman_code_a = huffman_code_dict[a]  # Most significant digit
                    huffman_code_b = huffman_code_dict[b]  # Least significant digit
                    file.write_bits('1' + huffman_code_a + huffman_code_b)

                # code symbol that is in our huffman_code_dict
                else:
                    huffman_code = huffman_code_dict[symbol]
                    file.write_bits('0' + huffman_code)
            if i < num_of_vec - 1:
                file.write_bits('0' + huffman_code_dict[self.VEC_SPACE_SYMBOL])

    @staticmethod
    def _transform_256_base(num: int) -> tuple[int,int]:
        """
        Transforms a decimal number into two digit representation in base 256.
        :param num: The number to be transformed
        :return: two digit representation of 'num' in base 256
        :Notes: Example: 256 -> a=1 b=0
                The method assumes that in base 256 'num' has at most 2 digits.
        """
        b = num % 256
        a = num // 256
        return a,b

    @staticmethod
    def _create_Canonical_huffman_code(len_dict: dict[int,int]) -> dict[int,str]:
        """
        Creates the canonical Huffman code to a dictionary with 'symbols' and 'lengths' of their location in a huffman tree
        :param len_dict: The dictionary from the symbols to lengths that is symbol, value:length
        :return: The canonical Huffman code dictionary 'key = symbol(int)' and 'value = huffman code(str)'
        :notes: If len_dict describe a valid huffman tree symbols and depths (each symbol is a node)
                it is garenteed that the algo we provided will give a valid huffman code for the symbols in 'len_dict'
        """
        huffman_code_dict: dict[int, str] = {}
        huffman_code = 0

        # Sort by len then by Symbol
        sorted_symbols: list[tuple[int, int]] = sorted(len_dict.items(), key=lambda item: (item[1], item[0]))

        pre_len = 0
        for symbol, cur_len in sorted_symbols:
            # Makes sure we reached a new length symbol
            if pre_len < cur_len:
                huffman_code = huffman_code << (cur_len - pre_len) # We shift by the diff in depth
                pre_len = cur_len
            huffman_code_dict[symbol] = f"{huffman_code:0{cur_len}b}"
            huffman_code += 1

        return huffman_code_dict

    @staticmethod
    def _add_hufman_dict_to_file(file: FileBitWriter, dict_symbol_to_len: dict[int,int]) -> None:
        """
        Writes the huffman dict to file in binary format.
        The format intails the coding by this order: 1. two bytes-> num of symbols encoded
                                                     2. for each symbol and length of code we encode:
                                                            a. two bytes-> symbol ASCII value
                                                            b. one byte-> len of huffman code
        :param file: the file to write into
        :param len_dict: the dict that hold the symbols and lenghts
        """
        # Add number of symbol Takes 2 Bytes of memory
        file.write_bits(f"{len(dict_symbol_to_len):016b}")

        # Add Symbol and length each taking 2 Bytes in memory (16 bits)
        for symbol, cur_len in dict_symbol_to_len.items():
            symbol_val = symbol
            file.write_bits(f"{symbol_val:016b}")
            file.write_bits(f"{cur_len:016b}")

    @staticmethod
    def _create_len_list_of_leaves(len_dict: dict[int,int], node: _Node, depth: int) -> None:
        """
        Dfs implementation to get the dpth of the leaves of the huffman tree
        :param len_dict: a mutable dict to be muted for the caller it is key:symbol, value:length
        :param node: the node to traverse from down the huffman tree
        :param depth: the current depth of the tree
        """
        if node is None:
            return

        if node.left is None and node.right is None:
            len_dict[node.symbol] = depth
            return

        HuffmanCode._create_len_list_of_leaves(len_dict, node.right, depth + 1)
        HuffmanCode._create_len_list_of_leaves(len_dict, node.left, depth + 1)
        return

    @staticmethod
    def _get_huffman_dict_from_file(buffer:bytearray) -> tuple[dict[int:int],int]:
        """
        Read from file the symbols and lengths of code to reconstruct the huffman code.
        :param buffer: the bytearray to read the dictionary from.
        :return: the dictionary from symbol to code, and the number of bytes read from the file.
        """
        # Get number of symbols encoded
        num_of_symbols = (buffer[0] << 8) | buffer[1] # bit operation to calculate the symbol out of 2 bytes
        cur_index = 2

        # Get the symbol and lenghts
        symbol_to_len_dict = {}
        for i in range(num_of_symbols):
            symbol = (buffer[cur_index] << 8) | buffer[cur_index + 1] # bit operation to calculate the symbol out of 2 bytes
            cur_index += 2
            length = (buffer[cur_index] << 8) | buffer[cur_index + 1] # bit operation to calculate the symbol out of 2 bytes
            cur_index += 2
            symbol_to_len_dict[symbol] = length

        # create the huffman code dict
        huffman_dict = HuffmanCode._create_Canonical_huffman_code(symbol_to_len_dict) # dict symbol(int)->code(str)

        # reverse for convenience
        reverse_dict = {value:key for key, value in huffman_dict.items()} # dict code(str) -> symbol(int)

        num_bytes_read = 2 + (num_of_symbols * 4) # 2 from num of symbols and 4 for each symbol and length coding
        return reverse_dict, num_bytes_read

    @staticmethod
    def _get_next_code(buffer: bitarray, cur_bit_location_in_buffer: int, huffman_dict: dict[int:int], max_len_huffman_code: int) -> str:
        """
        Find the next codded symbol in the file
        :param buffer: the buffer of the file in bitarray form
        :param cur_bit_location_in_buffer: The start location in the buffer to read from.
        :param huffman_dict: The dict that maps from huffman code to symbol
        :return: the huffman code
        """
        cur_code =''

        # Read code from file till we find a ,atch in the dict
        while(True):
            cur_bit = str(buffer[cur_bit_location_in_buffer])
            cur_code += cur_bit
            if cur_code in huffman_dict:
                return cur_code
            cur_bit_location_in_buffer += 1

            if len(cur_bit) > max_len_huffman_code:
                raise ValueError('ERROR: The huffman coded file is not in format please input a valid codded file')

    @staticmethod
    def _turn_from_base_256_to_decimal(symbol1: int, symbol2: int) -> int:
        """
        Returns the base 256 value of the number symbol1symbol2.
        :param symbol1:
        :param symbol2:
        :return: int
        """
        return  (symbol1 * 256) + symbol2

    #################################
    ### Validity varifiers methods###
    #################################
    def _invalid_input_vecs(self, vec_list):
        if vec_list is None:
            return True

        # check that the lists are lists of int
        for vec in vec_list:
            is_all_ints = all(isinstance(x, int) for x in vec)
            if not is_all_ints:
                return True

        # Test matched lengths
        all_same_len = len(set(len(sub) for sub in vec_list)) <= 1
        if not all_same_len:
            return True
        return False

    def _invalid_input_file_name(self, huffman_code_file_name):
        try:
            with open(huffman_code_file_name, 'rb') as f:
                pass
            return False
        except (FileNotFoundError, IOError):
            return True

    def _check_invalid_input(self,vec_list, huffman_code_file_name):
        if vec_list is None and huffman_code_file_name is None:
            raise ValueError("ERROR: Must initialize HuffmanTree with vec_list of integers or a valid incoded huffman code in binary file")

        # Bad Vec input
        if vec_list is not None and self._invalid_input_vecs(vec_list):
            raise ValueError("ERROR: Must input valid integers vectors to code.")

        # Bad huffman_code_file_name input
        if huffman_code_file_name is not None and self._invalid_input_file_name(huffman_code_file_name):
            raise ValueError("ERROR: Cannot open file")