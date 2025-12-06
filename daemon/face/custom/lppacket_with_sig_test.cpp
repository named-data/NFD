/*
 * simple_tlv_test.cpp
 * 目的:
 * 1. ecdsa_sig クラスの実際のメソッド (setup, sign, verify) を使用する。
 * 2. 経路情報および、署名を載せたLPパケットとしてエンコードし、バイト列にする。
 * 3. バイト列からデコードし、署名検証 (verify) を行う。
 *
 * 必要ファイル: ecdsa_sig.hpp, ecdsa_sig.cpp
 * 依存ライブラリ: ndn-cxx, mcl, openssl
 */

#include "ecdsa/ecdsa_sig.hpp"
#include "my-extension-tlv.hpp"

#include <ndn-cxx/encoding/tlv.hpp>
#include <ndn-cxx/lp/tlv.hpp>
#include <ndn-cxx/lp/packet.hpp>
#include <ndn-cxx/lp/fields.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/buffer.hpp> // ndn::Buffer
#include <ndn-cxx/util/span.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <stdexcept>

// 経路情報をstringへ変換するメソッド
std::string vector_vector_To_String(const std::vector<std::vector<uint8_t>>& routeInfo)
{
    std::string result;

    for (size_t i = 0; i < routeInfo.size(); ++i) {
        const auto& v = routeInfo[i];

        // vector<uint8_t> → string の追加
        if (!v.empty()) {
            result.append(reinterpret_cast<const char*>(v.data()), v.size());
        }

        // 最後以外は改行で区切り
        if (i + 1 < routeInfo.size()) {
            result.push_back('\n');
        }
    }

    return result;
}

int main() {
    std::cout << "--- Start Standalone Signature Verification Test (Real MCL) ---" << std::endl;

    try {
        std::cout << "[Setup] Initializing ecdsa_sig scheme..." << std::endl;
        ecdsa_sig signer;

        // ルーターID（ipアドレス）の設定 std::vector<uint8_t>を使うことになったため。 "10.0.0.1"で初期化
        std::vector<uint8_t> my_test_id = {10, 0, 0, 1};
        // ★ここでIDを設定！
        signer.set_id(my_test_id);

        // マスター鍵等の生成・読み込み
        signer.setup();
        signer.key_derivation(); // IDに基づいて鍵が作られる
        std::cout << " -> OK" << std::endl;

        std::vector<uint8_t> empty_sig;
        std::vector<std::vector<uint8_t>> RI1 = {{my_test_id}};

        // 署名作成
        std::vector<uint8_t> signature1 = signer.sign(RI1, empty_sig);

        std::cout << " -> Generated Signature Size: " << signature1.size() << " bytes" << std::endl;

        // LPpacketの作成。
        ndn::lp::Packet lpPacket;

        //routeinformationをlppacketにset
        lpPacket.set<ndn::lp::RouteInformationField>(RI1);

        
        // 署名ブロック (Type: Signature_ecdsaField) をヘッダーフィールドとして追加
        lpPacket.set<ndn::lp::Signature_ecdsaField>(signature1);

        //エンコード (Packet -> Wire/Block)
        ndn::Block wire = lpPacket.wireEncode();
        std::cout << "1. Wire Encoded. Total Size: " << wire.size() << " bytes" << std::endl;

        //デコード (Wire -> New Packet)
        ndn::lp::Packet receivedPacket(wire);
        std::cout << "2. Decoded into new lp::Packet." << std::endl;

        //デコードしたLPpacketの中身をとりだす。
        if(receivedPacket.has<ndn::lp::RouteInformationField>()) {
            auto riDecoded = receivedPacket.get<ndn::lp::RouteInformationField>();

            std::cout << "[Decoded] RouteInformation : " << std::endl;
            for(size_t i = 0; i < riDecoded.size(); ++i){
                std::cout << "Node" << i << ": ";
                for(uint8_t b : riDecoded[i]){
                    printf("%d.", b);
                }
                std::cout << std::endl;
            }
        } else {
            std::cout << "Could not find the RouteInformation in the LP packet." << std::endl;
        }

        //署名検証を行う。
        //デコードしたLPpacketの中身をとりだす。
        if(receivedPacket.has<ndn::lp::Signature_ecdsaField>()) {
            auto sig_Decoded = receivedPacket.get<ndn::lp::Signature_ecdsaField>();
            std::cout << "Signature information confirmed." << std::endl;

            auto ri_Decoded = receivedPacket.get<ndn::lp::RouteInformationField>();

            bool verify_result = signer.verify(ri_Decoded,sig_Decoded);

            if(verify_result) {
                std::cout << "verify passed" << std::endl;
            } else {
                std::cout << "verify failed" << std::endl;
            }

            
        } else {
            std::cout << "Could not find the RouteInformation in the LP packet." << std::endl;
        }



        } catch (const std::exception& e) {
        std::cerr << "Exception during verification: " << e.what() << std::endl;
        return 1;
    }

return 0;
}



        // // Interest (Message) の作成
        // ndn::Interest interest("/test/route/verify/real");
        // interest.setCanBePrefix(false);
        // interest.setMustBeFresh(false);
        // std::cout << "  - Interest created. Name: " << interest.getName() << std::endl;

        // // interest を wire (TLV バイト列) にエンコードする
        // const ndn::Block& interestWire = interest.wireEncode();

        // // 明示的にバッファへコピーしておく（ライフタイムを明確にするため）
        // ndn::Buffer interestBuf(interestWire.begin(), interestWire.end());

        // // LP の FragmentField に入れる際は、lp::FragmentField が期待する型に合わせること
        // // （ここでは std::pair<Buffer::const_iterator, Buffer::const_iterator> を渡す想定）
        // // コピー済みのバッファのイテレータを使うことでバッファのライフタイムが確保される。
        // // 注意: lpPacket に渡した後、lpPacket.wireEncode() 内で再エンコードされるため、
        // // interestBuf の寿命はこのスコープ内で十分であれば OK。
        // lpPacket.set<ndn::lp::FragmentField>(std::make_pair(interestBuf.begin(), interestBuf.end()));

        // std::cout << "\n--- [Verification] Encoding & Decoding Check ---" << std::endl;

    

//         // 3. 各フィールドの復元と確認

//         // --- A. Route Information (RI) の確認 ---
//         if (receivedPacket.has<ndn::lp::RouteInformationField>()) {
//             auto ri_rec = receivedPacket.get<ndn::lp::RouteInformationField>();
//             std::cout << "   [CHECK] RouteInfo: Found. (Size: " << ri_rec.size() << " entries)" << std::endl;

//             // 中身の検証 (1つ目のエントリが 10.0.0.1 か？)
//             if (!ri_rec.empty() && !ri_rec[0].empty()) {
//                  std::cout << "           First ID First Byte: " << static_cast<int>(ri_rec[0][0]) << " (Expected: 10)" << std::endl;
//             }
//         } else {
//             std::cerr << "   [ERROR] RouteInfo: NOT FOUND!" << std::endl;
//         }

//         // --- B. 署名 (Signature_ecdsaField) の確認 ---
//         if (receivedPacket.has<ndn::lp::Signature_ecdsaField>()) {
//             auto sig_rec = receivedPacket.get<ndn::lp::Signature_ecdsaField>();
//             std::cout << "   [CHECK] Signature: Found. Size: " << sig_rec.size() << " bytes" << std::endl;
//         } else {
//             std::cerr << "   [ERROR] Signature: NOT FOUND!" << std::endl;
//         }

//         // --- C. フラグメント (Interest) の確認 ---
//         if (receivedPacket.has<ndn::lp::FragmentField>()) {
//             auto fragPair = receivedPacket.get<ndn::lp::FragmentField>();

//             // fragPair は受信パケット内部のバッファに対するイテレータのペアになっているはず
//             // イテレータの差からサイズを求める
//             size_t fragSize = std::distance(fragPair.first, fragPair.second);
//             std::cout << "   [CHECK] Fragment: Found. Size: " << fragSize << " bytes" << std::endl;

//             // イテレータペアから一旦バッファを作成して Block を復元する（安全策）
//             ndn::Buffer fragBuf(fragPair.first, fragPair.second);
//             auto fragBufPtr = std::make_shared<const ndn::Buffer>(fragBuf);

//             // Block を復元（バッファが TLV 要素を先頭に含むことを期待）
//             ndn::Block interestBlock(fragBufPtr);

//             // Interest をパース
//             ndn::Interest receivedInterest(interestBlock);

//             std::cout << "           Decoded Interest Name: " << receivedInterest.getName() << std::endl;

//             if (receivedInterest.getName() == "/test/route/verify/real") {
//                 std::cout << "           -> MATCH! Interest restored successfully." << std::endl;
//             } else {
//                 std::cout << "           -> MISMATCH! Name is wrong." << std::endl;
//             }

//         } else {
//             std::cerr << "   [ERROR] Fragment: NOT FOUND!" << std::endl;
//         }

//     } catch (const std::exception& e) {
//         std::cerr << "Exception during verification: " << e.what() << std::endl;
//         return 1;
//     }

//     std::cout << "\n--- Test Finished ---" << std::endl;
//     return 0;
// }