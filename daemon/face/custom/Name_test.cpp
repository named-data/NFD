#include <ndn-cxx/name.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <iostream>
#include <vector>


int main() {
    // ---------------------------------------------------------
    // 1. 名前の作成 (Creation)
    // ---------------------------------------------------------
    std::cout << "--- 1. Creation ---" << std::endl;

    // URI文字列から作成
    ndn::Name name1("/ndn/jp/edu/oit");
    std::cout << "Name1 (from URI): " << name1 << std::endl;

    // 空の名前を作成してから構築
    ndn::Name name2;
    name2.append("ndn");
    name2.append("example");
    name2.append("test");
    std::cout << "Name2 (appended): " << name2 << std::endl;

    // ---------------------------------------------------------
    // 2. 名前の操作とメソッドチェーン (Manipulation)
    // ---------------------------------------------------------
    std::cout << "\n--- 2. Manipulation ---" << std::endl;

    // 特定のNDN命名規則（バージョン、セグメントなど）を追加
    // appendVersion() は引数なしだと現在時刻(ms)をバージョンとして付与
    ndn::Name dataName = name1;
    dataName.append("video")
            .append("hls")
            .appendVersion(1700000000000)      // 例: v=1700000000000
            .appendSegment(0);    // 例: seg=0

    std::cout << "Data Name: " << dataName << std::endl;

    // コンポーネントへのアクセス
    std::cout << "3rd component: " << dataName.get(2) << std::endl; // "oit" (0-indexed)
    std::cout << "Last component: " << dataName.get(-1) << std::endl; // seg=0

    // ---------------------------------------------------------
    // 3. 部分名前の取得 (Slicing)
    // ---------------------------------------------------------
    std::cout << "\n--- 3. Slicing ---" << std::endl;

    // getPrefix(n): 最初からn個のコンポーネントを取得
    ndn::Name prefix = dataName.getPrefix(4); // /ndn/jp/edu/oit
    std::cout << "Prefix (first 4): " << prefix << std::endl;

    // getPrefix(-1): 親の名前を取得（最後の1つを除く）
    ndn::Name parent = dataName.getPrefix(-1);
    std::cout << "Parent: " << parent << std::endl;

    // ---------------------------------------------------------
    // 4. 比較と検索 (Comparison)
    // ---------------------------------------------------------
    std::cout << "\n--- 4. Comparison ---" << std::endl;

    ndn::Name routePrefix("/ndn/jp");

    // isPrefixOf: ルーティングテーブル参照などで使用
    if (routePrefix.isPrefixOf(name1)) {
        std::cout << "'" << routePrefix << "' is a prefix of '" << name1 << "'" << std::endl;
    }

    // equals
    if (name1 == prefix) {
        std::cout << "Name1 and Prefix are equal." << std::endl;
    }

    // ---------------------------------------------------------
    // 5. ワイヤーエンコーディング (Encoding)
    // ---------------------------------------------------------
    std::cout << "\n--- 5. Encoding ---" << std::endl;

    // TLV形式（Block）へのエンコード
    // ネットワークに送信したり署名したりする前に必ず行われる
    const ndn::Block& wire = dataName.wireEncode();

    std::cout << "Wire encoding size: " << wire.size() << " bytes" << std::endl;
    std::cout << "Hex dump: ";
    for (const auto& byte : wire) {
        printf("%02X", byte);
    }
    std::cout << std::endl;

    return 0;
}