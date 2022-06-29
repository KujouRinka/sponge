#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template<typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _unassembled_bytes(0),
      _first_unassembled(0),
      _unassembled_data() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
  if (_output.eof())
    return;
  if (index < _first_unassembled)
    return;
  // discard for exceeding capacity
  if (index + data.size() > _first_unassembled + unassembled_cap())
    return;
  if (_unassembled_data.count(index))
    return;
  if (index != _first_unassembled) {
    _unassembled_data.insert({index, data});
    _unassembled_bytes += data.size();
    return;
  }
  _output.write(data);
  _first_unassembled += data.size();
  size_t next_slice_idx = index + data.size();
  while (_unassembled_data.count(next_slice_idx)) {
    auto bs = _unassembled_data[next_slice_idx].size();
    _first_unassembled += bs;
    // _first_unacceptable += bs;
    _output.write(_unassembled_data[next_slice_idx]);
    _unassembled_data.erase(next_slice_idx);
    next_slice_idx += bs;
  }
  if (eof)
    _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const {
  return _unassembled_bytes;
}

bool StreamReassembler::empty() const {
  return _unassembled_data.empty();
}

size_t StreamReassembler::unassembled_cap() const {
  return _capacity - _output.buffer_size();
}
