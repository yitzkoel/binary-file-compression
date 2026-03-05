class FileByteReader:
    """
    This class is used to read a binary data from a file.
    It holds a 'past' and 'current' buffers.
    The 'current' buffer hold the actual place we are in the file.
    The 'past' buffer hold the past bytes of the file up to a window size that can be set in the initialization of the object.
    It is designed to work well for implementing the Lempel ziv 77 algorithm.
    In order to progress the buffer use the 'slide_window' method with the amount of bytes you want to progress.
    """
    current: bytearray
    past: bytearray

    def __init__(self, window_size: int, file_name: str) -> None:
        self.window_size = window_size # Size of buffer
        self.past = bytearray() # will always be the past window_size bytes maximum
        self.current = bytearray() # will always be the next window_size bytes maximum

        # Open file
        try:
            self.file = open(file_name, 'rb')
        except FileNotFoundError:
            print("Failed to open file")
            raise

        # Read first window to lookahead buffer
        self.current.extend(self.file.read(window_size))

    def get_past(self) -> bytearray:
        """
        Gets past Buffer
        :return:
        """
        return self.past

    # Gets future Buffer
    def get_current(self):
        return self.current

    # Slides the buffers window_len, keeping the past and current buffers at max window size.
    # Returns False for end of file
    # Returns True for normal behavior
    def slide_window(self, window_len: int) -> bool:
        """
        Slides the buffers window_len, keeping the past and current buffers at max window size.
        Returns False for end of file
        Returns True for normal behavior (success at sliding the window)
        :param window_len: the number of bytes to load the buffers from the file.
        :return: A boolean indicating the success of the sliding window or the failure (end of file meaning there is no
         more bytes to add to buffer).
        """
        # trying to slide the window on the file that has reached end of file
        if not self.current:
            return False

        # Read new data from file
        new_bytes = self.file.read(window_len)

        # slide past buffer
        self.past.extend(self.current[:window_len])
        # check if we already have past of len window_len
        if (len(self.past) > self.window_size):
            excess = len(self.past) - self.window_size
            del self.past[:excess]

        # slide current buffer
        self.current.extend(new_bytes)
        del self.current[:window_len]

        # We got to end of file after slide
        if not self.current:
            return False

        # We did not reach end of file after slide
        return True


    def close(self):
        self.file.close()



class FileBitWriter:
    """
    This class is responsible for writing into a file binary data bit by bit.
    It is implemented using a bit buffer(a int that gets to 8 in binary) that holds the bit data and is flushed into a
    byte buffer, which is flushed into the file in chunks (4 KB per chunk) in order to reduce the write file time.

    When you finish writing you must use the 'close' method to close the file and to flush all the buffers and close the
    file.
    """
    def __init__(self,file_name):
        try:
            self.file = open(file_name, 'wb')
        except FileNotFoundError:
            raise FileNotFoundError("Can't open file")

        self.BYTE_SIZE = 8
        self.bit_buffer = 0
        self.count = 0 #counts how many bits went into buffer so far
        self.byte_array_buffer = bytearray()
        self.CHUNK_SIZE = 4096 # 4KB

    def write_bits(self, bits_string):
        for bit in bits_string:
            self.bit_buffer = (self.bit_buffer << 1) + int(bit)
            self.count += 1

            if self.count == self.BYTE_SIZE:
                self.byte_array_buffer.append(self.bit_buffer)
                self.bit_buffer = 0
                self.count = 0

                if len(self.byte_array_buffer) == self.CHUNK_SIZE:
                    self.file.write(self.byte_array_buffer)
                    self.byte_array_buffer.clear()


    def close(self):
        """
        close the
        """
        if self.count > 0:
            buffer = self.bit_buffer << (self.BYTE_SIZE - self.count)

            self.byte_array_buffer.append(buffer)
            self.file.write(self.byte_array_buffer)

        self.file.close()