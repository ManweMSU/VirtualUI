#pragma once

#include "Compression.h"
#include "../Miscellaneous/ThreadPool.h"
#include "../Streaming.h"

namespace Engine
{
	namespace Storage
	{
		class MethodChain
		{
		public:
			uint32 Code;
			MethodChain(void);
			MethodChain(uint32 code);
			MethodChain(CompressionMethod first);
			MethodChain(CompressionMethod first, CompressionMethod second);
			MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third);
			MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth);
			MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth, CompressionMethod fifth);
			MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth, CompressionMethod fifth, CompressionMethod sixth);
			MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth, CompressionMethod fifth, CompressionMethod sixth, CompressionMethod seventh);
			MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth, CompressionMethod fifth, CompressionMethod sixth, CompressionMethod seventh, CompressionMethod eighth);

			bool friend operator == (MethodChain a, MethodChain b);
			bool friend operator != (MethodChain a, MethodChain b);

			int Length(void) const;
			CompressionMethod operator [] (int at) const;
			CompressionMethod MethodAt(int at) const;
			MethodChain Subchain(uint32 mask) const;
			MethodChain Append(CompressionMethod method) const;
		};
		enum class CompressionQuality {
			// Always apply full compression chain, efficiency is not checked.
			Force,
			// Apply full compression chain or do not compress if efficiency is negative.
			Optional,
			// The compression step is skipped, if the local efficiency of this step is negative.
			Sequential,
			// Variates chaines by excluding some first methods from source chain. The most effective chain result is written.
			Variative,
			// Variates all subchaines of the source chain. The most effective chain result is written. Very expensive.
			ExtraVariative
		};
		Array<uint8> * Compress(Array<uint8> * data, MethodChain chain);
		Array<uint8> * Compress(Array<uint8> * data, MethodChain & chain, CompressionQuality quality);
		Array<uint8> * Decompress(Array<uint8> * data, MethodChain chain);

		bool ChainCompress(Streaming::Stream * dest, Streaming::Stream * source, MethodChain chain, CompressionQuality quality, ThreadPool * pool = 0, uint32 block_size = 0x100000);
		bool ChainDecompress(Streaming::Stream * dest, Streaming::Stream * source, ThreadPool * pool = 0);

		class DecompressionStream : public Streaming::Stream
		{
			struct BlockInfo {
				uint64 offset;
				uint64 real_offset;
				uint32 length;
				uint32 com_length;
				MethodChain chain;
				SafePointer< Array<uint8> > data;
			};
			SafePointer<Streaming::Stream> inner;
			Array<BlockInfo> blocks;
			uint64 pointer;
			uint64 size;
			uint32 current_block;

			uint32 block_at(uint64 offset) const;
		public:
			DecompressionStream(Streaming::Stream * source);
			~DecompressionStream(void) override;

			virtual void Read(void * buffer, uint32 length) override;
			virtual void Write(const void * data, uint32 length) override;
			virtual int64 Seek(int64 position, Streaming::SeekOrigin origin) override;
			virtual uint64 Length(void) override;
			virtual void SetLength(uint64 length) override;
			virtual void Flush(void) override;
		};
	}
}