#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstring>

// ndn-cxx
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/encoding/buffer.hpp> // ndn::Buffer

// 自作クラス
#include "ecdsa_sig.hpp"

int main() {
    try {
        std::cout << "[Setup] Initializing ecdsa_sig scheme..." << std::endl;
        ecdsa_sig signer;


        // ルーターID（ipアドレス）の設定 std::vector<uint8_t>を使うことになったため。 "10.0.0.1"で初期化
        std::vector<uint8_t> my_test_id = {10, 0, 0, 1};
        // ★ここでIDを設定
        signer.set_id(my_test_id);

        // マスター鍵等の生成・読み込み
        signer.setup();
        signer.key_derivation(); // IDに基づいて鍵が作られる
        std::cout << " -> OK" << std::endl;

        std::vector<uint8_t> empty_sig;
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
        ndn::Block signed_name_Block = interst.wireEncode();
        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 
         * 順番としては RI_of_Number || RI（署名者のIDを最後に含む） || Interestのname
        */
        //格納するデータサイズを計算
        size_t total_size = RI_of_Number + flat.size() + signed_name_Block.size();
        //格納先バイト列
        std::vector<uint8_t> buf(total_size);
        //格納先位置の調整用
        size_t offset = 0;

        //経由端末数(RI of Number)の格納
        buf[offset] = RI_of_Number;
        offset += RI_of_Number;

        //bufに格納したRI_of_Numberを確認
        std::cout << "RI_of_Number in buf: " << static_cast<int>(buf[0]) << std::endl;
        
        //RIの格納
        std::memcpy(buf.data() + offset, flat.data(), flat.size());
        offset += flat.size(); 
        //offsetの確認
        std::cout << "Offset after RI copy: " << offset << std::endl;

        //interest name "/example/data"の格納
        std::memcpy(buf.data() + offset, signed_name_Block.data(), signed_name_Block.size());
        offset += signed_name_Block.size(); 

            // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG: signed buf size = " << buf.size() << std::endl;
        std::cout << "DEBUG: signed buf data = ";
        for (const auto& byte : buf) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //署名作成
        std::vector<uint8_t> signature1 = signer.sign(buf, empty_sig);

        std::cout << " -> Generated Signature Size: " << signature1.size() << " bytes" << std::endl;

        //署名検証
        std::cout << "\n[Step 3] Verify Signature..." << std::endl;
        bool result = signer.verify(buf, signature1);

        if (result) {
            std::cout << " -> Signature Verified Successfully!" << std::endl;
        } else {
            std::cout << " -> Signature Verification Failed!" << std::endl;
        }

        } catch (const std::exception& e) {
        std::cerr << "Exception during verification: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}