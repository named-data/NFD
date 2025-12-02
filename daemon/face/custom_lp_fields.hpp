// custom-lp-fields.hpp
#ifndef CUSTOM_LP_FIELDS_HPP
#define CUSTOM_LP_FIELDS_HPP

#include "ndn-cxx/lp/tlv.hpp"
#include "ndn-cxx/encoding/block.hpp"
#include <vector>

/**
 * 目的
 * ndn::lp のパケットに「署名バイト列」を扱うカスタムフィールドを追加するためのユーティリティクラスです。
 * Packet のヘッダ／フィールドとして扱えるように、エンコード／デコードのインターフェースを提供しています。
 * 
 * 
 * 主な要素
 * using ValueType = std::vector<uint8_t>;
 *  → フィールドの値はバイト配列として扱う（署名データを格納）。
 * 
 * static constexpr uint32_t TlvType = 0xFD;
 *  → このフィールドに割り当てるTLVタイプ（任意に選んだ値）。既存のTLVと衝突しない値を選ぶ必要があります。
 * 
 * static constexpr bool IsRepeatable = false;
 *  → このフィールドは繰り返し不可であることを示すフラグ。
 * 
 * decode(const ndn::Block& block)
 *  → ndn::Block（既にTLVヘッダを解釈済みのブロック）からバイト列を抽出して ValueType を返す。
 * 
 * template<typename Buffer> static size_t encode(Buffer& buffer, const ValueType& value)
 *  → エンコード用。与えられたバッファに value のバイト列を prepend（先頭に追加）し、そのサイズを返す。
 * 
 * 実装上の注意点
 *  TlvType の値はライブラリ／プロトコルで予約されていないものを選ぶこと。
 *  衝突すると相互運用性の問題になります。
 *  decode は Block の value 部分だけを取り出すので問題なし。
 *  encode は生の値バイト列を buffer.prepend しているだけです。
 *  ndn-cxx のフィールド実装では「フィールドが値部分のみをエンコードし、FieldDecl 等がTLVヘッダを付ける」
 *  設計になっていることが多いので、その設計に合っていればこれで正しいです。
 *  もし呼び出し側でTLVヘッダの付与を期待している場合はヘッダも書く必要があります。
 *  Packet API（ndn::lp::Packet）に組み込んで使うには、このクラスを FieldDecl でラップして FieldSet に追加するか、
 *  直接 Packet のエンコード／デコード箇所で SignatureField::encode/decode を呼び出す形になります。
 */

namespace ndn::lp {

/**
 * @brief LPパケットに組み込む署名用カスタムTLV
 * 
 * 署名バイト列を保持する。LPパケット内の他フィールドと同様に扱える。
 */
class SignatureField
{
public:
  using ValueType = std::vector<uint8_t>;

  // 任意のTLVタイプ（LPv2の空いている範囲を利用）
  static constexpr uint32_t TlvType = 0xFD; // 適当な未使用値を選択
  static constexpr bool IsRepeatable = false;

  // デコード（LPPacketから取り出す）
  static ValueType decode(const ndn::Block& block)
  {
    return ValueType(block.value(), block.value() + block.value_size());
  }

  // エンコード（LPPacketに追加する際に使う）
  template<typename Buffer>
  static size_t encode(Buffer& buffer, const ValueType& value)
  {
    buffer.prepend(value.data(), value.size());
    return value.size();
  }
};

} // namespace ndn::lp

#endif // CUSTOM_LP_FIELDS_HPP