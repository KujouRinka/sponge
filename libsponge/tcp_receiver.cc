#include "tcp_receiver.hh"

#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template<typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

TCPReceiver::TCPReceiver(const size_t capacity)
    : _isn(nullopt), _reassembler(capacity) {
}

void TCPReceiver::segment_received(const TCPSegment &seg) {
  // handle SYN pack
  if (!_isn.has_value()) {
    if (seg.header().syn) {
      _isn.emplace(seg.header().seqno);
    } else {
      cerr << "TCPReceiver: bad syn" << endl;
      return;
    }
  }

  uint64_t idx = unwrap(
      seg.header().seqno + (seg.header().syn ? 1 : 0),
      _isn.value() + 1,
      _reassembler.stream_out().bytes_written());
  _reassembler.push_substring(seg.payload().copy(), idx, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
  optional<WrappingInt32> ackno(nullopt);
  if (_isn.has_value()) {
    ackno.emplace(wrap(_reassembler.stream_out().bytes_written(), _isn.value() + 1)
                      + (_reassembler.stream_out().input_ended() ? 1 : 0));
  }
  return ackno;
}

size_t TCPReceiver::window_size() const {
  return _reassembler.unassembled_cap();
}
