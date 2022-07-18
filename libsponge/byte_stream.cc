#include "byte_stream.hh"

#include <algorithm>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template<typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _error(false), _end(false), _p_head(0),
      _p_tail(0), _buf_size(0), _cap(capacity),
      _all_read(0), _all_write(0), _buffer(capacity, 0) {
}

size_t ByteStream::write(const string &data) {
  if (_end)
    return 0;
  size_t sz = min(data.size(), remaining_capacity());
  for_each(data.begin(), data.begin() + sz, [this](char ch) {
    _buffer[_p_tail] = ch;
    _p_tail = (_p_tail + 1) % _cap;
  });
  _buf_size += sz;
  _all_write += sz;
  return sz;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
  size_t peek_len = min(len, _buf_size);
  string ret;
  ret.reserve(peek_len);
  if (_p_head <= _cap - _buf_size) {
    ret.append(_buffer, _p_head, peek_len);
  } else {
    ret.append(_buffer, _p_head, min(_cap - _p_head, peek_len));
    ret.append(_buffer, 0, (peek_len > _cap - _p_head ? peek_len - (_cap - _p_head) : 0));
  }
  return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
  size_t pop_len = min(len, _buf_size);
  _p_head = (_p_head + pop_len) % _cap;
  _buf_size -= pop_len;
  _all_read += pop_len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
  size_t read_len = min(len, _buf_size);
  string ret;
  ret.reserve(read_len);
  if (_p_head <= _cap - _buf_size) {
    ret.append(_buffer, _p_head, read_len);
  } else {
    ret.append(_buffer, _p_head, min(_cap - _p_head, read_len));
    ret.append(_buffer, 0, (read_len > _cap - _p_head ? read_len - (_cap - _p_head) : 0));
  }
  _p_head = (_p_head + read_len) % _cap;
  _buf_size -= read_len;
  _all_read += read_len;
  return ret;
}

void ByteStream::end_input() {
  _end = true;
}

bool ByteStream::input_ended() const {
  return _end;
}

size_t ByteStream::buffer_size() const {
  return _buf_size;
}

bool ByteStream::buffer_empty() const {
  return _buf_size == 0;
}

bool ByteStream::eof() const {
  return _buf_size == 0 && _end;
}

size_t ByteStream::bytes_written() const {
  return _all_write;
}

size_t ByteStream::bytes_read() const {
  return _all_read;
}

size_t ByteStream::remaining_capacity() const {
  return _cap - _buf_size;
}
