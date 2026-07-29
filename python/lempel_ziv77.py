from binary_io import FileByteReader

class LempelZiv77:
    """
    This class implements the LempelZiv77 binary file compression algorithm.
    Each byte is a symbol in the compression (So we have 256 symbols)
    The algo is:
    1. start with i = 0 pointing to the first byte in the file.
    2. while i != len(file_bytes) do:
    3. find the largest len window in past (index j) that is the same as the window that starts in index i.
    4. save the triplet (j,len,file_bytes[i+len+1]).
    5. when we reach the end of file crerate 3 vecs from each index in the tuple (j,len,i+len+1).
    """
    buffer: FileByteReader

    def __init__(self, window_size: int, file_to_compress_name: str = None, window_vec: list[int] = None, literal_vec: list[int] = None,
                 past_start_index_vec: list[int] = None) -> None:
        """
        Initalize a Lempel viv obgect that can get a file and compress to vectors or it can get 3 vectors and decompress
        to a binary file.
        :param window_size: Size of the window we will seach for in the past for a match.
        :param file_to_compress_name: Name of the file to compress.
        :param window_vec: Vector that holds the len of the window in past (called len above)
        :param literal_vec: The file_bytes[i+len+1] parameters.
        :param past_start_index_vec: The j parameter of the start of the window in the past.
        """
        error_msg = "Wrong input parameters, must input window_size and file_to_compress_name to compress, or window_vec, input literal_vec, past_start_index_vec to decompress"
        self.window_size = window_size

        # Check for wrong input parameters
        if(window_vec is None and literal_vec is None and past_start_index_vec is None):
            if(file_to_compress_name is None):
                raise ValueError("Did not get a file to compress")

            self.buffer = FileByteReader(window_size=window_size, file_name=file_to_compress_name)
            return

        # got parameters for compression
        if(window_vec is None or literal_vec is None or past_start_index_vec is None):
            print(error_msg)
            raise ValueError("Missing vectors for decompression")

        # Check for wrong input parameters all vec inputs need to be same length
        len_window = len(window_vec)
        len_literal = len(literal_vec)
        len_past = len(past_start_index_vec)
        if(len_window != len_past or len_past != len_literal ):
            raise ValueError("the input vectors were not the same length")

        # Correct parameters for decompression
        self.window_vec = window_vec
        self.literal_vec = literal_vec
        self.past_start_index_vec = past_start_index_vec


    def compress_file(self) -> None:
        """
        Compress the file into 3 vectors.
        :return: window_vec, literal_vec, past_start_index_vec that code the file.
        """
        window_vec: list[int] = []
        literal_vec: list[int] = []
        past_start_index_vec: list[int] = []

        not_end_of_file = True

        cur_location_in_past = 0 # keeps the index location in the file of the first elem in the past of buffer
                                 # For example if the file is "12345678" and buffer holds past="34", current="56"
                                 # so cur_location_in_past will be 2 becase past starts at index 2 of file.

        while(not_end_of_file):
            # Get max past and lookahead
            past = self.buffer.get_past()
            current = self.buffer.get_current()

            # search for biggest windows match from past
            window_length ,index_j_in_past = self._find_window(past, current)

            # Get literal after window
            literal = current[window_length]

            # Encode the window data to the vectors
            window_vec.append(window_length)
            literal_vec.append(literal)
            past_start_index_vec.append(index_j_in_past)

            # slide the window to search for next max window match
            not_end_of_file = self.buffer.slide_window(window_length + 1)
            cur_location_in_past += window_length + 1
        self.window_vec = window_vec
        self.literal_vec = literal_vec
        self.past_start_index_vec = past_start_index_vec

    def _find_window(self, past: bytearray, current: bytearray) -> tuple[int,int]:
        """
        Finds the next largest window in the past.
        :param past: the past to look in.
        :param current: the current to find a window in past for.
        :return: len of largest window and the index it starts in the past.
        :comment: The function uses binary seach algo to find the largest window in the past.
        """
        data = past + current
        window_view = memoryview(current)

        len_past = len(past)

        # If current is len 1 (or smaller) it cant be encoded and must be a literal
        if len(current) <= 1:
            return 0, 0

        # eliminate the trivial case of no match
        best_j = past.rfind(current[0])
        if(best_j == -1):
            return 0,0

        # There is at least a match on len 1 in past
        window_size = 1 # The known window size to exist
        start = 1 # The smallest possible value for window_size
        end = min(len(current) - 1, self.window_size) # The largest possible value for window_size
                                                      # The offset of len(current) - 1 by 1 is desighned so the
                                                      # window will not be the size of len(current), because if it is we
                                                      # have the problem that the literal in the LempelZiv code will
                                                      # be out of range and we will get a index out of range error.

        # Binary Search foe optimal window size
        while start <= end:
            mid = (start + end) // 2
            pattern = window_view[0:mid] # The data in the window
            new_j = data.rfind(pattern, 0, len_past + mid -1) # Comparing to the range [0:len_past + window_size -1]
                                                                      # by offset of -1 so we will not match the pattern
                                                                      #(the last (window_size - 1) elem in [0:len_past + window_size -1]
                                                                      # are the same last (window_size - 1) elem in pattern

            # If No match we need to search a smaller window between lengths 'start' and 'window_size'
            if(new_j == -1):
                end = mid - 1

            # Else there is a match and we need to search a bigger window between lengths 'window_size' and 'end'
            else:
                start = mid + 1
                best_j = new_j
                window_size = mid

        return window_size, len_past - best_j

    def get_vecs(self) -> list[list[int]]:
        return [self.window_vec, self.literal_vec, self.past_start_index_vec]

    def decompress_file(self, dest_file_name: str) -> None:
        try:
            with open(dest_file_name, 'wb') as write_f:
                buffer = bytearray()  # a buffer that will be maintaied to be of max len self.window_size * 4
                # that will hold data to be writin into the file
                location_in_write_f = 0  # index in the write file that is the index 0 in buffer

                for window, literal, j_in_past in zip(self.window_vec, self.literal_vec, self.past_start_index_vec):
                    len_buffer = len(buffer)

                    if (len_buffer - j_in_past < 0):
                        raise ("got bad vectors that cant be decompressed")
                    start = len_buffer - j_in_past  # calculate index in the buffer to start from
                    end = start + window

                    # Case 1: The window to copy is all in the buffer (in the past)
                    if (end <= len_buffer):
                        buffer.extend(buffer[start:end])

                    # Case 2: The window to copy extends to the future (it is not in the current buffer)
                    else:
                        # Calculate the pattern length in the buffer
                        # The num of times it appears
                        # The remainder of the beginning of the pattern
                        len_pattern = len_buffer - start
                        num_copies = window // len_pattern
                        remainder_length = window % len_pattern

                        pattern = buffer[start: start + len_pattern]

                        buffer.extend(pattern * num_copies)
                        buffer.extend(pattern[:remainder_length])

                    # Add literal
                    buffer.append(literal)

                    # Flush buffer to file
                    if (len(buffer) > self.window_size * 4):
                        len_to_transfer_to_write_f = len(buffer) - self.window_size * 2
                        write_f.write(buffer[:len_to_transfer_to_write_f])
                        del buffer[:len_to_transfer_to_write_f]

                        location_in_write_f += len_to_transfer_to_write_f

                # Flush rest of buffer to file
                if buffer:
                    write_f.write(buffer)
        except FileNotFoundError:
            print("did not succeed to write into file")


###################################################################
###### INEFFICIENT PYTHON IMPLEMENTAION, MAY BE FASTER IN C++######
###################################################################
    # def _find_window(self, past, current):
    #     cur_literal = current[0]
    #
    #     len_past = len(past)
    #     max_window_size = min(self.window_size, len(current) - 1) # We dont want IndexError when we access cur_window_size index
    #
    #     # Find all 1 len matches
    #     match_window_index_in_past = [i for  i, x in enumerate(past) if x == cur_literal]
    #
    #     # No match in past
    #     if not match_window_index_in_past:
    #         return 0, 0
    #
    #     cur_window_size = 1
    #
    #     # Iterate indexes in match_window_index_in_past and look 1 literal ahead
    #     # to find a bigger window match from the indexes in match_window_index_in_past
    #     while(len(match_window_index_in_past) > 0 and cur_window_size < max_window_size):
    #         cur_literal = current[cur_window_size] #The new literal to compare
    #
    #         new_match_lst = []
    #         for i in match_window_index_in_past:
    #             # If the literal in the window we want to compare to is in 'current' lst
    #             if i + cur_window_size >= len_past:
    #                 correct_index_in_curr = cur_window_size - (len_past - i)
    #                 if(current[correct_index_in_curr] == cur_literal):
    #                     new_match_lst.append(i)
    #
    #             # Else the literal in the window we want to compatre to is in 'past' lst
    #             else:
    #                 if past[i + cur_window_size] == cur_literal:
    #                     new_match_lst.append(i)
    #
    #         # Found no bigger match window of size bigger than cur_window_size - 1
    #         # Means the largest window match is in match_window_index_in_past
    #         if len(new_match_lst) == 0:
    #             break
    #         else:
    #             cur_window_size += 1  # Advance window size
    #             match_window_index_in_past = new_match_lst
    #
    #     # If we have at least one match in past
    #     index_j_in_past = random.choice(match_window_index_in_past)
    #
    #     return  cur_window_size, len_past - index_j_in_past