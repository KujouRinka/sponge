#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template<typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _cap(capacity) {}

size_t ByteStream::write(const string &data) {
  size_t write_len = min(data.length(), _cap - _buffer.size());
  string s;
  s.assign(data.begin(), data.begin() + write_len);
  _buffer.append(std::move(s));
  _all_write += write_len;
  return write_len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
  size_t peek_len = min(len, _buffer.size());
  string ret;
  ret.reserve(peek_len);
  for (size_t idx = 0; peek_len != 0; ++idx) {
    auto &buf = _buffer.buffers()[idx];
    size_t sz = min(peek_len, buf.size());
    ret.append(buf.str().begin(), buf.str().begin() + sz);
    peek_len -= sz;
  }
  return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
  size_t pop_len = min(len, _buffer.size());
  _all_read += pop_len;
  _buffer.remove_prefix(pop_len);
}

string ByteStream::read(const size_t len) {
  string ret = peek_output(len);
  pop_output(len);
  return ret;
}

void ByteStream::end_input() {
  _end = true;
}

bool ByteStream::input_ended() const {
  return _end;
}

size_t ByteStream::buffer_size() const {
  return _buffer.size();
}

bool ByteStream::buffer_empty() const {
  return _buffer.size() == 0;
}

bool ByteStream::eof() const {
  return buffer_empty() && _end;
}

size_t ByteStream::bytes_written() const {
  return _all_write;
}

size_t ByteStream::bytes_read() const {
  return _all_read;
}

size_t ByteStream::remaining_capacity() const {
  return _cap - _buffer.size();
}