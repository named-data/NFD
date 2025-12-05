#ifndef MY_EXTENSION_TLV_HPP
#define MY_EXTENSION_TLV_HPP

#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/buffer.hpp>
#include <ndn-cxx/lp/tlv.hpp>
#include <ndn-cxx/lp/field-decl.hpp>
#include <ndn-cxx/lp/field.hpp>

#include <vector>
#include <cstdint>
#include <memory>

namespace ndn {
namespace lp {
namespace tlv {

inline constexpr uint32_t Signature_ecdsa = 904;
inline constexpr uint32_t RouteInformation = 912;

} // namespace tlv
} // namespace lp
} // namespace ndn

// ======================================================================
//  RouteInformationCodec
// ======================================================================
class RouteInformationCodec
{
public:
  using ValueType = std::vector<std::vector<uint8_t>>;

  static ValueType
  decode(const ndn::Block& block)
  {
    const_cast<ndn::Block&>(block).parse();
    ValueType res;

    for (const auto& child : block.elements()) {
      res.emplace_back(child.value(), child.value() + child.value_size());
    }
    return res;
  }

  static ndn::Block
  encode(const ValueType& routes)
  {
    ndn::Block outer(ndn::lp::tlv::RouteInformation);

    for (const auto& route : routes) {
      auto buf = std::make_shared<const ndn::Buffer>(route.begin(), route.end());
      ndn::Block inner(ndn::lp::tlv::RouteInformation, buf);
      inner.encode();
      outer.push_back(inner);
    }

    outer.encode();
    return outer;
  }
};

// ======================================================================
//  BytesCodec (for Signature)
// ======================================================================
class BytesCodec
{
public:
  using ValueType = std::vector<uint8_t>;

  static ValueType
  decode(const ndn::Block& block)
  {
    return ValueType(block.value(), block.value() + block.value_size());
  }

  static ndn::Block
  encode(const ValueType& bytes)
  {
    auto buf = std::make_shared<const ndn::Buffer>(bytes.begin(), bytes.end());
    ndn::Block b(ndn::lp::tlv::Signature_ecdsa, buf);
    b.encode();
    return b;
  }
};

// ======================================================================
//    Helper Tag Types
// ======================================================================

namespace ndn {
namespace lp {

struct RouteInformationTag {};
struct SignatureEcdsaTag {};

using RouteInfoTlv = std::integral_constant<uint32_t, tlv::RouteInformation>;
using SignatureEcdsaTlv = std::integral_constant<uint32_t, tlv::Signature_ecdsa>;

template<>
struct DecodeHelper<RouteInfoTlv, RouteInformationTag>
{
  using ValueType = RouteInformationCodec::ValueType;
  static ValueType decode(const ndn::Block& wire)
  {
    return RouteInformationCodec::decode(wire);
  }
};

template<>
struct DecodeHelper<SignatureEcdsaTlv, SignatureEcdsaTag>
{
  using ValueType = BytesCodec::ValueType;
  static ValueType decode(const ndn::Block& wire)
  {
    return BytesCodec::decode(wire);
  }
};

// RouteInformationCodec 用 EncodeHelper
template <ndn::encoding::Tag TAG, typename TlvType>
struct EncodeHelper<TAG, TlvType, RouteInformationCodec>
{
  static size_t
  encode(ndn::encoding::EncodingImpl<TAG>& encoder,
         const RouteInformationCodec::ValueType& value)
  {
    // Block を作って encode して value() を確定させる
    ndn::Block block = RouteInformationCodec::encode(value);
    block.encode();

    size_t total = 0;

    // VALUE
    total += encoder.prependRange(block.value_begin(), block.value_end());

    // LENGTH
    total += encoder.prependVarNumber(block.value_size());

    // TYPE
    total += encoder.prependVarNumber(block.type());

    return total;
  }
};

// BytesCodec 用 EncodeHelper (署名フィールド)
template <ndn::encoding::Tag TAG, typename TlvType>
struct EncodeHelper<TAG, TlvType, BytesCodec>
{
  static size_t
  encode(ndn::encoding::EncodingImpl<TAG>& encoder,
         const BytesCodec::ValueType& value)
  {
    ndn::Block block = BytesCodec::encode(value);
    block.encode();

    size_t total = 0;

    // VALUE
    total += encoder.prependRange(block.value_begin(), block.value_end());

    // LENGTH
    total += encoder.prependVarNumber(block.value_size());

    // TYPE
    total += encoder.prependVarNumber(block.type());

    return total;
  }
};

// ======================================================================
//  FieldDecl typedefs
// ======================================================================
typedef FieldDecl<
  field_location_tags::Header,
  RouteInformationCodec::ValueType,
  tlv::RouteInformation,
  false,
  RouteInformationTag,
  RouteInformationCodec
> RouteInformationField;

typedef FieldDecl<
  field_location_tags::Header,
  BytesCodec::ValueType,
  tlv::Signature_ecdsa,
  false,
  SignatureEcdsaTag,
  BytesCodec
> Signature_ecdsaField;

} // namespace lp
} // namespace ndn

#endif // MY_EXTENSION_TLV_HPP