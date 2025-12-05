#include <vector>
#include <iostream>

/**
 * vectorの動作確認のためのコード
 */


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