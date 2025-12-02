#include "custom-lp-fields.hpp"
#include "ndn-cxx/lp/packet.hpp"
#include <vector>
#include <iostream>

/**
 * custom-lp-fields.hppの使用例
 */

int main() {
  using namespace ndn::lp;

  // 署名バイト列（ECDSAなどで生成済みと仮定）
  std::vector<uint8_t> signature = {0x01, 0x02, 0x03, 0x04};

  // LPパケットを作成
  Packet lpPacket;

  // カスタム署名フィールドを追加
  lpPacket.add<SignatureField>(signature);

  // エンコードして wire フォーマットに変換
  ndn::Block wire = lpPacket.wireEncode();

  std::cout << "Encoded LP packet size: " << wire.size() << " bytes" << std::endl;

  // デコードして署名を取り出す例
  Packet decodedPacket(wire);
  auto extractedSig = decodedPacket.get<SignatureField>();
  
  std::cout << "Extracted signature size: " << extractedSig.size() << " bytes" << std::endl;

  return 0;
}