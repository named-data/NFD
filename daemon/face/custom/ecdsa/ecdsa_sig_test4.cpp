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

        std::vector<uint8_t> empty_sig(signer.signature_size(), 0); // 空の署名を初期化

                    // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG: empty_sig size = " << empty_sig.size() << std::endl;
        std::cout << "DEBUG: empty_sig data = ";
        for (const auto& byte : empty_sig) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

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
         * 順番としては RI_of_Number || RI（署名者のIDを最後に含む） || signatureそのもの || Interestのname
        */
        //格納するデータサイズを計算
        size_t total_size = signer.signature_size() + RI_of_Number + flat.size()  + signed_name_Block.size();
        //格納先バイト列
        std::vector<uint8_t> buf(total_size);
        //格納先位置の調整用
        size_t offset = 0;

        //signatureそのものの格納
        //0で埋めた署名を仮に格納しておく
        std::memcpy(buf.data() + offset, empty_sig.data(), signer.signature_size());
        offset += signer.signature_size();


        //経由端末数(RI of Number)の格納
        buf[offset] = RI_of_Number;
        offset += RI_of_Number;

        //bufに格納したRI_of_Numberを確認
        std::cout << "RI_of_Number in buf: " << static_cast<int>(buf[signer.signature_size()]) << std::endl;
        
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
        signer.sign(buf);

        //std::cout << " -> Generated Signature Size: " << signature1.size() << " bytes" << std::endl;



        //署名検証
        std::cout << "\n[Step 3] Verify Signature..." << std::endl;
        bool result = signer.verify(buf);

        if (result) {
            std::cout << " ->signer's Signature Verified Successfully!" << std::endl;
        } else {
            std::cout << " ->signer's Signature Verification Failed!" << std::endl;
            return 1;
        }

        // さらに別の署名も試す
        ecdsa_sig signer2;
        std::cout << " Initializing another ecdsa_sig scheme..." << std::endl;

        // ルーターID（ipアドレス）の設定 std::vector<uint8_t>を使うことになったため。 "10.0.0.1"で初期化
        std::vector<uint8_t> my_test_id2 = {10, 0, 0, 2};
        // ★ここでIDを設定
        signer2.set_id(my_test_id2);

        // マスター鍵等の生成・読み込み
        signer2.setup();
        signer2.key_derivation(); // IDに基づいて鍵が作られる
        std::cout << " -> OK" << std::endl;

        std::vector<std::vector<uint8_t>> RI2 = {{my_test_id}, {my_test_id2}};

        //経路情報の格納準備 2次元配列を1次元配列に変換
        std::vector<uint8_t> flat2;
        for (const auto& v : RI2) {
            flat2.insert(flat2.end(), v.begin(), v.end());
        }

        //flat2の中身を確認
        std::cout << "flat2 data: ";
        for (const auto& byte : flat2) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //RI2のサイズ確認
        std::uint8_t RI_of_Number2 = static_cast<uint8_t>(RI2.size());
        std::cout << "RI_of_Number2: " << static_cast<int>(RI_of_Number2) << std::endl;

        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 
         * 順番としては RI_of_Number || RI（署名者のIDを最後に含む） || signatureそのもの（signer1の署名） || Interestのname(signer1が署名したもの)
        */
        //格納するデータサイズを計算
        size_t total_size2 = signer.signature_size() + RI_LENGTH_SIZE + flat2.size()  + signed_name_Block.size();
        //total_size2の確認
        std::cout << "total_size2: " << total_size2 << std::endl;
        
        //格納先バイト列
        std::vector<uint8_t> buf2(total_size2);
        //格納先位置の調整用
        size_t offset2 = 0;

        //signatureそのものの格納
        //今回はsigner1の署名を格納
        std::memcpy(buf2.data() + offset2, buf.data(), signer.signature_size());
        offset2 += signer.signature_size();

        //経由端末数(RI of Number)の格納
        buf2[offset2] = RI_of_Number2;
        offset2 += RI_LENGTH_SIZE;

        //bufに格納したRI_of_Numberを確認　ここは2になるはず。
        std::cout << "RI_of_Number in buf: " << static_cast<int>(buf2[0]) << std::endl;
        
        //RI2（2端末分）の格納
        std::memcpy(buf2.data() + offset2, flat2.data(), flat2.size());
        offset2 += flat2.size(); 
        //offsetの確認
        std::cout << "Offset after RI2 copy: " << offset2 << std::endl;

        //interest name "/example/data"の格納
        std::memcpy(buf2.data() + offset2, signed_name_Block.data(), signed_name_Block.size());
        offset2 += signed_name_Block.size(); 

            // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG:  signed buf2 size = " << buf2.size() << std::endl;
        std::cout << "DEBUG:  signed buf2 data = ";
        for (const auto& byte : buf2) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //署名作成
        signer2.sign(buf2);

        //署名検証
        std::cout << "\n[Step 3] Verify Signature..." << std::endl;
        bool result2 = signer2.verify(buf2);

        if (result2) {
            std::cout << " -> signer2 Signature Verified Successfully!" << std::endl;
        } else {
            std::cout << " -> signer2 Signature Verification Failed!" << std::endl;
            return 1;
        }

        

        //std::cout << " -> Generated Signature Size: " << signature2.size() << " bytes" << std::endl;

        } catch (const std::exception& e) {
        std::cerr << "Exception during verification: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}