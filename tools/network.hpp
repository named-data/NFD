/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TOOLS_NETWORK_HPP
#define NFD_TOOLS_NETWORK_HPP

#include <boost/asio.hpp>

class Network
{
public:
  Network()
  {
  }

  Network(const boost::asio::ip::address& minAddress,
          const boost::asio::ip::address& maxAddress)
    : m_minAddress(minAddress)
    , m_maxAddress(maxAddress)
  {
  }

  void
  print(std::ostream& os) const
  {
    os << m_minAddress << " <-> " << m_maxAddress;
  }

  bool
  doesContain(const boost::asio::ip::address& address) const
  {
    return (m_minAddress <= address && address <= m_maxAddress);
  }

  static const Network&
  getMaxRangeV4()
  {
    using boost::asio::ip::address_v4;
    static Network range = Network(address_v4(0), address_v4(0xFFFFFFFF));
    return range;
  }

  static const Network&
  getMaxRangeV6()
  {
    using boost::asio::ip::address_v6;
    static address_v6::bytes_type maxV6 = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};
    static Network range = Network(address_v6(), address_v6(maxV6));
    return range;
  }

private:
  boost::asio::ip::address m_minAddress;
  boost::asio::ip::address m_maxAddress;

  friend std::istream&
  operator>>(std::istream& is, Network& network);

  friend std::ostream&
  operator<<(std::ostream& os, const Network& network);
};

inline std::ostream&
operator<<(std::ostream& os, const Network& network)
{
  network.print(os);
  return os;
}

inline std::istream&
operator>>(std::istream& is, Network& network)
{
  using namespace boost::asio;

  std::string networkStr;
  is >> networkStr;

  size_t position = networkStr.find('/');
  if (position == std::string::npos)
    {
      network.m_minAddress = ip::address::from_string(networkStr);
      network.m_maxAddress = ip::address::from_string(networkStr);
    }
  else
    {
      ip::address address = ip::address::from_string(networkStr.substr(0, position));
      size_t mask = boost::lexical_cast<size_t>(networkStr.substr(position+1));

      if (address.is_v4())
        {
          ip::address_v4::bytes_type maskBytes = {};
          for (size_t i = 0; i < mask; i++)
            {
              size_t byteId = i / 8;
              size_t bitIndex = 7 - i % 8;
              maskBytes[byteId] |= (1 << bitIndex);
            }

          ip::address_v4::bytes_type addressBytes = address.to_v4().to_bytes();
          ip::address_v4::bytes_type min;
          ip::address_v4::bytes_type max;

          for (size_t i = 0; i < addressBytes.size(); i++)
            {
              min[i] = addressBytes[i] & maskBytes[i];
              max[i] = addressBytes[i] | ~(maskBytes[i]);
            }

          network.m_minAddress = ip::address_v4(min);
          network.m_maxAddress = ip::address_v4(max);
        }
      else
        {
          ip::address_v6::bytes_type maskBytes = {};
          for (size_t i = 0; i < mask; i++)
            {
              size_t byteId = i / 8;
              size_t bitIndex = 7 - i % 8;
              maskBytes[byteId] |= (1 << bitIndex);
            }

          ip::address_v6::bytes_type addressBytes = address.to_v6().to_bytes();
          ip::address_v6::bytes_type min;
          ip::address_v6::bytes_type max;

          for (size_t i = 0; i < addressBytes.size(); i++)
            {
              min[i] = addressBytes[i] & maskBytes[i];
              max[i] = addressBytes[i] | ~(maskBytes[i]);
            }

          network.m_minAddress = ip::address_v6(min);
          network.m_maxAddress = ip::address_v6(max);
        }
    }
  return is;
}

#endif // NFD_TOOLS_NETWORK_HPP
