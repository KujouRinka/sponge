#include "stream_reassembler.hh"

#include <algorithm>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template<typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _unassembled_bytes(0),
      _first_unassembled(0), _eof(false), _unassembled_data(capacity),
      _unassembled_valid(capacity, false), _unassembled_start(0) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
  if (_output.eof())
    return;
  size_t first_unacceptable = _first_unassembled + unassembled_cap();
  size_t trunc_front = index < _first_unassembled ? _first_unassembled - index : 0;
  size_t trunc_back =
      index + data.size() > first_unacceptable ?
      index + data.size() - first_unacceptable : 0;
  if (trunc_front + trunc_back > data.size())
    return;
  size_t buf_to_start = _unassembled_start +
      (index < _first_unassembled ? 0 : index - _first_unassembled);
  for_each(data.begin() + trunc_front, data.end() - trunc_back, [this, &buf_to_start](char ch) {
    if (!_unassembled_valid[buf_to_start]) {
      _unassembled_data[buf_to_start] = ch;
      _unassembled_valid[buf_to_start] = true;
      ++_unassembled_bytes;
    }
    buf_to_start = (buf_to_start + 1) % _capacity;
  });
  buf_to_start = _unassembled_start;
  string write_data;
  write_data.reserve(_capacity);
  while (_unassembled_bytes && _unassembled_valid[buf_to_start]) {
    write_data.push_back(_unassembled_data[buf_to_start]);
    _unassembled_valid[buf_to_start] = false;
    --_unassembled_bytes;
    _unassembled_start = (_unassembled_start + 1) % _capacity;
    buf_to_start = (buf_to_start + 1) % _capacity;
  }
  _first_unassembled += write_data.size();
  _output.write(write_data);
  if (eof && trunc_back == 0)
    _eof = true;
  if (_eof && _unassembled_bytes == 0)
    _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const {
  return _unassembled_bytes;
}

bool StreamReassembler::empty() const {
  return _unassembled_bytes == 0;
}

size_t StreamReassembler::unassembled_cap() const {
  return _capacity - _output.buffer_size();
}
