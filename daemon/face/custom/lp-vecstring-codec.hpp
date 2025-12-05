#ifndef NDN_CXX_LP_VECSTRING_CODEC_HPP
#define NDN_CXX_LP_VECSTRING_CODEC_HPP

#include <ndn-cxx/lp/field-decl.hpp>
#include <string>
#include <vector>

namespace ndn::lp
{

  /** ============================================================
   * Empty tag-types representing codec
   * 
   * LpStringCodec: for single string
   * LpVecStringCodec: for vector<string>
   * ============================================================ */
  class LpStringCodec
  {
  };
  class LpVecStringCodec
  {
  };

  /** ============================================================
   * DecodeHelper for string
   * ============================================================ */
  template <typename TlvType>
  struct DecodeHelper<TlvType, LpStringCodec>
  {
    static std::string
    decode(const Block &wire)
    {
      return std::string(
          reinterpret_cast<const char *>(wire.value()),
          wire.value_size());
    }
  };

  /** ============================================================
   * EncodeHelper for string
   * ============================================================ */
  template <encoding::Tag TAG, typename TlvType>
  struct EncodeHelper<TAG, TlvType, LpStringCodec>
  {
    static size_t
    encode(encoding::EncodingImpl<TAG> &encoder, const std::string &value)
    {
      size_t length = 0;

      // Value
      length += encoder.prependRange(
          reinterpret_cast<const uint8_t *>(value.data()),
          reinterpret_cast<const uint8_t *>(value.data() + value.size()));

      // Length
      length += encoder.prependVarNumber(value.size());

      // Type
      length += encoder.prependVarNumber(TlvType::value);

      return length;
    }
  };

  /** ============================================================
   * DecodeHelper for vector<string>
   *
   * Block Structure:
   *   <TlvType> <Length> <elem1> <elem2> ... <elemN>
   *
   * Each elem is also <TlvType> TLV containing a string.
   * ============================================================ */
  template <typename TlvType>
  struct DecodeHelper<TlvType, LpVecStringCodec>
  {
    static std::vector<std::string>
    decode(const Block &wire)
    {
      std::vector<std::string> result;

      // Parse nested TLVs
      wire.parse();

      for (const auto &elem : wire.elements())
      {
        if (elem.type() == TlvType::value)
        {
          result.emplace_back(
              reinterpret_cast<const char *>(elem.value()),
              elem.value_size());
        }
      }

      return result;
    }
  };

  /** ============================================================
   * EncodeHelper for vector<string>
   *
   * Generates:
   *   <TlvType><len of all children>
   *      <TlvType><len><value>    for each element in vector
   *
   * Note: Encoder prepends → iterate vector in reverse
   * ============================================================ */
  template <encoding::Tag TAG, typename TlvType>
  struct EncodeHelper<TAG, TlvType, LpVecStringCodec>
  {
    static size_t
    encode(encoding::EncodingImpl<TAG> &encoder,
           const std::vector<std::string> &vec)
    {
      size_t innerLength = 0;

      // Encode children TLVs in reverse order
      for (auto it = vec.rbegin(); it != vec.rend(); ++it)
      {
        const std::string &s = *it;

        // Value
        innerLength += encoder.prependRange(
            reinterpret_cast<const uint8_t *>(s.data()),
            reinterpret_cast<const uint8_t *>(s.data() + s.size()));

        // Length
        innerLength += encoder.prependVarNumber(s.size());

        // Type
        innerLength += encoder.prependVarNumber(TlvType::value);
      }

      // Now wrap vector<string> with outer TLV
      size_t totalLength = 0;

      // Inner TLVs length
      totalLength += encoder.prependVarNumber(innerLength);

      // Outer type
      totalLength += encoder.prependVarNumber(TlvType::value);

      return totalLength + innerLength;
    }
  };

} // namespace ndn::lp

#endif // NDN_CXX_LP_VECSTRING_CODEC_HPP
