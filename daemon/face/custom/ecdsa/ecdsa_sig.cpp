#include "ecdsa_sig.hpp"
#include <openssl/sha.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>


#define  ADDR_SIZE 4



/**
 * @file ecdsa_sig.cpp
 * @brief IDベース ECDSA 署名スキームの実装
 *
 * このソースファイルは、`ecdsa_sig.hpp` に定義された ecdsa_sig クラスの
 * メンバ関数を実装する。
 *
 * 【主な関数】
 * - signature_size(): 署名サイズを取得
 * - H1, H2, H3: ハッシュ関数 (G1 / Fr へのマッピング)
 * - deserialize_keys(): マスター鍵と秘密鍵をバイト列から復元
 * - serialize_sig / deserialize_sig: 署名のシリアライズ・デシリアライズ
 * - setup(): 初期化およびマスター鍵読み込み
 * - key_derivation(): ユーザ秘密鍵生成
 * - sign(): メッセージ署名
 * - verify(): 署名検証
 *
 * 【署名アルゴリズムの概要】
 * 1. ハッシュ関数 H1, H2, H3 を用いて ID やメッセージを G1 / Fr にマッピング
 * 2. マスター秘密鍵を使用してユーザ秘密鍵を生成
 * 3. sign() で r, x をランダム値として使用し署名成分 g1, g2, g3 を計算
 * 4. verify() でペアリング演算を使って署名検証
 *
 * 【注意点】
 * - MCLライブラリの BN 曲線上で計算を行う
 * - OpenSSL を使って SHA256 ハッシュを計算
 * - バッファサイズやオフセット (INDEX_*) に依存しているため、呼び出し側は注意
 */


void ecdsa_sig::printVector(const std::vector<uint8_t>& v)
{
    for (auto b : v) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << std::endl;
}

std::uint32_t ecdsa_sig::signature_size(){
    std::cerr<<"signature length:"<<std::to_string(mclBn_getG1ByteSize()*3+mclBn_getG2ByteSize())<<std::endl;
    return mclBn_getG1ByteSize()*3+mclBn_getG2ByteSize();
}

void ecdsa_sig::H1(mclBnG1 *g1, const std::uint8_t *msg, int length){
    mclBnG1_hashAndMapTo(g1, msg, length);
}
void ecdsa_sig::H2(mclBnG1 *g1, const std::uint8_t *msg, int length){
	std::uint8_t hash[SHA256_DIGEST_LENGTH];
	SHA256( msg, length, hash );
	mclBnG1_hashAndMapTo(g1,hash, SHA256_DIGEST_LENGTH);
}
void ecdsa_sig::H3(mclBnFr *fr, const std::uint8_t *msg, int length){
	uint8_t hash[SHA256_DIGEST_LENGTH];
	SHA256( msg, length, hash );
	mclBnFr_setLittleEndian(fr, hash, SHA256_DIGEST_LENGTH);
}

void ecdsa_sig::deserialize_keys(){
    std::uint8_t mpk_buf[]={212,217,103,133,97,152,213,175,150,16,220,127,104,121,118,36,160,215,27,178,62,16,10,40,211,210,2,9,229,31,238,129,120,68,90,79,34,189,25,139,155,20,138,100,222,192,240,147,136,131,70,202,199,17,111,10,123,86,23,198,163,2,26,120,96,135,175,25,53,213,213,161,107,13,172,173,45,116,100,34,73,156,253,255,212,174,67,199,96,7,255,130,166,8,227,10,103,228,230,99,111,113,221,237,91,32,102,118,208,253,135,50,182,187,7,118,50,69,214,81,235,144,58,17,208,111,249,84,131,203,45,18,216,136,155,77,177,150,23,141,209,186,20,144,178,220,28,226,115,200,1,183,123,145,159,54,83,126,2,150,234,123,132,195,207,241,170,45,104,34,213,28,67,12,177,192,200,65,1,2,233,98,160,187,212,241,42,203,128,114,238,24,28,28,50,154,92,71,220,27,67,174,112,2,4,59,145,171,19,223,9,9,198,68,109,125,23,44,42,95,144,27,91,87,162,215,206,66,214,215,13,74,207,25,157,205,215,193,199,3,215,19,253,246,29,149,112,239,22,129,169,133,190,9,109,20,83,122,121,102,211,98,253,223,87,233,184,160,227,216,26,183,196,85,221,216,152,136,164,171,31,152,91,198,147,133,62,15,188,55,169,62,234,198,213,244,138,206,94,196,71,237,18,188,142,254,17,129,51,44,39,126,2,185,158,194,15,240,125,248,76,196,94,159,75,88,255,157,174,68,254,176,100,100,108,144};
    std::uint8_t msk_buf[]={50,70,198,236,253,24,7,72,152,93,122,32,71,129,32,127,220,8,76,8,175,12,178,0,224,149,81,82,46,15,128,72,146,208,76,226,176,51,252,162,198,190,120,127,221,27,247,188,46,9,104,113,163,222,237,231,246,147,159,212,71,215,253,42};
    mclBnG1_deserialize(&(this->mpk.mpk1g1),&mpk_buf[INDEX_MPK_1G1],G1_LENGTH);
    mclBnG2_deserialize(&(this->mpk.mpk1g2),&mpk_buf[INDEX_MPK_1G2],G2_LENGTH);
    mclBnG2_deserialize(&(this->mpk.mpk2),&mpk_buf[INDEX_MPK_2G2],G2_LENGTH);
    mclBnG2_deserialize(&(this->mpk.mpk3),&mpk_buf[INDEX_MPK_3G2],G2_LENGTH);
	mclBnFr_deserialize(&(this->msk.msk1),&msk_buf[INDEX_MSK_1],FR_LENGTH);
	mclBnFr_deserialize(&(this->msk.msk2),&msk_buf[INDEX_MSK_2],FR_LENGTH);

}
void ecdsa_sig::serialize_sig(const signature *sig, std::uint8_t *buf){

	mclBnG1_serialize(&(buf[INDEX_SIG_1G1]),G1_LENGTH,&(sig->g1));
	mclBnG1_serialize(&(buf[INDEX_SIG_2G1]),G1_LENGTH,&(sig->g2));
	mclBnG1_serialize(&(buf[INDEX_SIG_3G1]),G1_LENGTH,&(sig->g3g1));
	mclBnG2_serialize(&(buf[INDEX_SIG_3G2]),G2_LENGTH,&(sig->g3g2));
}
void ecdsa_sig::deserialize_sig(signature *sig, const std::uint8_t *buf){
	mclBnG1_deserialize(&(sig->g1),&(buf[INDEX_SIG_1G1]),G1_LENGTH);
	mclBnG1_deserialize(&(sig->g2),&(buf[INDEX_SIG_2G1]),G1_LENGTH);
	mclBnG1_deserialize(&(sig->g3g1),&(buf[INDEX_SIG_3G1]),G1_LENGTH);
	mclBnG2_deserialize(&(sig->g3g2),&(buf[INDEX_SIG_3G2]),G2_LENGTH);
}

void ecdsa_sig::set_id(const std::vector<uint8_t> ID) {

    //引数のIDをコピー　
    this->id = ID;

    //ちゃんとコピーされているかを確認
    printVector(this->id);

}
    
void ecdsa_sig::setup(){
    std::cerr<<"ecdsa setup1"<<std::endl;
    std::cerr<<"id:["<<std::to_string(this->id[0]);
    for(int i=1;i<this->id.size();i++){
        std::cerr<<","<<std::to_string(this->id[i]);
    }
    std::cerr<<"]"<<std::endl;
	int ret = mclBn_init(MCL_BLS12_381, MCLBN_COMPILED_TIME_VAR);
    //std::cerr<<"ecdsa setup2"<<std::endl;
	if (ret != 0) {
		printf("err ret=%d\n", ret);
		return ;
	}

    //std::cerr<<"ecdsa setup3"<<std::endl;
	this->deserialize_keys();
}

void ecdsa_sig::key_derivation(){
	mclBnG1 tmp;
    this->H1(&tmp,this->id.data(),this->id.size());
    mclBnG1_mul(&(this->isk.isk1),&tmp,&(this->msk.msk1));
    this->H2(&tmp,this->id.data(),this->id.size());
    mclBnG1_mul(&(this->isk.isk2),&tmp,&(this->msk.msk2));
    std::cerr<<"finish key derivation"<<std::endl;
}

/**
 * 引数 経路情報 RIと集約署名 signed_sig
 */
std::vector<uint8_t> ecdsa_sig::sign(const std::vector<std::vector<uint8_t>>& RI, std::vector<uint8_t> signed_sig) {
    signature sig;

    // -----------------------------------------------------------
    // 経路情報の数で確認
    // -----------------------------------------------------------
    // RIが空なら何もできない
    if (RI.empty()) return {};

    // RIのサイズが1 = 自分が最初の署名者
    if (RI.size() == 1) {
        // 初期化
        mclBnG1_clear(&(sig.g1));
        mclBnG1_clear(&(sig.g2));
        mclBnG1_clear(&(sig.g3g1));
        mclBnG2_clear(&(sig.g3g2));
    }
    else {
        // 前の署名を復元
        // 注意: signed_sig が空でないかチェックが必要かもしれません
        this->deserialize_sig(&sig, signed_sig.data());
    }
    
    // 2. 署名計算用の係数設定 (テスト用固定値)
    mclBnFr x, r;
    mclBnFr_setInt(&x, 13);
    mclBnFr_setInt(&r, 7);
    // mclBnFr_setByCSPRNG(&x);
    // mclBnFr_setByCSPRNG(&r);

    mclBnG1 h3isk1;
    mclBnG1 rsig3;
    mclBnG1 xsig2p;
    mclBnG1 xg1;
    mclBnG2 xg2;
    mclBnG1 rg;

    // --- 計算処理 ---
    mclBnG1_mul(&rg, &(mpk.mpk1g1), &r);         // rg
    mclBnG1_add(&(sig.g2), &(sig.g2), &rg);      // newsig2 = rg + sig2

    mclBnG1_mul(&rsig3, &(sig.g3g1), &r);        // r * sig3
    mclBnG1_mul(&xsig2p, &(sig.g2), &x);         // tmp = sig2' * x

    // 3. ID || Message の結合バッファ作成
    //size_t msg_len = message.size();
    //size_t idmsglength = ADDR_SIZE + msg_len; 

    std::cout << "RI check : " << RI.size() << std::endl;

    

    // IDをコピー
    // 末尾が自分のID
    const auto& my_ID = RI.back();

    // 必要な合計サイズを計算
    size_t total_size = my_ID.size();
    // 自分以外の過去の履歴分を加算 (RI.size()-1 まで)
    for(size_t k = 0; k < RI.size() - 1; ++k) {
        total_size += RI[k].size();
    }

    std::vector<uint8_t> idmsg(total_size);
    size_t offset = 0;

    // (1) 先頭に自分のIDをコピー
    std::memcpy(idmsg.data() + offset, my_ID.data(), my_ID.size());
    offset += my_ID.size();

    // (2) 過去の経路履歴をコピー
    for (size_t k = 0; k < RI.size() - 1; ++k) {
        std::memcpy(idmsg.data() + offset, RI[k].data(), RI[k].size());
        offset += RI[k].size();
    }

    // --- デバッグ用出力コード開始 ---
    std::cout << "DEBUG: idmsg size = " << idmsg.size() << std::endl;
    std::cout << "DEBUG: idmsg data = ";
    for (const auto& byte : idmsg) {
        // 2桁の16進数で表示 (例: 0a 00 ff ...)
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << std::endl; // 10進数に戻して改行
    
    mclBnFr h3;
    std::cout << "id check : ";
    
    // ハッシュ計算 H3(ID || m)
    this->H3(&h3, idmsg.data(), idmsg.size());

    // --- 署名構成要素の更新 ---
    mclBnG1_mul(&h3isk1, &(this->isk.isk1), &h3); // sk1 * H3(ID||m)
    
    mclBnG1_add(&(sig.g1), &(sig.g1), &rsig3);
    mclBnG1_add(&(sig.g1), &(sig.g1), &xsig2p);
    mclBnG1_add(&(sig.g1), &(sig.g1), &(this->isk.isk2));
    mclBnG1_add(&(sig.g1), &(sig.g1), &h3isk1);

    mclBnG1_mul(&xg1, &(this->mpk.mpk1g1), &x);    // newsig3g1
    mclBnG2_mul(&xg2, &(this->mpk.mpk1g2), &x);    // newsig3g2
    mclBnG1_add(&(sig.g3g1), &(sig.g3g1), &xg1);   // newsig3g1 update
    mclBnG2_add(&(sig.g3g2), &(sig.g3g2), &xg2);   // newsig3g2 update

    
    // 署名サイズ分のvectorを確保
    std::vector<uint8_t> result_sig(this->signature_size());

    // バッファに書き込み
    serialize_sig(&sig, result_sig.data());

    // そのまま返す
    return result_sig;
}

/**
 * 引数　i番目のIDとそれにより署名されたmessageのリスト、署名(型は後から)
 */
bool ecdsa_sig::verify(const std::vector<std::vector<uint8_t>>& RI ,const std::vector<uint8_t> verified_sig){
	signature sig;
	deserialize_sig(&sig,verified_sig.data());

	mclBnGT t1,t2;
    mclBn_pairing(&t1, &(sig.g1),&(this->mpk.mpk1g2));
    mclBn_pairing(&t2, &(sig.g2),&(sig.g3g2));

    std::cerr<<"verify\n"<<std::endl;
    mclBnG1 t3,t4,t5,t7;
    mclBnFr t6;
    mclBnG1_clear(&t3);
    mclBnG1_clear(&t7);


    int i=0;
    std::uint32_t len;

    // ハッシュ計算用バッファ
    std::vector<uint8_t> buffer;

    //リストを走査してハッシュ値を累積
    for(int i=0; i< RI.size(); ++i) {

        // i番目のIDを取得 (これが現在の検証対象ID)
        const std::vector<uint8_t>& current_id_vec = RI.at(i);

        buffer.clear();
        //署名対象データの作成
        buffer.insert(buffer.end(), current_id_vec.begin(), current_id_vec.end());

        for(size_t k = 0; k < i; k++){
            buffer.insert(buffer.end(),RI[k].begin(),RI[k].end());
        }

            // ---検証 デバッグ用出力コード開始 ---
        std::cout << "DEBUG: idmsg size = " << buffer.size() << std::endl;
        std::cout << "DEBUG: idmsg data = ";
        for (const auto& byte : buffer) {
            // 2桁の16進数で表示 (例: 0a 00 ff ...)
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(byte) << " ";
        }
        std::cout << std::dec << std::endl; // 10進数に戻して改行


    
        // --- H2(ID) の計算 ---
        // t4 = H2(ID)
        this->H2(&t4, current_id_vec.data(), current_id_vec.size());
        // t3 += t4 (累積)
        mclBnG1_add(&t3, &t3, &t4);

        // --- H1(ID) の計算 ---
        // t5 = H1(ID)
        this->H1(&t5, current_id_vec.data(), current_id_vec.size());

        // --- H3(ID || Message) の計算 ---        
        this->H3(&t6, buffer.data(), buffer.size());

        // t5 = t5 * t6 ( = H1(ID) * H3(ID||Msg) )
        mclBnG1_mul(&t5, &t5, &t6);
        
        // t7 += t5 (累積)
        mclBnG1_add(&t7, &t7, &t5);
    }

    mclBnGT t8, t9;
    // e(t3, mpk.mpk3)
    mclBn_pairing(&t8, &t3, &(this->mpk.mpk3));
    // e(t7, mpk.mpk2)
    mclBn_pairing(&t9, &t7, &(this->mpk.mpk2));

    mclBnGT_mul(&t2, &t2, &t8);
    mclBnGT_mul(&t2, &t2, &t9);

    int ret = mclBnGT_isEqual(&t1, &t2);
    return (ret == 1);
}