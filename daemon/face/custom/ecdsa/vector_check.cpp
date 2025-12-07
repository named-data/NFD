#include <vector>
#include <iostream>
#include <string>

/**
 * vectorの動作確認のためのコード
 */

// 経路情報をstringへ変換するメソッド
std::string vector_vector_To_String(const std::vector<std::vector<uint8_t>>& routeInfo)
{
    std::string result;

    for (size_t i = 0; i < routeInfo.size(); ++i) {
        const auto& v = routeInfo[i];

        // vector<uint8_t> → string の追加
        if (!v.empty()) {
            result.append(reinterpret_cast<const char*>(v.data()), v.size());
        }

        // 最後以外は改行で区切り
        if (i + 1 < routeInfo.size()) {
            result.push_back('\n');
        }
    }

    return result;
}


int main() {
    std::vector<std::vector<uint8_t>> RI = {
        {10,0,0,1},
        {10,0,0,2},
        {10,0,0,3},
        {10,0,0,4},
    };
    //std::vector<std::vector<uint8_t>> RI2 = {10,0,0,1}; コンパイルエラーを起こす書き方。
    
    std::cout << "RI.size() : " << RI.size() << std::endl;
    std::cout << "RI.at(0).size() : " << RI.at(0).size() << std::endl; 
    //std::cout << "RI[3] : " <<  RI[3] << std::endl;


}