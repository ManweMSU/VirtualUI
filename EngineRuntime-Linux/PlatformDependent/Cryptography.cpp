#include "../Interfaces/Cryptography.h"

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

namespace Engine
{
	namespace Cryptography
	{
		class SystemEncryptionKeyAES : public Key
		{
			uint8 _key_data[32];
			const EVP_CIPHER * _cipher;
			SafePointer<Algorithm> _alg;
		public:
			SystemEncryptionKeyAES(Algorithm * alg, const void * key_data, const EVP_CIPHER * cipher) : _cipher(cipher) { _alg.SetRetain(alg); MemoryCopy(&_key_data, key_data, 32); }
			virtual ~SystemEncryptionKeyAES(void) override {}
			virtual DataBlock * ExportKey(void) override
			{
				try {
					SafePointer<DataBlock> result = new DataBlock(32);
					result->SetLength(32);
					MemoryCopy(result->GetBuffer(), &_key_data, 32);
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual Algorithm * GetAlgorithm(void) const override { return _alg; }
			virtual DataBlock * EncryptData(const void * data, int length, const void * iv) override
			{
				SafePointer<DataBlock> result;
				auto ctx = EVP_CIPHER_CTX_new();
				if (!ctx) return 0;
				if (EVP_CIPHER_CTX_init(ctx)) {
					if (EVP_EncryptInit_ex(ctx, _cipher, 0, _key_data, reinterpret_cast<const uint8 *>(iv))) {
						try {
							result = new DataBlock(1);
							result->SetLength(length + _alg->EncryptionBlockSize());
							int bytes_used = 0, bytes_final = 0;
							if (!EVP_EncryptUpdate(ctx, result->GetBuffer(), &bytes_used, reinterpret_cast<const uint8 *>(data), length)) throw Exception();
							if (!EVP_EncryptFinal_ex(ctx, result->GetBuffer() + bytes_used, &bytes_final)) throw Exception();
							bytes_used += bytes_final;
							result->SetLength(bytes_used);
						} catch (...) { result = 0; }
					}
				}
				EVP_CIPHER_CTX_free(ctx);
				if (result) {
					result->Retain();
					return result;
				} else return 0;
			}
			virtual DataBlock * DecryptData(const void * data, int length, const void * iv) override
			{
				SafePointer<DataBlock> result;
				auto ctx = EVP_CIPHER_CTX_new();
				if (!ctx) return 0;
				if (EVP_CIPHER_CTX_init(ctx)) {
					if (EVP_DecryptInit_ex(ctx, _cipher, 0, _key_data, reinterpret_cast<const uint8 *>(iv))) {
						try {
							result = new DataBlock(1);
							result->SetLength(length + _alg->EncryptionBlockSize());
							int bytes_used = 0, bytes_final = 0;
							if (!EVP_DecryptUpdate(ctx, result->GetBuffer(), &bytes_used, reinterpret_cast<const uint8 *>(data), length)) throw Exception();
							if (!EVP_DecryptFinal_ex(ctx, result->GetBuffer() + bytes_used, &bytes_final)) throw Exception();
							bytes_used += bytes_final;
							result->SetLength(bytes_used);
						} catch (...) { result = 0; }
					}
				}
				EVP_CIPHER_CTX_free(ctx);
				if (result) {
					result->Retain();
					return result;
				} else return 0;
			}
		};
		class SystemEncryptionAlgorithm : public Algorithm
		{
			EncryptionAlgorithm _algorithm;
			EncryptionMode _mode;
		public:
			SystemEncryptionAlgorithm(EncryptionAlgorithm algorithm) : _algorithm(algorithm), _mode(EncryptionMode::CBC) { if (algorithm != EncryptionAlgorithm::AES) throw Exception(); }
			virtual ~SystemEncryptionAlgorithm(void) override {}
			virtual EncryptionAlgorithm Identifier(void) const override { return _algorithm; }
			virtual int EncryptionBlockSize(void) const override
			{
				if (_algorithm == EncryptionAlgorithm::AES) return 16;
				else return 0;
			}
			virtual EncryptionMode GetEncryptionMode(void) const override { return _mode; }
			virtual void SetEncryptionMode(EncryptionMode mode) override { _mode = mode; }
			virtual Key * ImportKey(const void * data, int length) override
			{
				if (_algorithm == EncryptionAlgorithm::AES) {
					if (length != 32) return 0;
					try {
						if (_mode == EncryptionMode::ECB) return new SystemEncryptionKeyAES(this, data, EVP_aes_256_ecb());
						else if (_mode == EncryptionMode::CBC) return new SystemEncryptionKeyAES(this, data, EVP_aes_256_cbc());
						else if (_mode == EncryptionMode::CFB) return new SystemEncryptionKeyAES(this, data, EVP_aes_256_cfb());
						else return 0;
					} catch (...) { return 0; }
				} else return 0;
			}
			virtual Key * GenerateKey(const void * secret, int length) override
			{
				if (length < 0) return 0;
				if (_algorithm == EncryptionAlgorithm::AES) {
					uint8 data[32];
					ZeroMemory(&data, 32);
					MemoryCopy(&data, secret, min(length, 32));
					return ImportKey(&data, 32);
				} else return 0;
			}
		};

		Key * Algorithm::ImportKey(const DataBlock * data) { return ImportKey(data->GetBuffer(), data->Length()); }
		Key * Algorithm::GenerateKey(const DataBlock * data) { return GenerateKey(data->GetBuffer(), data->Length()); }
		DataBlock * Key::EncryptData(const DataBlock * data, const void * iv) { return EncryptData(data->GetBuffer(), data->Length(), iv); }
		DataBlock * Key::DecryptData(const DataBlock * data, const void * iv) { return DecryptData(data->GetBuffer(), data->Length(), iv); }

		Algorithm * OpenEncryptionAlgorithm(EncryptionAlgorithm algorithm) { try { return new SystemEncryptionAlgorithm(algorithm); } catch (...) { return 0; } }
		DataBlock * CreateHash(HashAlgorithm algorithm, const void * data, int length)
		{
			EVP_MD_CTX * ctx = EVP_MD_CTX_create();
			if (!ctx) return 0;
			const EVP_MD * method = 0;
			if (algorithm == HashAlgorithm::MD5) method = EVP_md5();
			else if (algorithm == HashAlgorithm::SHA1) method = EVP_sha1();
			else if (algorithm == HashAlgorithm::SHA256) method = EVP_sha256();
			else if (algorithm == HashAlgorithm::SHA384) method = EVP_sha384();
			else if (algorithm == HashAlgorithm::SHA512) method = EVP_sha512();
			if (method && !EVP_DigestInit_ex(ctx, method, 0)) method = 0;
			if (!method) { EVP_MD_CTX_destroy(ctx); return 0; }
			if (!EVP_DigestUpdate(ctx, data, length)) { EVP_MD_CTX_destroy(ctx); return 0; }
			auto size = EVP_MD_CTX_size(ctx);
			try {
				if (!size) throw Exception();
				SafePointer<DataBlock> result = new DataBlock(size);
				result->SetLength(size);
				if (!EVP_DigestFinal_ex(ctx, result->GetBuffer(), 0)) throw Exception();
				result->Retain();
				return result;
			} catch (...) { EVP_MD_CTX_destroy(ctx); return 0; }
		}
		DataBlock * CreateHash(HashAlgorithm algorithm, const DataBlock * data) { return CreateHash(algorithm, data->GetBuffer(), data->Length()); }
		DataBlock * CreateSecureRandom(int length)
		{
			SafePointer<DataBlock> result;
			try {
				result = new DataBlock(1);
				result->SetLength(length);
			} catch (...) { return 0; }
			auto error = RAND_bytes(result->GetBuffer(), result->Length());
			if (error == 1) {
				result->Retain();
				return result;
			} else return 0;
		}
	}
}