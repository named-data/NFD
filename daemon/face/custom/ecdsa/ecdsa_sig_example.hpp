#ifndef ECDSA_SIG_HPP
#define ECDSA_SIG_HPP

#include <mcl/bn_c384_256.h>
#include <string>

#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <vector>

#define ID_SIZE 4
#define RI_LENGTH_SIZE 1
#define INDEX_RI_LENGTH RI_LENGTH_SIZE+ID_SIZE
//#define ADDR_SIZE 6 //MACアドレスをIDとする場合こっちを使う

/**
 * @file ecdsa_sig.hpp
 * @brief IDベース ECDSA 署名スキームの定義
 *
 * このヘッダは、Ad-hoc ルーティング向けに実装された
 * IDベース ECDSA 署名スキーム `ecdsa_sig` クラスを定義する。
 *
 * 【主な役割】
 * - マスター鍵（公開鍵 / 秘密鍵）とユーザ秘密鍵を管理
 * - 署名生成および検証を行う
 * - 署名や鍵のシリアライズ / デシリアライズをサポート
 *
 * 【データ構造】
 * - master_pk: マスター公開鍵 (G1, G2 上の要素を保持)
 * - master_sk: マスター秘密鍵 (Fr 上の要素を保持)
 * - id_secret_key: ユーザ秘密鍵 (G1 上の要素を保持)
 * - signature: 署名 (G1, G2 上の要素を保持)
 *
 * 【クラス構造】
 * class ecdsa_sig : public signature_scheme
 * - setup(): 鍵ペアの生成
 * - key_derivation(): ユーザ秘密鍵生成
 * - sign(): メッセージ署名
 * - verify(): 署名検証
 * - H1, H2, H3: ハッシュ関数（G1 / Fr へのマッピング）
 * - serialize_sig / deserialize_sig: 署名のバイト列変換
 * - signature_scheme_name(): "ecdsa" を返す
 * - signature_size(): 署名サイズを返す
 *
 * 【利用方法】
 * 1. setup() でマスター鍵を生成
 * 2. key_derivation() でユーザ秘密鍵を生成
 * 3. sign() でメッセージに署名
 * 4. verify() で署名検証
 *
 * このクラスは mcl ライブラリを用いて BN 曲線 (BN254 など) 上で
 * 計算を行うことを前提としている。
 */

#define G1_LENGTH mclBn_getG1ByteSize()
#define G2_LENGTH mclBn_getG2ByteSize()
#define FR_LENGTH mclBn_getFrByteSize()

#define INDEX_MPK_1G1 0
#define INDEX_MPK_1G2 (INDEX_MPK_1G1+G1_LENGTH)
#define INDEX_MPK_2G2 (INDEX_MPK_1G2+G2_LENGTH)
#define INDEX_MPK_3G2 (INDEX_MPK_2G2+G2_LENGTH)
#define MPK_LENGTH (G1_LENGTH+G2_LENGTH*3)
typedef struct{
  mclBnG1 mpk1g1;
  mclBnG2 mpk1g2;
  mclBnG2 mpk2;
  mclBnG2 mpk3;
} master_pk;

#define INDEX_MSK_1 0
#define INDEX_MSK_2 (INDEX_MSK_1+FR_LENGTH)
#define MSK_LENGTH (FR_LENGTH*2())
typedef struct{
  mclBnFr msk1;
  mclBnFr msk2;
} master_sk;

#define INDEX_ISK_1 0
#define INDEX_ISK_2 (INDEX_ISK_1+G1_LENGTH)
#define ISK_LENGTH (G1_LENGTH*2)
typedef struct{
  mclBnG1 isk1;
  mclBnG1 isk2;
} id_secret_key;

#define INDEX_SIG_1G1 0
#define INDEX_SIG_2G1 (INDEX_SIG_1G1+G1_LENGTH)
#define INDEX_SIG_3G1 (INDEX_SIG_2G1+G1_LENGTH)
#define INDEX_SIG_3G2 (INDEX_SIG_3G1+G1_LENGTH)
#define SIG_LENGTH (G1_LENGTH*3+G2_LENGTH)
typedef struct{
  mclBnG1 g1;
  mclBnG1 g2;
  mclBnG1 g3g1;
  mclBnG2 g3g2;
} signature;

class ecdsa_sig{
    protected:
    master_pk mpk;
    master_sk msk;
    id_secret_key isk;
    std::vector<uint8_t> id;

    void H1(mclBnG1 *g1, const std::uint8_t *msg, int length);
    void H2(mclBnG1 *g1, const std::uint8_t *msg, int length);
    void H3(mclBnFr *fr, const std::uint8_t *msg, int length);
    void deserialize_keys();
    void serialize_sig(const signature *sig, std::uint8_t *buf);
    void deserialize_sig(signature *sig, const std::uint8_t *buf);
    public:
    ecdsa_sig(){}
    ~ecdsa_sig(){}

    //virtual void setup() override;
    //virtual void key_derivation() override;
    //virtual void sign(std::uint8_t *buf) override;
    //virtual bool verify(std::uint8_t *buf) override;
    //virtual std::string signature_scheme_name() override {return "ecdsa";};
    //virtual std::uint32_t signature_size() override;

    //追加部分
    std::uint32_t signature_size();
    void setup();
    void key_derivation();
    std::vector<uint8_t> sign(const std::vector<std::vector<uint8_t>>& RI, std::vector<uint8_t> signed_sig);
    std::vector<uint8_t> sign(const std::vector<uint8_t>& buf, std::vector<uint8_t> signed_sig);
    void sign(std::vector<uint8_t>& buf);
    bool verify(const std::vector<std::vector<uint8_t>>& RI ,const std::vector<uint8_t> verified_sig);
    bool verify(const std::vector<std::uint8_t>& buf, const std::vector<uint8_t> verified_sig);
    bool verify(const std::vector<std::uint8_t>& buf);
    // 【追加】任意のIDを設定する関数
    void set_id(const std::vector<uint8_t> ID);
    void printVector(const std::vector<uint8_t>& v);
};
#endif