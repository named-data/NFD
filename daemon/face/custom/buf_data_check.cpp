/*
 *bufの中身が想定通りになっているかを確認するプログラム
 */

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

#define ID_SIZE 4


int main() {
    std::cout << "--- Start Standalone Signature Verification Test (Real MCL) ---" << std::endl;

    try {
        std::cout << "[Setup] Initializing ecdsa_sig scheme..." << std::endl;

        // ルーターID（ipアドレス）の設定 std::vector<uint8_t>を使うことになったため。 "10.0.0.1"で初期化
        std::vector<uint8_t> my_test_id = {10, 0, 0, 1};
        
        std::vector<std::vector<uint8_t>> RI1 = {{my_test_id}};

        //経路情報の格納準備 2次元配列を1次元配列に変換
        std::vector<uint8_t> flat;
        for (const auto& v : RI1) {
            flat.insert(flat.end(), v.begin(), v.end());
        }

        /**
         * lppacketに載せるinterestの作成を行う。
        */
        //Nameの宣言
        ndn::Name ex_name("/example/data"); 

        //interestにNameを設定
        ndn::Interest interst(ex_name);

        //名前チェック
        std::cout << "Interest1 created with name: " << interst.toUri() << std::endl;

        //RI1のサイズ確認
        std::uint8_t RI_of_Number = static_cast<uint8_t>(RI1.size());
        /**
         * 経路情報と組み合わせて一つのバイト列にしてしまう。
         * まずはBlockに変換（これ必要か？）
         * Blockの中身をbufにmemcpyし署名対象メッセージの完成
        */
        ndn::Block signed_name_Block = ex_name.wireEncode();
        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 
         * 順番としては署名者のID || RI_of_Number || RI || Interestのname
        */
        //格納するデータサイズを計算
        size_t total_size = my_test_id.size()+ RI_of_Number + flat.size() + signed_name_Block.size();
        //格納先バイト列
        std::vector<uint8_t> buf(total_size);
        //格納先位置の調整用
        size_t offset = 0;

        //署名者のIDの格納
        std::memcpy(buf.data() + offset, my_test_id.data(), my_test_id.size());
        offset = my_test_id.size();
        //経由端末数(RI of Number)の格納
        buf[offset] = RI_of_Number;
        offset += 1;
        
        //bufに格納したRI_of_Numberを確認
        std::cout << "RI_of_Number in buf: " << static_cast<int>(buf[my_test_id.size()]) << std::endl;
        std::cout << "RI_of_Number in buf[ID_SIZE]: " << static_cast<int>(buf[ID_SIZE]) << std::endl;


        
        //RIの格納
        std::memcpy(buf.data() + offset, flat.data(), flat.size());
        offset += flat.size(); 
        //interest name "/example/data"の格納
        std::memcpy(buf.data() + offset, signed_name_Block.data(), signed_name_Block.size());
        offset += signed_name_Block.size(); 

        //bufの中身を全て確認
        std::cout << "Buffer contents:" << std::endl;
        for (size_t i = 0; i < buf.size(); ++i) {
            std::cout << static_cast<int>(buf[i]) << " ";
        }
        std::cout << std::endl;

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