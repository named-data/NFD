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

        std::vector<uint8_t> id1 {10,0,0,1};
        std::vector<uint8_t> id2 {10,0,0,2};
        std::vector<uint8_t> id3 {10,0,0,3};
        std::vector<uint8_t> id4 {10,0,0,4};
        std::vector<uint8_t> id5 {10,0,0,5};
        std::vector<uint8_t> id6 {10,0,0,6};
        std::vector<uint8_t> id7 {10,0,0,7};
        std::vector<uint8_t> id8 {10,0,0,8};
        std::vector<uint8_t> id9 {10,0,0,9};
        std::vector<uint8_t> id10{10,0,0,10};

        // ============================
        //   ID1 の鍵生成
        // ============================
        ecdsa_sig signer1;
        signer1.set_id(id1);
        signer1.setup();
        std::cout << "Set ID = ";
        signer1.printVector(id1);
        signer1.key_derivation();
        std::cout << " -> key_derivation OK\n";

        // ============================
        //   ID2 の鍵生成
        // ============================
        ecdsa_sig signer2;
        signer2.set_id(id2);
        signer2.setup();
        std::cout << "Set ID = ";
        signer2.printVector(id2);
        signer2.key_derivation();
        std::cout << " -> key_derivation OK\n";


        // ============================
        //   ID3 の鍵生成
        // ============================
        ecdsa_sig signer3;
        signer3.set_id(id3);
        signer3.setup();
        std::cout << "Set ID = ";
        signer3.printVector(id3);
        signer3.key_derivation();
        std::cout << " -> key_derivation OK\n";

        // ============================
        //   ID4
        // ============================
        ecdsa_sig signer4;
        signer4.set_id(id4);
        signer4.setup();
        std::cout << "Set ID = ";
        signer4.printVector(id4);
        signer4.key_derivation();
        std::cout << " -> key_derivation OK\n";

        // ============================
        //   ID5
        // ============================
        ecdsa_sig signer5;
        signer5.set_id(id5);
        signer5.setup();
        std::cout << "Set ID = ";
        signer5.printVector(id5);
        signer5.key_derivation();
        std::cout << " -> key_derivation OK\n";

        // ============================
        //   ID6
        // ============================
        ecdsa_sig signer6;
        signer6.set_id(id6);
        signer6.setup();
        std::cout << "Set ID = ";
        signer6.printVector(id6);
        signer6.key_derivation();
        std::cout << " -> key_derivation OK\n";

        // ============================
        //   ID7
        // ============================
        ecdsa_sig signer7;
        signer7.set_id(id7);
        signer7.setup();
        std::cout << "Set ID = ";
        signer7.printVector(id7);
        signer7.key_derivation();
        std::cout << " -> key_derivation OK\n";

        // ============================
        //   ID8
        // ============================
        ecdsa_sig signer8;
        signer8.set_id(id8);
        signer8.setup();
        std::cout << "Set ID = ";
        signer8.printVector(id8);
        signer8.key_derivation();
        std::cout << " -> key_derivation OK\n";

        // ============================
        //   ID9
        // ============================
        ecdsa_sig signer9;
        signer9.set_id(id9);
        signer9.setup();
        std::cout << "Set ID = ";
        signer9.printVector(id9);
        signer9.key_derivation();
        std::cout << " -> key_derivation OK\n";
        // ============================
        //   ID10
        // ============================
        ecdsa_sig signer10;
        signer10.set_id(id10);
        std::cout << "Set ID = ";
        signer10.printVector(id10);
        signer10.key_derivation();
        std::cout << " -> key_derivation OK\n";

        /**
         * 署名付与と検証の実施
         */

        //各端末が持つIDを経路情報として格納するベクター
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
        ndn::Block signed_name_Block = interst.wireEncode();

        //経路情報の準備
        node_ids.push_back(id1);

        //経路情報の格納準備 2次元配列を1次元配列に変換
        std::vector<uint8_t> flat;
        for (const auto& v : node_ids) {
            flat.insert(flat.end(), v.begin(), v.end());
        }
        std::cout << "flat data: ";
        for (const auto& byte : flat) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        std::cout << "flat.size(): " << flat.size() << std::endl;

        //RIのサイズ確認
        std::uint8_t RI_of_Number = static_cast<uint8_t>(node_ids.size());
        std::cout << "RI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;

        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 順番としては RI_of_Number || RI（署名者のIDを最後に含む） || signatureそのもの || Interestのname
        */
        //格納するデータサイズを計算

        size_t total_size = signer1.signature_size() + RI_of_Number + flat.size()  + signed_name_Block.size();
        std::cout << "Total signed message size(signer.signature_size() + RI_of_Number + flat.size()  + signed_name_Block.size()): " << total_size << " bytes" << std::endl;
        //各サイズの内訳を確認
        std::cout << " - signer1.signature_size(): " << signer1.signature_size() << " bytes" << std::endl;
        std::cout << " - RI_of_Number: " << static_cast<int>(RI_of_Number) << " bytes" << std::endl;
        std::cout << " - flat.size(): " << flat.size() << " bytes" << std::endl;
        std::cout << " - signed_name_Block.size(): " << signed_name_Block.size() << " bytes" << std::endl;
        std::vector<uint8_t> signer1_sig(total_size, 0); // 空の署名を初期化

        //格納先位置の調整用
        size_t offset = 0;

        //0で埋めた署名を仮に格納しているため、署名サイズ分offsetを進める 
        offset += signer1.signature_size();


        //経由端末数(RI of Number)の格納
        std::cout << "格納するRI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;
        signer1_sig[offset] = RI_of_Number;
        //bufに格納したRI_of_Numberを確認
        std::cout << "RI_of_Number in buf: " << static_cast<int>(signer1_sig[offset]) << std::endl;
        offset += RI_LENGTH_SIZE;
        
        //RIの格納
        std::memcpy(signer1_sig.data() + offset, flat.data(), flat.size());
        offset += flat.size(); 
        //offsetの確認
        
        //interest name "/example/data"の格納
        std::memcpy(signer1_sig.data() + offset, signed_name_Block.data(), signed_name_Block.size());
        offset += signed_name_Block.size(); 

            // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG: " << 1 << "番目の署名付与前の size = " << signer1_sig.size() << std::endl;
        std::cout << "DEBUG: " << 1 << "番目の署名付与前の data = ";
        for (const auto& byte : signer1_sig) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //signer1の署名作成
        signer1.sign(signer1_sig);
        std::cout << " -> signer1 sign OK\n" << std::endl;

        //署名検証
        std::cout << "\n[Step 1] Verify Signature..." << std::endl;
        bool result1 = signer2.verify(signer1_sig);
        if (result1) {
            std::cout << " -> signer1's Signature Verified Successfully!" << std::endl;
        } else {
            std::cout << " -> signer1's Signature Verification Failed!" << std::endl;
            return 1;
        }
        //---------------------こから2端末目以降の処理を実施-------------------------
        //経路情報の格納準備
        node_ids.push_back(id2);
        flat.clear();

        for (const auto& v : node_ids) {
            flat.insert(flat.end(), v.begin(), v.end());
        }
        std::cout << "flat data: ";
        for (const auto& byte : flat) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //RIのサイズ確認
        RI_of_Number = static_cast<uint8_t>(node_ids.size());
        std::cout << "RI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;

        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 
         * 順番としては RI_of_Number || RI（署名者のIDを最後に含む） || signatureそのもの || Interestのname
        */
        //格納するデータサイズを計算
        total_size = signer2.signature_size() + RI_LENGTH_SIZE + flat.size()  + signed_name_Block.size();
        std::cout << "Total signed message size(signer2.signature_size() + RI_of_Number + flat.size()  + signed_name_Block.size()): " << total_size << " bytes" << std::endl;
        std::vector<uint8_t> signer2_sig(total_size, 0); // 空の署名を初期化

        //格納先位置の調整用
        offset = 0;

        //signer2が検証したsigner1の署名を格納
        std::memcpy(signer2_sig.data(), signer1_sig.data(), signer1.signature_size());
        //署名サイズ分offsetを進める 
        offset += signer1.signature_size();


        //経由端末数(RI of Number)の格納
        std::cout << "格納するRI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;
        signer2_sig[offset] = RI_of_Number;
        //bufに格納したRI_of_Numberを確認
        std::cout << "RI_of_Number in buf: " << static_cast<int>(signer2_sig[offset]) << std::endl;
        offset += RI_LENGTH_SIZE;
        
        //RIの格納
        std::memcpy(signer2_sig.data() + offset, flat.data(), flat.size());
        offset += flat.size(); 
        //offsetの確認
        std::cout << "Offset after RI copy: " << offset << std::endl;

        //interest name "/example/data"の格納
        std::memcpy(signer2_sig.data() + offset, signed_name_Block.data(), signed_name_Block.size());
        offset += signed_name_Block.size(); 

            // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG: " << 2 << "番目の署名付与前の size = " << signer2_sig.size() << std::endl;
        std::cout << "DEBUG: " << 2 << "番目の署名付与前の data = ";
        for (const auto& byte : signer2_sig) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //signer2の署名作成
        signer2.sign(signer2_sig);
        std::cout << " -> signer2 sign OK\n" << std::endl;

        //signer3による署名検証
        std::cout << "\n[Step 2] Verify Signature..." << std::endl;
        bool result2 = signer3.verify(signer2_sig);
        if (result2) {
            std::cout << " -> signer2's Signature Verified Successfully!" << std::endl;
        } else {
            std::cout << " -> signer2's Signature Verification Failed!" << std::endl;
            return 1;
        }
        //---------------------こから3端末目以降の処理を実施-------------------------
        node_ids.push_back(id3);
        flat.clear();

        for (const auto& v : node_ids) {
            flat.insert(flat.end(), v.begin(), v.end());
        }
        std::cout << "flat data: ";
        for (const auto& byte : flat) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //RIのサイズ確認
        RI_of_Number = static_cast<uint8_t>(node_ids.size());
        std::cout << "RI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;

        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 
         * 順番としては RI_of_Number || RI（署名者のIDを最後に含む） || signatureそのもの || Interestのname
        */
        //格納するデータサイズを計算
        total_size = signer3.signature_size() + RI_LENGTH_SIZE + flat.size()  + signed_name_Block.size();
        std::cout << "Total signed message size(signer2.signature_size() + RI_of_Number + flat.size()  + signed_name_Block.size()): " << total_size << " bytes" << std::endl;
        std::vector<uint8_t> signer3_sig(total_size, 0); // 空の署名を初期化

        //格納先位置の調整用
        offset = 0;

        //signer3が検証したsigner2の署名を格納
        std::memcpy(signer3_sig.data(), signer2_sig.data(), signer2.signature_size());
        //署名サイズ分offsetを進める 
        offset += signer2.signature_size();


        //経由端末数(RI of Number)の格納
        std::cout << "格納するRI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;
        signer3_sig[offset] = RI_of_Number;
        //bufに格納したRI_of_Numberを確認
        std::cout << "RI_of_Number in buf: " << static_cast<int>(signer3_sig[offset]) << std::endl;
        offset += RI_LENGTH_SIZE;
        
        //RIの格納
        std::memcpy(signer3_sig.data() + offset, flat.data(), flat.size());
        offset += flat.size(); 
        //offsetの確認
        std::cout << "Offset after RI copy: " << offset << std::endl;

        //interest name "/example/data"の格納
        std::memcpy(signer3_sig.data() + offset, signed_name_Block.data(), signed_name_Block.size());
        offset += signed_name_Block.size(); 

            // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG: " << 2 << "番目の署名付与前の size = " << signer3_sig.size() << std::endl;
        std::cout << "DEBUG: " << 2 << "番目の署名付与前の data = ";
        for (const auto& byte : signer3_sig) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //signer3の署名作成
        signer3.sign(signer3_sig);
        std::cout << " -> signer3 sign OK\n" << std::endl;

        //signer4による署名検証
        std::cout << "\n[Step 2] Verify Signature..." << std::endl;
        bool result3 = signer4.verify(signer3_sig);
        if (result3) {
            std::cout << " -> signer3's Signature Verified Successfully!" << std::endl;
        } else {
            std::cout << " -> signer3's Signature Verification Failed!" << std::endl;
            return 1;
        }
        //---------------------ここから4端末目の処理を実施-------------------------
        node_ids.push_back(id4);
        flat.clear();

        for (const auto& v : node_ids) {
            flat.insert(flat.end(), v.begin(), v.end());
        }
        std::cout << "flat data: ";
        for (const auto& byte : flat) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //RIのサイズ確認
        RI_of_Number = static_cast<uint8_t>(node_ids.size());
        std::cout << "RI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;

        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 
         * 順番としては RI_of_Number || RI（署名者のIDを最後に含む） || signatureそのもの || Interestのname
        */
        //格納するデータサイズを計算
        total_size = signer4.signature_size() + RI_LENGTH_SIZE + flat.size()  + signed_name_Block.size();
        std::cout << "Total signed message size(signer4.signature_size() + RI_of_Number + flat.size()  + signed_name_Block.size()): " << total_size << " bytes" << std::endl;
        std::vector<uint8_t> signer4_sig(total_size, 0); // 空の署名を初期化

        //格納先位置の調整用
        offset = 0;

        //signer4が検証したsigner3の署名を格納
        std::memcpy(signer4_sig.data(), signer3_sig.data(), signer3.signature_size());
        //署名サイズ分offsetを進める 
        offset += signer3.signature_size();


        //経由端末数(RI of Number)の格納
        std::cout << "格納するRI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;
        signer4_sig[offset] = RI_of_Number;
        //bufに格納したRI_of_Numberを確認
        std::cout << "RI_of_Number in buf: " << static_cast<int>(signer4_sig[offset]) << std::endl;
        offset += RI_LENGTH_SIZE;
        
        //RIの格納
        std::memcpy(signer4_sig.data() + offset, flat.data(), flat.size());
        offset += flat.size(); 
        //offsetの確認
        std::cout << "Offset after RI copy: " << offset << std::endl;

        //interest name "/example/data"の格納
        std::memcpy(signer4_sig.data() + offset, signed_name_Block.data(), signed_name_Block.size());
        offset += signed_name_Block.size(); 

            // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG: " << 2 << "番目の署名付与前の size = " << signer4_sig.size() << std::endl;
        std::cout << "DEBUG: " << 2 << "番目の署名付与前の data = ";
        for (const auto& byte : signer4_sig) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //signer4の署名作成
        signer4.sign(signer4_sig);
        std::cout << " -> signer4 sign OK\n" << std::endl;

        //signer4による署名検証
        std::cout << "\n[Step 2] Verify Signature..." << std::endl;
        bool result4 = signer4.verify(signer3_sig);
        if (result4) {
            std::cout << " -> signer4's Signature Verified Successfully!" << std::endl;
        } else {
            std::cout << " -> signer4's Signature Verification Failed!" << std::endl;
            return 1;
        }
        //---------------------ここから5端末目の処理を実施-------------------------
        node_ids.push_back(id5);
        flat.clear();

        for (const auto& v : node_ids) {
            flat.insert(flat.end(), v.begin(), v.end());
        }
        std::cout << "flat data: ";
        for (const auto& byte : flat) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //RIのサイズ確認
        RI_of_Number = static_cast<uint8_t>(node_ids.size());
        std::cout << "RI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;

        /**
         * 署名対象メッセージの作成（ひとまず、InterstのName＋経路情報をバイト列にしてしまう）
         * 順番としては RI_of_Number || RI（署名者のIDを最後に含む） || signatureそのもの || Interestのname
        */
        //格納するデータサイズを計算
        total_size = signer5.signature_size() + RI_LENGTH_SIZE + flat.size()  + signed_name_Block.size();
        std::cout << "Total signed message size(signer5.signature_size() + RI_of_Number + flat.size()  + signed_name_Block.size()): " << total_size << " bytes" << std::endl;
        std::vector<uint8_t> signer5_sig(total_size, 0); // 空の署名を初期化

        //格納先位置の調整用
        offset = 0;

        //signer5が検証したsigner4の署名を格納
        std::memcpy(signer5_sig.data(), signer4_sig.data(), signer4.signature_size());
        //署名サイズ分offsetを進める 
        offset += signer4.signature_size();

        //経由端末数(RI of Number)の格納
        std::cout << "格納するRI_of_Number: " << static_cast<int>(RI_of_Number) << std::endl;
        signer5_sig[offset] = RI_of_Number;
        //bufに格納したRI_of_Numberを確認
        std::cout << "RI_of_Number in buf: " << static_cast<int>(signer5_sig[offset]) << std::endl;
        offset += RI_LENGTH_SIZE;
        
        //RIの格納
        std::memcpy(signer5_sig.data() + offset, flat.data(), flat.size());
        offset += flat.size(); 
        //offsetの確認
        std::cout << "Offset after RI copy: " << offset << std::endl;

        //interest name "/example/data"の格納
        std::memcpy(signer5_sig.data() + offset, signed_name_Block.data(), signed_name_Block.size());
        offset += signed_name_Block.size(); 

            // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG: " << 5 << "番目の署名付与前の size = " << signer5_sig.size() << std::endl;
        std::cout << "DEBUG: " << 5 << "番目の署名付与前の data = ";
        for (const auto& byte : signer5_sig) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行

        //signer5の署名作成
        signer5.sign(signer5_sig);
        std::cout << " -> signer5 sign OK\n" << std::endl;

        //signer5による署名検証
        std::cout << "\n[Step 2] Verify Signature..." << std::endl;
        bool result5 = signer6.verify(signer5_sig);
        if (result5) {
            std::cout << " -> signer5's Signature Verified Successfully!" << std::endl;
        } else {
            std::cout << " -> signer5's Signature Verification Failed!" << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception during verification: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}