#ifndef SPONGE_LIBSPONGE_BYTE_STREAM_HH
#define SPONGE_LIBSPONGE_BYTE_STREAM_HH

#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <string>
#include <utility>

class ByteStream {
  private:
    std::deque<char> _buffer = {}; // 使用deque<char>_buffer来作为缓冲区也就是字节流
    size_t _capacity = 0;
    size_t _read_count = 0;
    size_t _write_count = 0;
    bool _input_ended_flag = false;
    bool _error = false;

  public:
    // Construct a stream with room for `capacity` bytes.
    ByteStream(const size_t capacity);

   
    // Write a string of bytes into the stream. Write as many
    // as will fit, and return how many were written.
    //! \returns the number of bytes accepted into the stream
    size_t write(const std::string &data);

    //! \returns the number of additional bytes that the stream has space for
    size_t remaining_capacity() const;

    // Signal that the byte stream has reached its ending
    void end_input();

    // Indicate that the stream suffered an error.
    void set_error() { _error = true; }

    //! \returns a string
    std::string peek_output(const size_t len) const;

    // Remove bytes from the buffer
    void pop_output(const size_t len);

    //! \returns a vector of bytes read
    std::string read(const size_t len) {
        const auto ret = peek_output(len);
        pop_output(len);
        return ret;
    }

    //! \returns `true` if the stream input has ended
    bool input_ended() const;

    //! \returns `true` if the stream has suffered an error
    bool error() const { return _error; }

    //! \returns the maximum amount that can currently be read from the stream
    size_t buffer_size() const;

    //! \returns `true` if the buffer is empty
    bool buffer_empty() const;

    //! \returns `true` if the output has reached the ending
    bool eof() const; 

    // Total number of bytes written
    size_t bytes_written() const;

    // Total number of bytes popped
    size_t bytes_read() const;
};

#endif  // SPONGE_LIBSPONGE_BYTE_STREAM_HH
