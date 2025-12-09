#include <iostream>
#include <vector>
#include <cstring> // memcpy
#include <iomanip> // setw, setfill

// ndn-cxx
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/encoding/buffer.hpp> // ndn::Buffer

// 自作クラス
#include "ecdsa_sig.hpp"

// メイン関数
int main() {
    try {
        // --- [設定] テストするノード（ルーター）のIDリスト ---
        // ここに検証したい順序でIDを追加していけば、自動で連鎖的にテストされます
        std::vector<std::vector<uint8_t>> node_ids = {
            {10, 0, 0, 1}, // Node 1
            {10, 0, 0, 2}  // Node 2
            // {10, 0, 0, 3} // 必要なら追加可能
        };

        // --- [共通設定] Interest Nameの準備 ---
        ndn::Name ex_name("/example/data");
        ndn::Interest interest(ex_name);
        std::cout << "[Setup] Interest Name: " << interest.toUri() << std::endl;
        
        // NameをBlock化 (署名対象の一部)
        ndn::Block signed_name_Block = interest.wireEncode();

        // --- ループ用変数の初期化 ---
        
        // 1. 前回の署名 (最初のノード用にはAll Zeroの署名を用意)
        // ※署名サイズを取得するためだけに一度ダミーでインスタンス化するか、既知のサイズ(例:64)を指定します
        ecdsa_sig dummy_signer; 
        size_t sig_size = dummy_signer.signature_size(); 
        std::vector<uint8_t> prev_signature(sig_size, 0); 

        // 2. 累積経路情報 (RI)
        std::vector<uint8_t> accumulated_ri_flat;

        // 3. RIの個数（Lengthフィールドのサイズ。uint8_tなら1バイト）
        const size_t RI_LENGTH_FIELD_SIZE = 1; 

        std::cout << "------------------------------------------------" << std::endl;

        // --- [ループ処理] 各ノードでの署名生成と検証 ---
        for (size_t i = 0; i < node_ids.size(); ++i) {
            std::cout << "\n[Step " << (i + 1) << "] Processing Node ID: ";
            for(auto b : node_ids[i]) std::cout << (int)b << ".";
            std::cout << std::endl;

            // 1. Signerの初期化
            ecdsa_sig signer;
            signer.set_id(node_ids[i]);
            signer.setup();
            signer.key_derivation();

            // 2. 現在のRIリストを更新（自身のIDを末尾に追加）
            // 2次元配列をフラットな1次元配列に追加していく
            accumulated_ri_flat.insert(accumulated_ri_flat.end(), node_ids[i].begin(), node_ids[i].end());

            // 現在のホップ数 (RIに含まれるIDの数)
            uint8_t current_hop_count = static_cast<uint8_t>(i + 1);

            // 3. バッファサイズの計算
            // 構成: [署名(Sig)] || [RI数(1byte)] || [累積RIデータ] || [Name]
            size_t total_size = sig_size + RI_LENGTH_FIELD_SIZE + accumulated_ri_flat.size() + signed_name_Block.size();
            
            std::vector<uint8_t> buf(total_size);
            size_t offset = 0;

            // 4. データ格納
            
            // (A) 前回の署名を格納 (今回はこの領域に自分の署名を上書きする仕様と仮定)
            // ※ 元コードの仕様では、署名領域にデータを入れた状態で sign() を呼ぶと
            //    そこに新しい署名が書き込まれる想定に見えます。
            //    ただし、署名対象データとして「前の署名」を含める必要があるため、まずはコピーします。
            std::memcpy(buf.data() + offset, prev_signature.data(), sig_size);
            offset += sig_size;

            // (B) 経由端末数 (RI Number)
            buf[offset] = current_hop_count;
            offset += RI_LENGTH_FIELD_SIZE;

            // (C) 累積RIデータ
            std::memcpy(buf.data() + offset, accumulated_ri_flat.data(), accumulated_ri_flat.size());
            offset += accumulated_ri_flat.size();

            // (D) Interest Name
            std::memcpy(buf.data() + offset, signed_name_Block.data(), signed_name_Block.size());
            offset += signed_name_Block.size();

            // --- デバッグ出力 ---
            std::cout << "  DEBUG: Buffer constructed. Total size: " << buf.size() << std::endl;
            std::cout << "  DEBUG: RI Count field: " << (int)current_hop_count << std::endl;

            // 5. 署名生成 (Sign)
            // signer.sign(buf) は buf の先頭 (offset 0) に署名を書き込むと仮定
            signer.sign(buf);
            std::cout << "  -> Signature Generated." << std::endl;

            // 6. 署名検証 (Verify)
            bool result = signer.verify(buf);
            if (result) {
                std::cout << "  -> Verification SUCCESS!" << std::endl;
            } else {
                std::cout << "  -> Verification FAILED!" << std::endl;
                return 1; // エラー終了
            }

            // 7. 次のループのために「今回の署名」を保存
            // bufの先頭からsig_size分が「今回の署名」になっているはずなので抽出して保存
            prev_signature.assign(buf.begin(), buf.begin() + sig_size);
        }

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nAll nodes processed successfully." << std::endl;
    return 0;
}