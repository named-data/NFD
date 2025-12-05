#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cstring>

// ndn-cxx
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/block-helpers.hpp>

// 自作クラス
#include "ecdsa_sig.hpp"


// テスト用のヘルパー関数: ダミーの ndn::Block を作成
ndn::Block createDummyBlock(uint32_t type, const std::string& content) {
    return ndn::makeStringBlock(type, content);
}

// テスト用のヘルパー関数: RI (Route Information) ブロックを作成
// elements_count: 経路の長さ (ホップ数)
ndn::Block createDummyRI(int elements_count) {
    ndn::EncodingBuffer encoder;
    size_t totalLength = 0;
    
    // RIの中身としてダミーのIDブロックなどを詰める
    for (int i = 0; i < elements_count; ++i) {
        std::string dummyId = "NODE_" + std::to_string(i);
        totalLength += prependStringBlock(encoder, 804, dummyId);
    }
    
    encoder.prependVarNumber(totalLength);
    encoder.prependVarNumber(904); // RIのType 越智くんの番号を採用
    
    return encoder.block();
}

//経路情報をstringへ変換するメソッド
std::string vector_vector_To_String(const std::vector<std::vector<uint8_t>>& routeInfo)
{
    std::string result;

    for (size_t i = 0; i < routeInfo.size(); ++i) {
        const auto& v = routeInfo[i];

        // vector<uint8_t> → string の追加
        result.append(reinterpret_cast<const char*>(v.data()), v.size());

        // 最後以外は改行で区切り
        if (i + 1 < routeInfo.size()) {
            result.push_back('\n');
        }
    }

    return result;
}

int main() {
    try {
        std::cout << "=== ECDSA Signature Scheme Test Start ===" << std::endl;

        // 【修正】テストで使用する任意のIDを定義
        std::vector<uint8_t> my_test_id = {10, 0, 0, 1}; 

        
        // 1. インスタンス生成とID設定
        std::cout << "[Step 1] Setup and Key Derivation..." << std::endl;
        ecdsa_sig signer;
        
        // ★ここでIDを設定！
        signer.set_id(my_test_id);
        
        signer.setup();
        signer.key_derivation(); // IDに基づいて鍵が作られる
        std::cout << " -> OK" << std::endl;

        // ==========================================
        // シナリオ 1: 署名 (Sign)
        // ==========================================
        std::cout << "\n[Step 2] Sign (First Hop)..." << std::endl;
        //ここ適当にmessageのtypeを10として設定しているけどおそらく厳密に定義する必要がある。
        std::vector<std::vector<u_int8_t>> ri1 = {{10, 0, 0, 1}};
        std::vector<uint8_t> empty_sig;

        // 署名 (内部で signer.id が使われる)
        std::vector<uint8_t> signature1 = signer.sign(ri1, empty_sig);
        
        std::cout << " -> Generated Signature Size: " << signature1.size() << " bytes" << std::endl;

        // シナリオ 2: 検証 (Verify)
        std::cout << "\n[Step 3] Verify (First Hop)..." << std::endl;
        
        // 検証実行
        bool result1 = signer.verify(ri1, signature1);
        
        if (result1) {
            std::cout << " -> Verification PASSED!" << std::endl;
        } else {
            std::cerr << " -> Verification FAILED!" << std::endl;
            return 1;
        }

        /**
         * ここから2端末目の署名の生成及び集約を行う
         */

        std::vector<uint8_t> my_test_id2 = {10, 0, 0, 2}; 
        
        // 1. インスタンス生成とID設定
        std::cout << "signer2 Setup and Key Derivation..." << std::endl;
        ecdsa_sig signer2;
        
        // ★ここでIDを設定！
        signer2.set_id(my_test_id2);
        
        signer2.setup();
        signer2.key_derivation(); // IDに基づいて鍵が作られる
        std::cout << " -> OK" << std::endl;

        std::cout << "\n[Step 2] Sign (Second Hop)..." << std::endl;

        std::vector<std::vector<u_int8_t>> ri2 = {
            {my_test_id},
            {my_test_id2}
        };
        std::string message2 = vector_vector_To_String(ri2);        

        // 署名 (内部で signer.id が使われる)
        std::vector<uint8_t> signature2 = signer2.sign(ri2, signature1);
        
        std::cout << " -> Generated Signature Size: " << signature2.size() << " bytes" << std::endl;

        std::cout << "\n[Step 3] Verify (Second Hop)..." << std::endl;

        // ★【修正2】これまでの履歴をすべてリストに入れる！
        // 集約署名 verify には「誰が(ID)」「何を(Msg)」署名したかの全履歴が必要です。
        

        
        // 検証実行
        // signature2 は (Aの署名 + Bの署名) なので、検証には (Aの情報 + Bの情報) が必要
        bool result2 = signer2.verify(ri2, signature2);
        
        if (result2) {
            std::cout << " -> Verification PASSED!" << std::endl;
        } else {
            std::cerr << " -> Verification FAILED!" << std::endl;
            return 1;
        }


        

        std::cout << "\n=== All Tests Passed Successfully ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}