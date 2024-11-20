//
// Created by lion on 11/19/24.
//

#ifndef CRYPTO_H
#define CRYPTO_H
#include <iostream>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <string>
class Crypto {
public:
    Crypto() {
        try {
            EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_DH, nullptr);
            if (!ctx || EVP_PKEY_paramgen_init(ctx) <= 0) {
                throw std::runtime_error("Failed to initialize DH paramgen");
            }
            if (EVP_PKEY_CTX_set_dh_nid(ctx, NID_ffdhe2048) <= 0) {
                throw std::runtime_error("Failed to set standard DH group");
            }
            if (EVP_PKEY_paramgen(ctx, &params) <= 0) {
                throw std::runtime_error("Failed to generate DH params");
            }
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_CTX *key_gen_ctx = EVP_PKEY_CTX_new(params, nullptr);
            if (!key_gen_ctx || EVP_PKEY_keygen_init(key_gen_ctx) <= 0) {
                throw std::runtime_error("Failed to initialize EVP_PKEY_CTX");
            }
            if (EVP_PKEY_keygen(key_gen_ctx, &local_key) <= 0) {
                throw std::runtime_error("Failed to generate key");
            }
            EVP_PKEY_CTX_free(key_gen_ctx);
            std::cout << "Crypto System Init Success." << std::endl;
            long len{};
            std::cout << get_key(len) << std::endl;
        } catch (std::runtime_error &e) {
            handleErrors(e);
        }
    }
    ~Crypto() {
        EVP_PKEY_free(local_key);
        EVP_PKEY_free(params);
    }
    std::string get_key(long &len) const {
        try {
            BIO *bio = BIO_new(BIO_s_mem());
            if (!PEM_write_bio_PUBKEY(bio, local_key)) {
                throw std::runtime_error("Failed to write pubkey");
            }
            char *pem_data = nullptr;
            len = BIO_get_mem_data(bio, &pem_data);
            std::string pem(pem_data, pem_data + len);
            BIO_free(bio);
            return pem;
        } catch (std::runtime_error &e) {
            handleErrors(e);
        }
        return {};
    }
    void set_key(const std::vector<unsigned char> &key) {
        try {
            BIO *bio = BIO_new_mem_buf(key.data(), static_cast<int>(key.size())); // pem_data 是接收到的 PEM 数据
            EVP_PKEY *pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
            if (!pkey) {
                throw std::runtime_error("Failed to read pubkey");
            }
            dh_compute_shared_secret(pkey);
            BIO_free(bio);
        } catch (std::runtime_error &e) {
            handleErrors(e);
        }
    }
    [[nodiscard]] std::string get_shared_secret() const { return shared_secret; }
    void AES_Encrypt(const std::string &plaintext, std::string &output) const {
        try {
            EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
            if (!ctx)
                throw std::runtime_error("Failed to initialize EVP_CIPHER_CTX");
            // 初始化加密
            if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                                        reinterpret_cast<const unsigned char *>(shared_secret.c_str()), fix_iv)) {
                throw std::runtime_error("Failed to initialize encryption");
                                        }
            // 加密数据
            unsigned char ciphertext_buf[1024]{};
            int len{};
            if (1 != EVP_EncryptUpdate(ctx, ciphertext_buf, &len,
                                       reinterpret_cast<const unsigned char *>(plaintext.c_str()),
                                       static_cast<int>(plaintext.size()))) {
                throw std::runtime_error("Failed to encrypt update");
                                       }
            int ciphertext_len = len;
            // 完成加密
            if (1 != EVP_EncryptFinal_ex(ctx, ciphertext_buf + len, &len)) {
                throw std::runtime_error("Failed to encrypt final ex");
            }
            ciphertext_len += len;
            output = base64_encode(std::string(reinterpret_cast<const char *>(ciphertext_buf), ciphertext_len));
            EVP_CIPHER_CTX_free(ctx);
        } catch (std::runtime_error &e) {
            handleErrors(e);
        }
    }
    void AES_Decrypt(const std::string &message, std::string &plaintext)const {
        try {
            std::string ciphertext{base64_decode(message)};
            EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
            if (!ctx)
                throw std::runtime_error("Failed to initialize EVP_CIPHER_CTX");
            // 初始化解密
            if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                                        reinterpret_cast<const unsigned char *>(shared_secret.c_str()), fix_iv)) {
                throw std::runtime_error("Failed to initialize encryption");
                                        }
            // 解密数据
            unsigned char plaintext_buf[1024];
            int len;
            if (1 != EVP_DecryptUpdate(ctx, plaintext_buf, &len,
                                       reinterpret_cast<const unsigned char *>(ciphertext.c_str()),
                                       static_cast<int>(ciphertext.size()))) {
                throw std::runtime_error("Failed to decrypt update");
                                       }
            int plaintext_len = len;
            // 完成解密
            if (1 != EVP_DecryptFinal_ex(ctx, plaintext_buf + len, &len)) {
                throw std::runtime_error("Failed to decrypt final ex");
            }
            plaintext_len += len;
            plaintext = std::string(reinterpret_cast<const char *>(plaintext_buf), plaintext_len);
            EVP_CIPHER_CTX_free(ctx);
        } catch (std::runtime_error &e) {
            handleErrors(e);
        }
    }


private:
    EVP_PKEY *local_key{};
    EVP_PKEY *params = nullptr;
    const unsigned char fix_iv[16]{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
    std::string shared_secret{};
    static void handleErrors(const std::runtime_error &e) {
        std::cerr << "Crypto Error: " << e.what() << std::endl;
        std::cerr << "Error Code: " << ERR_get_error() << std::endl;
        abort();
    }
    void dh_compute_shared_secret(EVP_PKEY *peer_key) {
        try {
            EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(local_key, nullptr);
            if (!ctx || EVP_PKEY_derive_init(ctx) <= 0) {
                throw std::runtime_error("Failed to initialize EVP_PKEY_CTX");
            }
            if (EVP_PKEY_derive_set_peer(ctx, peer_key) <= 0) {
                throw std::runtime_error("Failed to set derive peer");
            }
            EVP_PKEY_free(peer_key);
            // Determine shared secret length
            size_t secret_len = 0;
            if (EVP_PKEY_derive(ctx, nullptr, &secret_len) <= 0) {
                throw std::runtime_error("Failed to derive secret");
            }
            // Compute shared secret
            shared_secret.resize(secret_len);
            if (EVP_PKEY_derive(ctx, reinterpret_cast<unsigned char *>(shared_secret.data()), &secret_len) <= 0) {
                throw std::runtime_error("Failed to compute shared_secret");
            }
            EVP_PKEY_CTX_free(ctx);
            shared_secret.resize(secret_len);
        } catch (std::runtime_error &e) {
            handleErrors(e);
        }
    }
    static std::string base64_encode(const std::string &in) {
        // Base64 编码后的长度是原长度的 4/3，且每个编码块的长度为 4 的倍数
        size_t encode_len = 4 * ((in.size() + 2) / 3);  // 计算编码后长度

        std::vector<unsigned char> out(encode_len);

        // EVP_EncodeBlock 会返回编码后的字节数
        const int length = EVP_EncodeBlock(out.data(), reinterpret_cast<const unsigned char *>(in.c_str()), static_cast<int>(in.size()));

        return std::string{reinterpret_cast<char*>(out.data()), static_cast<unsigned int>(length)};
    }
    static std::string base64_decode(const std::string &in) {
        // EVP_DecodeBlock 解码后的最大长度是编码长度的 3/4
        size_t decode_len = in.size() * 3 / 4;

        std::vector<unsigned char> out(decode_len);

        // EVP_DecodeBlock 返回解码后的字节数
        const int length = EVP_DecodeBlock(out.data(), reinterpret_cast<const unsigned char*>(in.c_str()), static_cast<int>(in.size()));

        if (length < 0) {
            throw std::runtime_error("Base64 decode failed");
        }

        return std::string{reinterpret_cast<char*>(out.data()), static_cast<unsigned int>(length)};
    }
};
#endif //CRYPTO_H
