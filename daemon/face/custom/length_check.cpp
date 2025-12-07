/**
 * 各データ要素の長さを確認するためのコード。
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
        /**
         * まずはBlockに変換（これ必要か？）
         * Blockの中身をbufにmemcpyし署名対象メッセージの完成
        */
        ndn::Block signed_name_Block = ex_name.wireEncode();
        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 
         * 順番としては署名者のID || RI || Interestのname
        */
        //格納するデータサイズを計算
        size_t total_size = my_test_id.size()+ flat.size() + signed_name_Block.size();

        std::cout << "my_test_id_size : " << my_test_id.size() << std::endl;

        std::cout << "RI of Number : " << RI1.size() << std::endl;
        
        std::cout << "flat_size : " << flat.size() << std::endl;
        
        std::cout << "signed_name_Block_size : " << signed_name_Block.size() << std::endl; 

        std::cout << "total_size : " << total_size << std::endl; 

        } catch (const std::exception& e) {
        std::cerr << "Exception during verification: " << e.what() << std::endl;
        return 1;
    }

return 0;
}