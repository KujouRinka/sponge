#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template<typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
  cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
  // convert IP address of next hop to raw 32-bit representation (used in ARP header)
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();
  EthernetFrame eth;
  eth.header().src = _ethernet_address;
  if (_arp_map.count(next_hop_ip) == 0) {   // prepare ARP frame
    if (_arp_req_time.count(next_hop_ip) && _arp_req_time[next_hop_ip] <= 5000)
      return;
    _cache_frame.push({dgram, next_hop_ip});
    eth.header().dst = ETHERNET_BROADCAST;
    eth.header().type = EthernetHeader::TYPE_ARP;
    ARPMessage arp_req;
    arp_req.opcode = ARPMessage::OPCODE_REQUEST;
    arp_req.sender_ethernet_address = _ethernet_address;
    arp_req.sender_ip_address = _ip_address.ipv4_numeric();
    arp_req.target_ethernet_address = ETHERNET_UNKNOWN;
    arp_req.target_ip_address = next_hop_ip;
    eth.payload() = arp_req.serialize();
    _arp_req_time[next_hop_ip] = 0;
  } else {  // prepare IPv4 frame
    eth.header().dst = _arp_map[next_hop_ip].first;
    eth.header().type = EthernetHeader::TYPE_IPv4;
    eth.payload() = dgram.serialize();
  }
  _frames_out.push(std::move(eth));
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
  optional<InternetDatagram> ret(nullopt);
  auto eth_type = frame.header().type;
  auto eth_dst = frame.header().dst;
  if (eth_type == EthernetHeader::TYPE_IPv4 && eth_dst == _ethernet_address) {   // IPv4 frame
    InternetDatagram dgram;
    if (dgram.parse(frame.payload()) == ParseResult::NoError)
      ret.emplace(std::move(dgram));
  } else if (eth_type == EthernetHeader::TYPE_ARP) {  // ARP frame
    ARPMessage arp;
    if (arp.parse(frame.payload()) != ParseResult::NoError)
      return ret;
    if (eth_dst == ETHERNET_BROADCAST) {  // ARP request received
      if (arp.opcode == ARPMessage::OPCODE_REQUEST
          && arp.target_ethernet_address == ETHERNET_UNKNOWN
          && arp.target_ip_address == _ip_address.ipv4_numeric()) {
        _arp_map[arp.sender_ip_address] = {arp.sender_ethernet_address, 30000};   // update ARP table
        ARPMessage arp_reply;
        arp_reply.opcode = ARPMessage::OPCODE_REPLY;
        arp_reply.sender_ethernet_address = _ethernet_address;
        arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
        arp_reply.target_ethernet_address = arp.sender_ethernet_address;
        arp_reply.target_ip_address = arp.sender_ip_address;
        EthernetFrame eth_reply;
        eth_reply.header().src = _ethernet_address;
        eth_reply.header().dst = arp.sender_ethernet_address;
        eth_reply.header().type = EthernetHeader::TYPE_ARP;
        eth_reply.payload() = arp_reply.serialize();
        _frames_out.push(std::move(eth_reply));
      }
    } else if (eth_dst == _ethernet_address) {  // ARP response received
      if (arp.opcode == ARPMessage::OPCODE_REPLY
          && arp.target_ethernet_address == _ethernet_address
          && arp.target_ip_address == _ip_address.ipv4_numeric()) {
        _arp_map[arp.sender_ip_address] = {arp.sender_ethernet_address, 30000};
        recheckOutQueue();
      }
    }
  } else {  // unsupported type
    // do nothing
  }
  return ret;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
  for (auto &kv : _arp_map) {
    if (kv.second.second <= ms_since_last_tick)   // do compare, for time has unsigned type
      _arp_map.erase(kv.first);
    else
      kv.second.second -= ms_since_last_tick;
  }
  for (auto &kv : _arp_req_time)
    kv.second += ms_since_last_tick;
}

void NetworkInterface::recheckOutQueue() {
  auto sz = _cache_frame.size();
  for (size_t i = 0; i < sz; ++i) {
    auto [dgram, next_hop] = _cache_frame.front();
    _cache_frame.pop();
    if (_arp_map.count(next_hop) > 0) {
      EthernetFrame eth;
      eth.header().src = _ethernet_address;
      eth.header().dst = _arp_map[next_hop].first;
      eth.header().type = EthernetHeader::TYPE_IPv4;
      eth.payload() = dgram.serialize();
      _frames_out.push(std::move(eth));
    } else {
      _cache_frame.push({dgram, next_hop});
    }
  }
}
