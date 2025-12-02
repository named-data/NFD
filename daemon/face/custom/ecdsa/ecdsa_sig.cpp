#include "ecdsa_sig.hpp"
#include <openssl/sha.h>
#include <iostream>
#include <fstream>
#include <cstring>

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

using namespace oit::ist::nws::adhoc_routing;
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
    
void ecdsa_sig::setup(){
    std::cerr<<"ecdsa setup1"<<std::endl;
    std::cerr<<"id:["<<std::to_string(this->id[0]);
    for(int i=1;i<ADDR_SIZE;i++){
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
    this->H1(&tmp,this->id,ADDR_SIZE);
    mclBnG1_mul(&(this->isk.isk1),&tmp,&(this->msk.msk1));
    this->H2(&tmp,this->id,ADDR_SIZE);
    mclBnG1_mul(&(this->isk.isk2),&tmp,&(this->msk.msk2));
    std::cerr<<"finish key derivation"<<std::endl;
}

void ecdsa_sig::sign(std::uint8_t *buf){
	signature sig;
    if(buf[INDEX_RI_LENGTH]==1){
        std::cerr<<"ri is 1"<<std::endl;
        mclBnG1_clear(&(sig.g1));
		mclBnG1_clear(&(sig.g2));
		mclBnG1_clear(&(sig.g3g1));
		mclBnG2_clear(&(sig.g3g2));
    }
    else{
        this->deserialize_sig(&sig,&(buf[INDEX_RI+buf[INDEX_RI_LENGTH]*ADDR_SIZE]));
    }
	
    mclBnFr x,r;
    mclBnFr_setInt(&x, 13);
    mclBnFr_setInt(&r, 7);
    //mclBnFr_setByCSPRNG(&x);
    //mclBnFr_setByCSPRNG(&r);

    mclBnG1 h3isk1;
    mclBnG1 rsig3;
    mclBnG1 xsig2p;
    mclBnG1 xg1;
    mclBnG2 xg2;
    mclBnG1 rg;

    mclBnG1_mul(&rg, &(mpk.mpk1g1),&r);//rg
    mclBnG1_add(&(sig.g2),&(sig.g2),&rg);// newsig2=rg+sig2 sig2' is done

    mclBnG1_mul(&rsig3, &(sig.g3g1),&r);// r*sig3
    mclBnG1_mul(&xsig2p,&(sig.g2),&x);// tmp = sig2'*x

    int idmsglength=ADDR_SIZE+SEQUENCE_NO_SIZE+TYPE_SIZE+PACKET_LENGTH_SIZE+ADDR_SIZE+ADDR_SIZE+RI_LENGTH_SIZE+buf[INDEX_RI_LENGTH]*ADDR_SIZE;

    std::uint8_t idmsg[idmsglength];//id||msg
    std::memcpy(idmsg,this->id,ADDR_SIZE);//copy own id
    std::memcpy(&(idmsg[ADDR_SIZE]),buf,idmsglength-ADDR_SIZE);//copy dsr header to tmp array
    
    mclBnFr h3;
    std::cerr<<"signing"<<std::endl;
    this->H3(&h3,idmsg,idmsglength);// H3(ID || m)

    mclBnG1_mul(&h3isk1,&(this->isk.isk1),&h3);// sk1 * H3(ID||m), sk1:alpha1*H1(ID)
    mclBnG1_add(&(sig.g1),&(sig.g1), &rsig3);
    mclBnG1_add(&(sig.g1),&(sig.g1), &xsig2p);
    mclBnG1_add(&(sig.g1),&(sig.g1), &(this->isk.isk2));
    mclBnG1_add(&(sig.g1),&(sig.g1), &h3isk1);

    mclBnG1_mul(&xg1, &(this->mpk.mpk1g1),&x);// newsig3g1=xg
    mclBnG2_mul(&xg2, &(this->mpk.mpk1g2),&x);// newsig3g2=xg
    mclBnG1_add(&(sig.g3g1),&(sig.g3g1), &xg1);// newsig3g1=xg+sig3g1 sig3' is done
    mclBnG2_add(&(sig.g3g2),&(sig.g3g2), &xg2);// newsig3g2=xg+sig3g2 sig3' is done

	serialize_sig(&sig,&(buf[INDEX_RI+buf[INDEX_RI_LENGTH]*ADDR_SIZE]));
   
    //p.set_sig(tmp);
	return;
}

bool ecdsa_sig::verify(std::uint8_t *buf){
	signature sig;
	deserialize_sig(&sig,&(buf[INDEX_RI+buf[INDEX_RI_LENGTH]*ADDR_SIZE]));

	mclBnGT t1,t2;
    mclBn_pairing(&t1, &(sig.g1),&(this->mpk.mpk1g2));
    mclBn_pairing(&t2, &(sig.g2),&(sig.g3g2));

    std::uint32_t plen;
    std::memcpy(&plen,&(buf[INDEX_PACKET_LENGTH]),PACKET_LENGTH_SIZE);
    int header=plen-SIG_LENGTH;
    int idmsglength=ADDR_SIZE+header;
    std::uint8_t idmsg[idmsglength];
    std::memset(idmsg,0,idmsglength);
    std::memcpy(&(idmsg[ADDR_SIZE]),buf,header);

    std::cerr<<"verify\n"<<std::endl;
    mclBnG1 t3,t4,t5,t7;
    mclBnFr t6;
    mclBnG1_clear(&t3);
    mclBnG1_clear(&t7);

    int i=0;
    std::uint32_t len;

    do{
        len=plen-(ADDR_SIZE*i);
        std::memcpy(&(idmsg[ADDR_SIZE+INDEX_PACKET_LENGTH]),&len,PACKET_LENGTH_SIZE);
        idmsg[ADDR_SIZE+INDEX_RI_LENGTH]=buf[INDEX_RI_LENGTH]-i;
        this->H2(&t4,&(idmsg[ADDR_SIZE+INDEX_RI+(idmsg[ADDR_SIZE+INDEX_RI_LENGTH]-1)*ADDR_SIZE]),ADDR_SIZE);
        mclBnG1_add(&t3,&t3,&t4);
        this->H1(&t5,&(idmsg[ADDR_SIZE+INDEX_RI+(idmsg[ADDR_SIZE+INDEX_RI_LENGTH]-1)*ADDR_SIZE]),ADDR_SIZE);
        std::memcpy(idmsg,&(idmsg[ADDR_SIZE+INDEX_RI+(idmsg[ADDR_SIZE+INDEX_RI_LENGTH]-1)*ADDR_SIZE]),ADDR_SIZE);
        this->H3(&t6,idmsg,idmsglength-(i*ADDR_SIZE));
        mclBnG1_mul(&t5,&t5,&t6);
        mclBnG1_add(&t7,&t7,&t5);
        i++;
    }while(i<buf[INDEX_RI_LENGTH]);
    mclBnGT t8,t9;
    mclBn_pairing(&t8, &t3,&(mpk.mpk3));
    mclBn_pairing(&t9, &t7,&(mpk.mpk2));

    mclBnGT_mul(&t2,&t2,&t8);
    mclBnGT_mul(&t2,&t2,&t9);

    int ret=mclBnGT_isEqual(&t1,&t2);
    return (ret==1);
}
