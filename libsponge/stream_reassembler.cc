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
    : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
  DataBlock new_data;
  if (_output.eof() || index > _first_unassembeld + unassembled_cap())  // first byte in first unacceptable
    return;
  if (index + data.size() <= _first_unassembeld) {  // last byte in byte_stream
    eof_logic(eof, false);
    return;
  }

  size_t front_trunc = _first_unassembeld > index ? _first_unassembeld - index : 0;
  size_t back_trunc = index + data.size() > _first_unassembeld + unassembled_cap() ?
                      index + data.size() - _first_unassembeld - unassembled_cap() : 0;
  new_data.data.assign(data.begin() + front_trunc, data.end() - back_trunc);
  new_data.start_idx = front_trunc ? _first_unassembeld : index;

  // merge logic
  while (true) {
    auto inserter_it = _unassembled_blocks.lower_bound(new_data);
    if (inserter_it != _unassembled_blocks.end() && inserter_it->start_idx == new_data.start_idx) {
      while (inserter_it != _unassembled_blocks.end() && merge_block_and_del(new_data, *inserter_it++)) {}
      break;
    } else { // inserter_it->start_idx > new_data.start_idx
      if (inserter_it != _unassembled_blocks.begin())
        merge_block_and_del(new_data, *prev(inserter_it));
      while (inserter_it != _unassembled_blocks.end() && merge_block_and_del(new_data, *inserter_it++)) {}
      break;
    }
  }
  _unassembled_bytes += new_data.data.size();
  _unassembled_blocks.insert(std::move(new_data));

  // ...
  if (_unassembled_bytes && _first_unassembeld == _unassembled_blocks.begin()->start_idx) {
    const DataBlock &block_to_write = *_unassembled_blocks.begin();
    size_t written_bytes = _output.write(block_to_write.data);
    _unassembled_bytes -= written_bytes;
    _first_unassembeld += written_bytes;
    _unassembled_blocks.erase(_unassembled_blocks.begin());
  }

  eof_logic(eof, back_trunc);
}

size_t StreamReassembler::unassembled_bytes() const {
  return _unassembled_bytes;
}

bool StreamReassembler::empty() const {
  return _unassembled_bytes == 0;
}

// actually it is the windows size
size_t StreamReassembler::unassembled_cap() const {
  return _capacity - _output.buffer_size();
}

// merge data from rhs to lhs, delete rhs in
// _unassembled_blocks and set _unassembled_bytes.
// return true if it really did merge job.
bool StreamReassembler::merge_block_and_del(DataBlock &lhs, const DataBlock &rhs) {
  DataBlock &front = lhs < rhs ? lhs : const_cast<DataBlock &>(rhs);
  DataBlock &back = lhs < rhs ? const_cast<DataBlock &>(rhs) : lhs;
  if (front.start_idx + front.size() < back.start_idx)
    return false;
  _unassembled_bytes -= rhs.size();
  DataBlock tmp;
  tmp.start_idx = front.start_idx;
  if (front.start_idx + front.size() >= back.start_idx + back.size()) {
    tmp.data = std::move(front.data);
  } else {
    size_t trunc = front.start_idx + front.size() - back.start_idx;
    tmp.data.reserve(lhs.size() + rhs.size() - trunc);
    tmp.data.append(front.data.begin(), front.data.end()).append(back.data.begin() + trunc, back.data.end());
  }
  _unassembled_blocks.erase(rhs);
  lhs.swap(tmp);
  return true;
}

void StreamReassembler::eof_logic(bool eof, bool trunc) {
  if (eof && !trunc)
    _eof = true;
  if (_eof && _unassembled_bytes == 0)
    _output.end_input();
}
