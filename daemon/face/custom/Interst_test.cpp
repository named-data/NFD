/**
 * Interestパケットを作成し、エンコードおよびデコードをテストするためのファイル。
 */

#include <iostream>
#include <memory>       // std::shared_ptr, std::make_shared
//#include "Name.hpp"     // NDN Nameクラス（仮）
#include <ndn-cxx/interest.hpp>// Interestクラス（仮）

int main() {
    try {
        // Nameオブジェクトを作成
        ndn::Name name("/example/data");

        // 1. 通常のInterestを生成（デフォルト寿命を使用）
        auto interest1 = std::make_shared<ndn::Interest>(name);
        std::cout << "Interest1 created with name: " << name.toUri() << std::endl;

        // 2. Interestを指定寿命で生成
        ndn::time::milliseconds lifetime(2000); // 2秒
        auto interest2 = std::make_shared<ndn::Interest>(name, lifetime);
        std::cout << "Interest2 created with lifetime: " << lifetime.count() << " ms" << std::endl;

        // 3. デフォルトコンストラクタでも作れる（名前未指定）
        auto interest3 = std::make_shared<ndn::Interest>();
        std::cout << "Interest3 created with default name" << std::endl;

        interest1->getNonce(); // Nonceを取得（存在しない場合は生成される）
        std::cout << "Interest1 Nonce: " << interest1->getNonce() << std::endl;

        

    } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: " << e.what() << std::endl;
    } catch (const std::bad_weak_ptr& e) {
        std::cerr << "Error: shared_from_this() requires std::make_shared - " << e.what() << std::endl;
    }

    return 0;
}