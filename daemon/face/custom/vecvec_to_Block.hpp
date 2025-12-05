#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/lp/tlv.hpp>
#include "my-extension-tlv.hpp"
#include <vector>

class RouteInformationCodec
{
public:
  static ndn::Block
  encode(const std::vector<std::vector<uint8_t>>& routes, uint32_t outerType)
  {
    ndn::Block outer(outerType);

    for (const auto& route : routes) {
      auto buf = std::make_shared<ndn::Buffer>(route.begin(), route.end());
      ndn::Block inner(912, buf);  // inner は固定でも可
      inner.encode();
      outer.push_back(inner);
    }

    outer.encode();
    return outer;
  }

  static std::vector<std::vector<uint8_t>>
decode(const ndn::Block& block)
{
  block.parse();
  std::vector<std::vector<uint8_t>> res;

  for (const auto& child : block.elements()) {
    const uint8_t* b = child.value();
    const uint8_t* e = child.value() + child.value_size();
    res.emplace_back(b, e);
  }
  return res;
 }

};

class BytesCodec {
public:
  static std::vector<uint8_t>
  decode(const ndn::Block& block)
  {
    return std::vector<uint8_t>(block.value(), block.value() + block.value_size());
  }

  static ndn::Block
  encode(const std::vector<uint8_t>& vec, uint32_t type)
  {
    // vec の内容を ndn::Buffer にコピーして shared_ptr<const Buffer> を作る
    auto bufPtr = std::make_shared<const ndn::Buffer>(vec.begin(), vec.end());

    // Block のコンストラクタ (type, ConstBufferPtr) を使う
    ndn::Block b(type, bufPtr);

    // ワイヤフォーマットが必要なら encode() を呼ぶ
    b.encode();

    return b;
  }
};