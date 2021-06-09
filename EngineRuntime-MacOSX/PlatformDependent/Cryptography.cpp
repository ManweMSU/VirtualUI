#include "../Interfaces/Cryptography.h"

#include "Security/Security.h"
#include "Security/SecEncryptTransform.h"
#include "Security/SecDigestTransform.h"

namespace Engine
{
	namespace Cryptography
	{
		class SecKey : public Key
		{
		public:
			SafePointer<DataBlock> raw;
			SafePointer<Algorithm> algorithm;
			SecKeyRef key;

			virtual ~SecKey(void) override { CFRelease(key); }
			virtual DataBlock * ExportKey(void) override { return new DataBlock(*raw); }
			virtual Algorithm * GetAlgorithm(void) const override { return algorithm; }
			virtual DataBlock * EncryptData(const void * data, int length, const void * iv) override
			{
				auto block = algorithm->EncryptionBlockSize();
				DataBlock local_iv(block);
				local_iv.SetLength(block);
				if (iv) MemoryCopy(local_iv.GetBuffer(), iv, block); else ZeroMemory(local_iv.GetBuffer(), block);
				CFErrorRef error;
				SecTransformRef transform = SecEncryptTransformCreate(key, &error);
				if (!transform) { CFRelease(error); return 0; }
				CFDataRef data_ref = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const uint8 *>(data), length, kCFAllocatorNull);
				if (!data_ref) { CFRelease(transform); return 0; }
				CFDataRef iv_ref = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, local_iv.GetBuffer(), block, kCFAllocatorNull);
				if (!iv_ref) { CFRelease(transform); CFRelease(data_ref); return 0; }
				SecTransformSetAttribute(transform, kSecTransformInputAttributeName, data_ref, &error);
				if (error) { CFRelease(error); CFRelease(transform); CFRelease(data_ref); CFRelease(iv_ref); return 0; }
				if (algorithm->GetEncryptionMode() == EncryptionMode::CBC) SecTransformSetAttribute(transform, kSecEncryptionMode, kSecModeCBCKey, &error);
				else if (algorithm->GetEncryptionMode() == EncryptionMode::CFB) SecTransformSetAttribute(transform, kSecEncryptionMode, kSecModeCFBKey, &error);
				else SecTransformSetAttribute(transform, kSecEncryptionMode, kSecModeECBKey, &error);
				if (error) { CFRelease(error); CFRelease(transform); CFRelease(data_ref); CFRelease(iv_ref); return 0; }
				SecTransformSetAttribute(transform, kSecIVKey, iv_ref, &error);
				if (error) { CFRelease(error); CFRelease(transform); CFRelease(data_ref); CFRelease(iv_ref); return 0; }
				SecTransformSetAttribute(transform, kSecPaddingKey, kSecPaddingPKCS7Key, &error);
				if (error) { CFRelease(error); CFRelease(transform); CFRelease(data_ref); CFRelease(iv_ref); return 0; }
				CFDataRef output_ref = reinterpret_cast<CFDataRef>(SecTransformExecute(transform, &error));
				CFRelease(data_ref);
				CFRelease(iv_ref);
				if (!output_ref) { CFRelease(error); CFRelease(transform); return 0; }
				CFRelease(transform);
				SafePointer<DataBlock> result;
				try {
					result = new DataBlock(CFDataGetLength(output_ref));
					result->SetLength(CFDataGetLength(output_ref));
					MemoryCopy(result->GetBuffer(), CFDataGetBytePtr(output_ref), result->Length());
				} catch (...) { CFRelease(output_ref); return 0; }
				CFRelease(output_ref);
				result->Retain();
				return result;
			}
			virtual DataBlock * DecryptData(const void * data, int length, const void * iv) override
			{
				auto block = algorithm->EncryptionBlockSize();
				DataBlock local_iv(block);
				local_iv.SetLength(block);
				if (iv) MemoryCopy(local_iv.GetBuffer(), iv, block); else ZeroMemory(local_iv.GetBuffer(), block);
				CFErrorRef error;
				SecTransformRef transform = SecDecryptTransformCreate(key, &error);
				if (!transform) { CFRelease(error); return 0; }
				CFDataRef data_ref = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const uint8 *>(data), length, kCFAllocatorNull);
				if (!data_ref) { CFRelease(transform); return 0; }
				CFDataRef iv_ref = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, local_iv.GetBuffer(), block, kCFAllocatorNull);
				if (!iv_ref) { CFRelease(transform); CFRelease(data_ref); return 0; }
				SecTransformSetAttribute(transform, kSecTransformInputAttributeName, data_ref, &error);
				if (error) { CFRelease(error); CFRelease(transform); CFRelease(data_ref); CFRelease(iv_ref); return 0; }
				if (algorithm->GetEncryptionMode() == EncryptionMode::CBC) SecTransformSetAttribute(transform, kSecEncryptionMode, kSecModeCBCKey, &error);
				else if (algorithm->GetEncryptionMode() == EncryptionMode::CFB) SecTransformSetAttribute(transform, kSecEncryptionMode, kSecModeCFBKey, &error);
				else SecTransformSetAttribute(transform, kSecEncryptionMode, kSecModeECBKey, &error);
				if (error) { CFRelease(error); CFRelease(transform); CFRelease(data_ref); CFRelease(iv_ref); return 0; }
				SecTransformSetAttribute(transform, kSecIVKey, iv_ref, &error);
				if (error) { CFRelease(error); CFRelease(transform); CFRelease(data_ref); CFRelease(iv_ref); return 0; }
				SecTransformSetAttribute(transform, kSecPaddingKey, kSecPaddingPKCS7Key, &error);
				if (error) { CFRelease(error); CFRelease(transform); CFRelease(data_ref); CFRelease(iv_ref); return 0; }
				CFDataRef output_ref = reinterpret_cast<CFDataRef>(SecTransformExecute(transform, &error));
				CFRelease(data_ref);
				CFRelease(iv_ref);
				if (!output_ref) { CFRelease(error); CFRelease(transform); return 0; }
				CFRelease(transform);
				SafePointer<DataBlock> result;
				try {
					result = new DataBlock(CFDataGetLength(output_ref));
					result->SetLength(CFDataGetLength(output_ref));
					MemoryCopy(result->GetBuffer(), CFDataGetBytePtr(output_ref), result->Length());
				} catch (...) { CFRelease(output_ref); return 0; }
				CFRelease(output_ref);
				result->Retain();
				return result;
			}
		};
		class SecAlgorithm : public Algorithm
		{
		public:
			EncryptionAlgorithm identifier;
			EncryptionMode mode;
			CFStringRef algorithm;
			size_t block_size;
			size_t key_size;

			virtual ~SecAlgorithm(void) override {}
			virtual EncryptionAlgorithm Identifier(void) const override { return identifier; }
			virtual int EncryptionBlockSize(void) const override { return block_size; }
			virtual EncryptionMode GetEncryptionMode(void) const override { return mode; }
			virtual void SetEncryptionMode(EncryptionMode _mode) override { mode = _mode; }
			virtual Key * ImportKey(const void * data, int length) override { return GenerateKey(data, length); }
			virtual Key * GenerateKey(const void * secret, int length) override
			{
				auto secret_ref = reinterpret_cast<const uint8 *>(secret);
				SafePointer<DataBlock> raw_key = new DataBlock(key_size);
				for (int i = 0; i < min(int(key_size), length); i++) raw_key->Append(secret_ref[i]);
				while (raw_key->Length() < key_size) raw_key->Append(0);
				CFErrorRef error;
				CFDataRef data_ref = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, raw_key->GetBuffer(), raw_key->Length(), kCFAllocatorNull);
				if (!data_ref) return 0;
				CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				if (!dict) { CFRelease(data_ref); return 0; }
				CFDictionaryAddValue(dict, kSecAttrKeyType, algorithm);
				SecKeyRef key = SecKeyCreateFromData(dict, data_ref, &error);
				CFRelease(dict);
				CFRelease(data_ref);
				if (!key) { CFRelease(error); return 0; }
				SafePointer<SecKey> result = new (std::nothrow) SecKey;
				if (!result) { CFRelease(key); return 0; }
				result->key = key;
				result->algorithm.SetRetain(this);
				result->raw.SetRetain(raw_key);
				result->Retain();
				return result;
			}
		};

		Key * Algorithm::ImportKey(const DataBlock * data) { return ImportKey(data->GetBuffer(), data->Length()); }
		Key * Algorithm::GenerateKey(const DataBlock * data) { return GenerateKey(data->GetBuffer(), data->Length()); }
		DataBlock * Key::EncryptData(const DataBlock * data, const void * iv) { return EncryptData(data->GetBuffer(), data->Length(), iv); }
		DataBlock * Key::DecryptData(const DataBlock * data, const void * iv) { return DecryptData(data->GetBuffer(), data->Length(), iv); }

		Algorithm * OpenEncryptionAlgorithm(EncryptionAlgorithm algorithm)
		{
			CFStringRef algorithm_name;
			if (algorithm == EncryptionAlgorithm::AES) algorithm_name = kSecAttrKeyTypeAES;
			else return 0;
			SafePointer<SecAlgorithm> result = new (std::nothrow) SecAlgorithm;
			if (!result) throw OutOfMemoryException();
			result->algorithm = algorithm_name;
			result->block_size = 0;
			result->identifier = algorithm;
			result->mode = EncryptionMode::CBC;
			result->block_size = 16;
			result->key_size = 32;
			result->Retain();
			return result;
		}
		DataBlock * CreateHash(HashAlgorithm algorithm, const void * data, int length)
		{
			CFIndex hash_length;
			CFTypeRef hash_class;
			if (algorithm == HashAlgorithm::MD5) { hash_class = kSecDigestMD5; hash_length = 0; }
			else if (algorithm == HashAlgorithm::SHA1) { hash_class = kSecDigestSHA1; hash_length = 0; }
			else if (algorithm == HashAlgorithm::SHA256) { hash_class = kSecDigestSHA2; hash_length = 256; }
			else if (algorithm == HashAlgorithm::SHA384) { hash_class = kSecDigestSHA2; hash_length = 384; }
			else if (algorithm == HashAlgorithm::SHA512) { hash_class = kSecDigestSHA2; hash_length = 512; }
			else return 0;
			CFErrorRef error;
			SecTransformRef transform = SecDigestTransformCreate(hash_class, hash_length, &error);
			if (!transform) { CFRelease(error); return 0; }
			CFDataRef data_ref = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const uint8 *>(data), length, kCFAllocatorNull);
			if (!data_ref) { CFRelease(transform); return 0; }
			SecTransformSetAttribute(transform, kSecTransformInputAttributeName, data_ref, &error);
			if (error) { CFRelease(error); CFRelease(transform); CFRelease(data_ref); return 0; }
			CFDataRef output_ref = reinterpret_cast<CFDataRef>(SecTransformExecute(transform, &error));
			CFRelease(data_ref);
			CFRelease(transform);
			if (!output_ref) { CFRelease(error); return 0; }
			SafePointer<DataBlock> result;
			try {
				result = new DataBlock(CFDataGetLength(output_ref));
				result->SetLength(CFDataGetLength(output_ref));
				MemoryCopy(result->GetBuffer(), CFDataGetBytePtr(output_ref), result->Length());
			} catch (...) { CFRelease(output_ref); return 0; }
			CFRelease(output_ref);
			result->Retain();
			return result;
		}
		DataBlock * CreateHash(HashAlgorithm algorithm, const DataBlock * data) { return CreateHash(algorithm, data->GetBuffer(), data->Length()); }
		DataBlock * CreateSecureRandom(int length)
		{
			SafePointer<DataBlock> result = new DataBlock(length);
			result->SetLength(length);
			if (SecRandomCopyBytes(kSecRandomDefault, length, result->GetBuffer()) != errSecSuccess) return 0;
			result->Retain();
			return result;
		}
	}
}