#include "Cryptography.h"

#include <Windows.h>
#include <bcrypt.h>
#include <ntstatus.h>

#pragma comment(lib, "Bcrypt.lib")

#undef ZeroMemory

namespace Engine
{
	namespace Cryptography
	{
		class BCryptKey : public Key
		{
		public:
			SafePointer<Algorithm> algorithm;
			BCRYPT_KEY_HANDLE key_handle;
			PUCHAR key_data;

			virtual ~BCryptKey(void) override { BCryptDestroyKey(key_handle); free(key_data); }
			virtual DataBlock * ExportKey(void) override
			{
				ULONG blob_size;
				if (BCryptExportKey(key_handle, 0, BCRYPT_KEY_DATA_BLOB, 0, 0, &blob_size, 0) != STATUS_SUCCESS) return 0;
				auto buffer = reinterpret_cast<PUCHAR>(malloc(blob_size));
				if (!buffer) throw OutOfMemoryException();
				if (BCryptExportKey(key_handle, 0, BCRYPT_KEY_DATA_BLOB, buffer, blob_size, &blob_size, 0) != STATUS_SUCCESS) { free(buffer); return 0; }
				SafePointer<DataBlock> result;
				try {
					result = new DataBlock(blob_size - sizeof(BCRYPT_KEY_DATA_BLOB_HEADER));
					result->SetLength(blob_size - sizeof(BCRYPT_KEY_DATA_BLOB_HEADER));
					MemoryCopy(result->GetBuffer(), buffer + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER), blob_size - sizeof(BCRYPT_KEY_DATA_BLOB_HEADER));
				} catch (...) { free(buffer); throw; }
				free(buffer);
				result->Retain();
				return result;
			}
			virtual Algorithm * GetAlgorithm(void) const override { return algorithm; }
			virtual DataBlock * EncryptData(const void * data, int length, const void * iv) override
			{
				auto block = algorithm->EncryptionBlockSize();
				DataBlock local_iv(block);
				local_iv.SetLength(block);
				if (iv) MemoryCopy(local_iv.GetBuffer(), iv, block); else ZeroMemory(local_iv.GetBuffer(), block);
				auto data_ptr = reinterpret_cast<PUCHAR>(const_cast<void *>(data));
				ULONG result_length;
				if (BCryptEncrypt(key_handle, data_ptr, length, 0, reinterpret_cast<PUCHAR>(local_iv.GetBuffer()), block, 0, 0, &result_length, BCRYPT_BLOCK_PADDING) != STATUS_SUCCESS) return 0;
				SafePointer<DataBlock> result = new DataBlock(result_length);
				result->SetLength(result_length);
				if (BCryptEncrypt(key_handle, data_ptr, length, 0, reinterpret_cast<PUCHAR>(local_iv.GetBuffer()), block, result->GetBuffer(), result->Length(), &result_length, BCRYPT_BLOCK_PADDING) != STATUS_SUCCESS) return 0;
				result->Retain();
				return result;
			}
			virtual DataBlock * DecryptData(const void * data, int length, const void * iv) override
			{
				auto block = algorithm->EncryptionBlockSize();
				DataBlock local_iv(block);
				local_iv.SetLength(block);
				if (iv) MemoryCopy(local_iv.GetBuffer(), iv, block); else ZeroMemory(local_iv.GetBuffer(), block);
				auto data_ptr = reinterpret_cast<PUCHAR>(const_cast<void *>(data));
				ULONG result_length;
				if (BCryptDecrypt(key_handle, data_ptr, length, 0, reinterpret_cast<PUCHAR>(local_iv.GetBuffer()), block, 0, 0, &result_length, BCRYPT_BLOCK_PADDING) != STATUS_SUCCESS) return 0;
				SafePointer<DataBlock> result = new DataBlock(result_length);
				result->SetLength(result_length);
				if (BCryptDecrypt(key_handle, data_ptr, length, 0, reinterpret_cast<PUCHAR>(local_iv.GetBuffer()), block, result->GetBuffer(), result->Length(), &result_length, BCRYPT_BLOCK_PADDING) != STATUS_SUCCESS) return 0;
				result->SetLength(result_length);
				result->Retain();
				return result;
			}
		};
		class BCryptAlgorithm : public Algorithm
		{
		public:
			EncryptionAlgorithm identifier;
			BCRYPT_ALG_HANDLE algorithm;
			ULONG block_size;
			ULONG object_size;

			virtual ~BCryptAlgorithm(void) override { BCryptCloseAlgorithmProvider(algorithm, 0); }
			virtual EncryptionAlgorithm Identifier(void) const override { return identifier; }
			virtual int EncryptionBlockSize(void) const override { return block_size; }
			virtual EncryptionMode GetEncryptionMode(void) const override
			{
				ULONG mode_size;
				PUCHAR mode_name;
				if (BCryptGetProperty(algorithm, BCRYPT_CHAINING_MODE, 0, 0, &mode_size, 0) != STATUS_SUCCESS) return EncryptionMode::Unknown;
				mode_name = reinterpret_cast<PUCHAR>(malloc(mode_size));
				if (!mode_name) return EncryptionMode::Unknown;
				if (BCryptGetProperty(algorithm, BCRYPT_CHAINING_MODE, mode_name, mode_size, &mode_size, 0) != STATUS_SUCCESS) { free(mode_name); return EncryptionMode::Unknown; }
				auto result = EncryptionMode::Unknown;
				if (StringCompare(reinterpret_cast<const widechar *>(mode_name), BCRYPT_CHAIN_MODE_ECB) == 0) result = EncryptionMode::ECB;
				else if (StringCompare(reinterpret_cast<const widechar *>(mode_name), BCRYPT_CHAIN_MODE_CBC) == 0) result = EncryptionMode::CBC;
				else if (StringCompare(reinterpret_cast<const widechar *>(mode_name), BCRYPT_CHAIN_MODE_CFB) == 0) result = EncryptionMode::CFB;
				free(mode_name);
				return result;
			}
			virtual void SetEncryptionMode(EncryptionMode mode) override
			{
				LPWSTR mode_name;
				if (mode == EncryptionMode::ECB) mode_name = BCRYPT_CHAIN_MODE_ECB;
				else if (mode == EncryptionMode::CBC) mode_name = BCRYPT_CHAIN_MODE_CBC;
				else if (mode == EncryptionMode::CFB) mode_name = BCRYPT_CHAIN_MODE_CFB;
				else throw InvalidArgumentException();
				BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE, reinterpret_cast<PUCHAR>(mode_name), StringLength(mode_name) * 2 + 2, 0);
			}
			virtual Key * ImportKey(const void * data, int length) override
			{
				DataBlock blob(length + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER));
				blob.SetLength(length + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER));
				MemoryCopy(blob.GetBuffer() + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER), data, length);
				BCRYPT_KEY_DATA_BLOB_HEADER * hdr = reinterpret_cast<BCRYPT_KEY_DATA_BLOB_HEADER *>(blob.GetBuffer());
				hdr->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
				hdr->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
				hdr->cbKeyData = length;
				BCRYPT_KEY_HANDLE key;
				PUCHAR key_data = reinterpret_cast<PUCHAR>(malloc(object_size));
				if (!key_data) throw OutOfMemoryException();
				if (BCryptImportKey(algorithm, 0, BCRYPT_KEY_DATA_BLOB, &key, key_data, object_size, blob.GetBuffer(), blob.Length(), 0) != STATUS_SUCCESS) { free(key_data); return 0; }
				SafePointer<BCryptKey> key_obj = new (std::nothrow) BCryptKey;
				if (!key_obj) { free(key_data); BCryptDestroyKey(key); throw OutOfMemoryException(); }
				key_obj->key_handle = key;
				key_obj->key_data = key_data;
				key_obj->algorithm.SetRetain(this);
				key_obj->Retain();
				return key_obj;
			}
			virtual Key * GenerateKey(const void * secret, int length) override
			{
				auto secret_ptr = reinterpret_cast<PUCHAR>(const_cast<void *>(secret));
				BCRYPT_KEY_HANDLE key;
				PUCHAR key_data = reinterpret_cast<PUCHAR>(malloc(object_size));
				if (!key_data) throw OutOfMemoryException();
				if (BCryptGenerateSymmetricKey(algorithm, &key, key_data, object_size, secret_ptr, length, 0) != STATUS_SUCCESS) { free(key_data); return 0; }
				SafePointer<BCryptKey> key_obj = new (std::nothrow) BCryptKey;
				if (!key_obj) { free(key_data); BCryptDestroyKey(key); throw OutOfMemoryException(); }
				key_obj->key_handle = key;
				key_obj->key_data = key_data;
				key_obj->algorithm.SetRetain(this);
				key_obj->Retain();
				return key_obj;
			}
		};

		Key * Algorithm::ImportKey(const DataBlock * data) { return ImportKey(data->GetBuffer(), data->Length()); }
		Key * Algorithm::GenerateKey(const DataBlock * data) { return GenerateKey(data->GetBuffer(), data->Length()); }
		DataBlock * Key::EncryptData(const DataBlock * data, const void * iv) { return EncryptData(data->GetBuffer(), data->Length(), iv); }
		DataBlock * Key::DecryptData(const DataBlock * data, const void * iv) { return DecryptData(data->GetBuffer(), data->Length(), iv); }

		Algorithm * OpenEncryptionAlgorithm(EncryptionAlgorithm algorithm)
		{
			LPCWSTR algorithm_name;
			if (algorithm == EncryptionAlgorithm::AES) algorithm_name = BCRYPT_AES_ALGORITHM;
			else if (algorithm == EncryptionAlgorithm::DES) algorithm_name = BCRYPT_DES_ALGORITHM;
			else if (algorithm == EncryptionAlgorithm::TripleDES) algorithm_name = BCRYPT_3DES_ALGORITHM;
			else if (algorithm == EncryptionAlgorithm::RC2) algorithm_name = BCRYPT_RC2_ALGORITHM;
			else if (algorithm == EncryptionAlgorithm::RC4) algorithm_name = BCRYPT_RC4_ALGORITHM;
			else return 0;
			BCRYPT_ALG_HANDLE handle;
			if (BCryptOpenAlgorithmProvider(&handle, algorithm_name, 0, 0) != STATUS_SUCCESS) return 0;
			SafePointer<BCryptAlgorithm> result = new (std::nothrow) BCryptAlgorithm;
			if (!result) { BCryptCloseAlgorithmProvider(handle, 0); return 0; }
			result->algorithm = handle;
			result->block_size = result->object_size = 0;
			result->identifier = algorithm;
			ULONG prop_size;
			if (BCryptGetProperty(handle, BCRYPT_BLOCK_LENGTH, reinterpret_cast<PUCHAR>(&result->block_size), sizeof(result->block_size), &prop_size, 0) != STATUS_SUCCESS) { return 0; }
			if (BCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&result->object_size), sizeof(result->object_size), &prop_size, 0) != STATUS_SUCCESS) { return 0; }
			result->SetEncryptionMode(EncryptionMode::CBC);
			result->Retain();
			return result;
		}
		DataBlock * CreateHash(HashAlgorithm algorithm, const void * data, int length)
		{
			LPCWSTR alg_name;
			if (algorithm == HashAlgorithm::MD5) alg_name = BCRYPT_MD5_ALGORITHM;
			else if (algorithm == HashAlgorithm::SHA1) alg_name = BCRYPT_SHA1_ALGORITHM;
			else if (algorithm == HashAlgorithm::SHA256) alg_name = BCRYPT_SHA256_ALGORITHM;
			else if (algorithm == HashAlgorithm::SHA384) alg_name = BCRYPT_SHA384_ALGORITHM;
			else if (algorithm == HashAlgorithm::SHA512) alg_name = BCRYPT_SHA512_ALGORITHM;
			else return 0;
			BCRYPT_ALG_HANDLE alg;
			BCRYPT_HASH_HANDLE hash;
			if (BCryptOpenAlgorithmProvider(&alg, alg_name, 0, 0) != STATUS_SUCCESS) return 0;
			ULONG object_size = 0;
			ULONG hash_size = 0;
			ULONG size_size;
			if (BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_size), sizeof(object_size), &size_size, 0) != STATUS_SUCCESS) { BCryptCloseAlgorithmProvider(alg, 0); return 0; }
			if (BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hash_size), sizeof(hash_size), &size_size, 0) != STATUS_SUCCESS) { BCryptCloseAlgorithmProvider(alg, 0); return 0; }
			auto object = malloc(object_size);
			if (!object) { BCryptCloseAlgorithmProvider(alg, 0); return 0; }
			if (BCryptCreateHash(alg, &hash, reinterpret_cast<PUCHAR>(object), object_size, 0, 0, 0) != STATUS_SUCCESS) { free(object); BCryptCloseAlgorithmProvider(alg, 0); return 0; }
			if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<void *>(data)), length, 0) != STATUS_SUCCESS) {
				BCryptDestroyHash(hash);
				free(object);
				BCryptCloseAlgorithmProvider(alg, 0);
				return 0;
			}
			SafePointer<DataBlock> result;
			try {
				result = new DataBlock(hash_size);
				result->SetLength(hash_size);
				if (BCryptFinishHash(hash, reinterpret_cast<PUCHAR>(result->GetBuffer()), result->Length(), 0) != STATUS_SUCCESS) throw Exception();
			} catch (...) {
				BCryptDestroyHash(hash);
				free(object);
				BCryptCloseAlgorithmProvider(alg, 0);
				return 0;
			}
			BCryptDestroyHash(hash);
			free(object);
			BCryptCloseAlgorithmProvider(alg, 0);
			result->Retain();
			return result;
		}
		DataBlock * CreateHash(HashAlgorithm algorithm, const DataBlock * data) { return CreateHash(algorithm, data->GetBuffer(), data->Length()); }
		DataBlock * CreateSecureRandom(int length)
		{
			SafePointer<DataBlock> result = new DataBlock(length);
			result->SetLength(length);
			BCRYPT_ALG_HANDLE alg;
			if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_RNG_ALGORITHM, 0, 0) != STATUS_SUCCESS) return 0;
			if (BCryptGenRandom(alg, reinterpret_cast<PUCHAR>(result->GetBuffer()), result->Length(), 0) != STATUS_SUCCESS) { BCryptCloseAlgorithmProvider(alg, 0); return 0; }
			BCryptCloseAlgorithmProvider(alg, 0);
			result->Retain();
			return result;
		}
	}
}