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
        signer.setup();
        
        std::vector<std::vector<uint8_t>> node_ids;

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
         * 経路情報と組み合わせて一つのバイト列にしてしまう。
         * まずはBlockに変換（これ必要か？）
         * Blockの中身をbufにmemcpyし署名対象メッセージの完成
        */
        ndn::Block signed_name_Block = interst.wireEncode();


    for (uint8_t last = 1; last <= 10; last++) {
        node_ids.push_back({10, 0, 0, last});
    
        // ★ここでIDを設定
        signer.set_id(node_ids.back());
        //設定したIDの確認
        std::cout << "Set node ID: ";
        signer.printVector(node_ids.back());

        // マスター鍵等の生成・読み込み
        signer.key_derivation(); // IDに基づいて鍵が作られる
        std::cout << " -> OK" << std::endl;

        std::vector<std::vector<uint8_t>> RI1 = {node_ids};

        //経路情報の格納準備 2次元配列を1次元配列に変換
        std::vector<uint8_t> flat;
        for (const auto& v : RI1) {
            flat.insert(flat.end(), v.begin(), v.end());
        }

        //flatの中身を確認
        std::cout << "flat data: ";
        for (const auto& byte : flat) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行


        //RI1のサイズ確認
        std::uint8_t RI_of_Number = static_cast<uint8_t>(RI1.size());
        std::cout << "RI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;

        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 
         * 順番としては RI_of_Number || RI（署名者のIDを最後に含む） || signatureそのもの || Interestのname
        */
        //格納するデータサイズを計算
        size_t total_size = signer.signature_size() + RI_of_Number + flat.size()  + signed_name_Block.size();
        std::cout << "Total signed message size(signer.signature_size() + RI_of_Number + flat.size()  + signed_name_Block.size()): " << total_size << " bytes" << std::endl;
        std::vector<uint8_t> empty_sig(total_size, 0); // 空の署名を初期化

        //自身の鍵を作成したらまずは一つ前の端末により生成された署名の検証を行う（2端末目以降）
        if(last > 1){
            // ---検証 デバッグ用出力コード開始 ---
            std::cout << "DEBUG: " << last << "番目の署名付与後のsize = " << empty_sig.size() << std::endl;
            std::cout << "DEBUG: " << last << "番目の署名付与後の data = ";
            for (const auto& byte : empty_sig) {
                // 2桁の16進数で表示 (例: 0a 00 ff ...)
                std::cout << std::hex << std::setw(2) << std::setfill('0') 
                        << static_cast<int>(byte) << " ";
            }

            //署名検証
            std::cout << "\n[Step 3] Verify Signature..." << std::endl;
            bool result = signer.verify(empty_sig);

            if (result) {
                std::cout << " ->signer's Signature Verified Successfully!" << std::endl;
            } else {
                std::cout << " ->signer's Signature Verification Failed!" << std::endl;
            }
        }
        //格納先バイト列
        //std::vector<uint8_t> buf(total_size);
        //格納先位置の調整用
        size_t offset = 0;

        //0で埋めた署名を仮に格納しているため、署名サイズ分offsetを進める 
        offset += signer.signature_size();


        //経由端末数(RI of Number)の格納
        std::cout << "格納するRI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;
        empty_sig[offset] = RI_of_Number;
        //bufに格納したRI_of_Numberを確認
        std::cout << "RI_of_Number in buf: " << static_cast<int>(empty_sig[offset]) << std::endl;
        offset += RI_LENGTH_SIZE;
        
        //RIの格納
        std::memcpy(empty_sig.data() + offset, flat.data(), flat.size());
        offset += flat.size(); 
        //offsetの確認
        std::cout << "Offset after RI copy: " << offset << std::endl;

        //interest name "/example/data"の格納
        std::memcpy(empty_sig.data() + offset, signed_name_Block.data(), signed_name_Block.size());
        offset += signed_name_Block.size(); 

            // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG: " << last << "番目の署名付与前の size = " << empty_sig.size() << std::endl;
        std::cout << "DEBUG: " << last << "番目の署名付与前の data = ";
        for (const auto& byte : empty_sig) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //署名作成
        signer.sign(empty_sig);

    }
        } catch (const std::exception& e) {
        std::cerr << "Exception during verification: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}