#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template<typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()})),
      _initial_retransmission_timeout{retx_timeout},
      _time_out(retx_timeout), _rwnd(1), _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const {
  return _next_seqno - _last_acked;
}

void TCPSender::fill_window() {
  // fill receive window full,
  // SYN and FIN take place of window
  string data;
  uint64_t remain_wnd;
  uint16_t rwnd = max<uint16_t>(_rwnd, 1);
  while ((remain_wnd = _last_acked + rwnd - _next_seqno) > 0) {
    TCPSegment seg;
    setFlag(seg);   // take place for SYN
    size_t sz = min(remain_wnd, TCPConfig::MAX_PAYLOAD_SIZE) - seg.length_in_sequence_space();
    data = _stream.read(sz);
    seg.header().seqno = wrap(_next_seqno, _isn);
    if (remain_wnd > data.size())   // detect space for FIN
      setFlag(seg);
    seg.payload() = Buffer(std::move(data));
    if (seg.length_in_sequence_space() == 0)
      break;
    _retrans_buf[_next_seqno] = seg;
    _next_seqno += seg.length_in_sequence_space();
    _segments_out.push(std::move(seg));
  }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
  uint64_t absolute_ack = unwrap(ackno, _isn, _last_acked);
  if (absolute_ack > _next_seqno) {   // ack invalid, bigger than _next_seqno
    return;
  } else if (_last_acked < absolute_ack) {   // ack move ahead
    _last_acked = absolute_ack;
    auto lb = _retrans_buf.lower_bound(absolute_ack);
    if (lb->first == absolute_ack || (lb != _retrans_buf.begin()
        && prev(lb)->first + prev(lb)->second.length_in_sequence_space() == absolute_ack)) {
      _retrans_buf.erase(_retrans_buf.begin(), lb);
    } else {
      _retrans_buf.erase(_retrans_buf.begin(), prev(lb));
    }
    _rwnd = window_size;  // regard windows size as 1 if it is 0
    _timer.reset();
    _time_out = _initial_retransmission_timeout;
    _retrans_times = 0;
  } else if (_last_acked == absolute_ack) {  // dup ack
    _rwnd = max(_rwnd, window_size);
    // if ((++_dup_ack) == 3) {
    //   // TODO: fast retransmit
    //   retransmit(absolute_ack);
    //   // NOTE: DON'T do fast retransmit here !!!
    //   _dup_ack = 0;
    // }
  } else {  // ack acked
    return;
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
  _timer.add(ms_since_last_tick);
  if (_timer.expired(_time_out)) {  // expired and retransmission logic
    retransmit(_last_acked);
    if (_rwnd != 0)   // DON'T double timeout when receive window equal to zero
      _time_out *= 2;
    ++_retrans_times;
    _timer.reset();
  }
}

unsigned int TCPSender::consecutive_retransmissions() const {
  return _retrans_times;
}

void TCPSender::send_empty_segment() {
  TCPSegment seg;
  seg.header().seqno = wrap(_next_seqno, _isn);
  setFlag(seg);
  _segments_out.push(seg);
}

void TCPSender::retransmit(uint64_t seq) {
  if (!_retrans_buf.empty())
    _segments_out.push(prev(_retrans_buf.upper_bound(seq))->second);
}

void TCPSender::setFlag(TCPSegment &seg) {
  if (_stat == CLOSED) {
    seg.header().syn = true;
    _stat = SYNC;
  } else if (_stat == SYNC) {
    if (_stream.eof()) {
      seg.header().fin = true;
      _stat = FIN;
    }
  } else if (_stat == FIN) {
    // do nothing
  }
}
