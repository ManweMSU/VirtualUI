#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Cryptography
	{
		class Algorithm;
		class Key;

		enum class EncryptionAlgorithm { AES, DES, TripleDES, RC2, RC4 };
		enum class HashAlgorithm { MD5, SHA1, SHA256, SHA384, SHA512 };
		enum class EncryptionMode { ECB, CBC, CFB, Unknown };

		class Algorithm : public Object
		{
		public:
			virtual EncryptionAlgorithm Identifier(void) const = 0;
			virtual int EncryptionBlockSize(void) const = 0;
			virtual EncryptionMode GetEncryptionMode(void) const = 0;
			virtual void SetEncryptionMode(EncryptionMode mode) = 0;
			virtual Key * ImportKey(const void * data, int length) = 0;
			virtual Key * GenerateKey(const void * secret, int length) = 0;

			Key * ImportKey(const DataBlock * data);
			Key * GenerateKey(const DataBlock * data);
		};
		class Key : public Object
		{
		public:
			virtual DataBlock * ExportKey(void) = 0;
			virtual Algorithm * GetAlgorithm(void) const = 0;
			virtual DataBlock * EncryptData(const void * data, int length, const void * iv = 0) = 0;
			virtual DataBlock * DecryptData(const void * data, int length, const void * iv = 0) = 0;

			DataBlock * EncryptData(const DataBlock * data, const void * iv = 0);
			DataBlock * DecryptData(const DataBlock * data, const void * iv = 0);
		};

		Algorithm * OpenEncryptionAlgorithm(EncryptionAlgorithm algorithm = EncryptionAlgorithm::AES);
		DataBlock * CreateHash(HashAlgorithm algorithm, const void * data, int length);
		DataBlock * CreateHash(HashAlgorithm algorithm, const DataBlock * data);
		DataBlock * CreateSecureRandom(int length);
	}
}