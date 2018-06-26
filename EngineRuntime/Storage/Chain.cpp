#include "Chain.h"

namespace Engine
{
	namespace Storage
	{
		MethodChain::MethodChain(void) : Code(0) {}
		MethodChain::MethodChain(uint32 code) : Code(code) {}
		MethodChain::MethodChain(CompressionMethod first)
		{
			Code = static_cast<uint32>(first);
		}
		MethodChain::MethodChain(CompressionMethod first, CompressionMethod second)
		{
			Code = static_cast<uint32>(first) | (static_cast<uint32>(second) << 4);
		}
		MethodChain::MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third)
		{
			Code = static_cast<uint32>(first) | (static_cast<uint32>(second) << 4) | (static_cast<uint32>(third) << 8);
		}
		MethodChain::MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth)
		{
			Code = static_cast<uint32>(first) | (static_cast<uint32>(second) << 4) | (static_cast<uint32>(third) << 8) | (static_cast<uint32>(fourth) << 12);
		}
		MethodChain::MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth, CompressionMethod fifth)
		{
			Code = static_cast<uint32>(first) | (static_cast<uint32>(second) << 4) | (static_cast<uint32>(third) << 8) | (static_cast<uint32>(fourth) << 12) | (static_cast<uint32>(fifth) << 16);
		}
		MethodChain::MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth, CompressionMethod fifth, CompressionMethod sixth)
		{
			Code = static_cast<uint32>(first) | (static_cast<uint32>(second) << 4) | (static_cast<uint32>(third) << 8) | (static_cast<uint32>(fourth) << 12) |
				(static_cast<uint32>(fifth) << 16) | (static_cast<uint32>(sixth) << 20);
		}
		MethodChain::MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth, CompressionMethod fifth, CompressionMethod sixth, CompressionMethod seventh)
		{
			Code = static_cast<uint32>(first) | (static_cast<uint32>(second) << 4) | (static_cast<uint32>(third) << 8) | (static_cast<uint32>(fourth) << 12) |
				(static_cast<uint32>(fifth) << 16) | (static_cast<uint32>(sixth) << 20) | (static_cast<uint32>(seventh) << 24);
		}
		MethodChain::MethodChain(CompressionMethod first, CompressionMethod second, CompressionMethod third, CompressionMethod fourth, CompressionMethod fifth, CompressionMethod sixth, CompressionMethod seventh, CompressionMethod eighth)
		{
			Code = static_cast<uint32>(first) | (static_cast<uint32>(second) << 4) | (static_cast<uint32>(third) << 8) | (static_cast<uint32>(fourth) << 12) |
				(static_cast<uint32>(fifth) << 16) | (static_cast<uint32>(sixth) << 20) | (static_cast<uint32>(seventh) << 24) | (static_cast<uint32>(eighth) << 28);
		}
		int MethodChain::Length(void) const { int l = 0; while (((Code >> (l << 2)) & 0xF) && (l < 7)) l++; return l; }
		CompressionMethod MethodChain::operator[](int at) const { return MethodAt(at); }
		CompressionMethod MethodChain::MethodAt(int at) const { return static_cast<CompressionMethod>((Code >> (at << 2)) & 0xF); }
		MethodChain MethodChain::Subchain(uint32 mask) const
		{
			uint32 result = 0;
			uint32 shift = 0;
			for (int i = 0; i < 8; i++) {
				if ((mask >> i) & 1) {
					result |= static_cast<uint32>(MethodAt(i)) << shift;
					shift += 4;
				}
			}
			return result;
		}
		MethodChain MethodChain::Append(CompressionMethod method) const { return MethodChain(Code | (static_cast<uint32>(method) << (Length() << 2))); }
		bool operator==(MethodChain a, MethodChain b) { return a.Code == b.Code; }
		bool operator!=(MethodChain a, MethodChain b) { return a.Code != b.Code; }
		Array<uint8>* Compress(Array<uint8> * data, MethodChain chain)
		{
			SafePointer< Array<uint8> > source;
			SafePointer< Array<uint8> > output;
			source.SetRetain(data);
			for (int i = 0; i < chain.Length(); i++) {
				output.SetReference(Compress(*source, chain[i]));
				if (!output) return 0;
				source.SetRetain(output);
			}
			source->Retain();
			return source;
		}
		Array<uint8>* Compress(Array<uint8> * data, MethodChain & chain, CompressionQuality quality)
		{
			if (quality == CompressionQuality::Force) return Compress(data, chain);
			else if (quality == CompressionQuality::Optional) {
				auto com = Compress(data, chain);
				if (com->Length() < data->Length()) {
					return com;
				} else {
					chain = MethodChain();
					com->Release();
					data->Retain();
					return data;
				}
			} else if (quality == CompressionQuality::Sequential) {
				MethodChain generic;
				SafePointer< Array<uint8> > source;
				SafePointer< Array<uint8> > output;
				source.SetRetain(data);
				for (int i = 0; i < chain.Length(); i++) {
					output.SetReference(Compress(*source, chain[i]));
					if (!output) continue;
					if (output->Length() >= source->Length()) { output.SetReference(0); continue; }
					source.SetRetain(output);
					generic = generic.Append(chain[i]);
				}
				source->Retain();
				chain = generic;
				return source;
			} else if (quality == CompressionQuality::Variative) {
				SafePointer< Array<uint8> > best_data;
				MethodChain best_chain;
				for (int k = 0; k <= chain.Length(); k++) {
					MethodChain local;
					for (int i = k; i < chain.Length(); i++) local = local.Append(chain[i]);
					SafePointer< Array<uint8> > result = Compress(data, local);
					if (!result) continue;
					if (!best_data || result->Length() < best_data->Length()) {
						best_data.SetRetain(result);
						best_chain = local;
					}
				}
				chain = best_chain;
				if (best_data) best_data->Retain();
				return best_data;
			} else if (quality == CompressionQuality::ExtraVariative) {
				SafePointer< Array<uint8> > best_data;
				MethodChain best_chain;
				for (uint8 k = 0; k < (1 << chain.Length()); k++) {
					MethodChain local = chain.Subchain(k);
					SafePointer< Array<uint8> > result = Compress(data, local);
					if (!result) continue;
					if (!best_data || result->Length() < best_data->Length()) {
						best_data.SetRetain(result);
						best_chain = local;
					}
				}
				chain = best_chain;
				if (best_data) best_data->Retain();
				return best_data;
			} else return 0;
		}
		Array<uint8>* Decompress(Array<uint8> * data, MethodChain chain)
		{
			SafePointer< Array<uint8> > source;
			SafePointer< Array<uint8> > output;
			source.SetRetain(data);
			for (int i = chain.Length() - 1; i >= 0; i--) {
				output.SetReference(Decompress(*source, chain[i]));
				if (!output) return 0;
				source.SetRetain(output);
			}
			source->Retain();
			return source;
		}
		class ChainCompressJob : public Tasks::ThreadJob
		{
		public:
			ObjectArray<Semaphore> * access_sem;
			int index;
			uint64 * length;
			int count;
			bool * success;
			MethodChain chain;
			CompressionQuality quality;
			uint32 block_size;
			Streaming::Stream * source;
			Streaming::Stream * dest;
			virtual void DoJob(Tasks::ThreadPool * pool) override
			{
				access_sem->ElementAt(index)->Wait();
				while ((*length) && (*success)) {
					try {
						uint32 local_length = ((*length) > block_size) ? block_size : uint32(*length);
						SafePointer< Array<uint8> > block = new Array<uint8>(local_length);
						block->SetLength(local_length);
						source->Read(block->GetBuffer(), local_length);
						(*length) -= local_length;
						access_sem->ElementAt((index + 1) % count)->Open();
						MethodChain block_chain = chain;
						SafePointer< Array<uint8> > com = Compress(block, block_chain, quality);
						if (!com) throw Exception();
						uint32 new_length = com->Length();
						access_sem->ElementAt(index)->Wait();
						if (!(*success)) return;
						dest->Write(&new_length, 4);
						dest->Write(&local_length, 4);
						dest->Write(&block_chain.Code, 4);
						dest->Write(com->GetBuffer(), com->Length());
					}
					catch (...) {
						*success = false;
						for (int i = 0; i < count; i++) access_sem->ElementAt(i)->Open();
						return;
					}
				}
				access_sem->ElementAt((index + 1) % count)->Open();
			}
		};
		class ChainDecompressJob : public Tasks::ThreadJob
		{
		public:
			ObjectArray<Semaphore> * access_sem;
			int index;
			uint32 * block_count;
			int count;
			bool * success;
			Streaming::Stream * source;
			Streaming::Stream * dest;
			virtual void DoJob(Tasks::ThreadPool * pool) override
			{
				access_sem->ElementAt(index)->Wait();
				while ((*block_count) && (*success)) {
					try {
						uint32 com_length;
						uint32 native_length;
						MethodChain chain;
						source->Read(&com_length, 4);
						source->Read(&native_length, 4);
						source->Read(&chain.Code, 4);
						SafePointer< Array<uint8> > com = new Array<uint8>(com_length);
						com->SetLength(com_length);
						source->Read(com->GetBuffer(), com_length);
						(*block_count)--;
						access_sem->ElementAt((index + 1) % count)->Open();
						SafePointer< Array<uint8> > data = Decompress(com, chain);
						if (!data) throw Exception();
						access_sem->ElementAt(index)->Wait();
						if (!(*success)) return;
						dest->Write(data->GetBuffer(), data->Length());
					} catch (...) {
						*success = false;
						for (int i = 0; i < count; i++) access_sem->ElementAt(i)->Open();
						return;
					}
				}
				access_sem->ElementAt((index + 1) % count)->Open();
			}
		};
		bool ChainCompress(Streaming::Stream * dest, Streaming::Stream * source, MethodChain chain, CompressionQuality quality, Tasks::ThreadPool * pool, uint32 block_size)
		{
			if (!block_size) return false;
			uint64 length = source->Length();
			uint64 block_count_long = (length + block_size - 1) / block_size;
			if (block_count_long > 0xFFFFFFFF) return false;
			uint32 block_count = uint32(block_count_long);
			dest->Write(&block_count, 4);
			if (pool) {
				pool->Wait();
				ObjectArray<Semaphore> access_sem(0x10);
				for (int i = 0; i < pool->GetThreadCount(); i++) {
					SafePointer<Semaphore> sem = CreateSemaphore(i ? 0 : 1);
					access_sem.Append(sem);
				}
				int thread_count = pool->GetThreadCount();
				bool success = true;
				pool->BeginSubmit();
				for (int i = 0; i < thread_count; i++) {
					SafePointer<ChainCompressJob> job = new ChainCompressJob;
					job->access_sem = &access_sem;
					job->index = i;
					job->length = &length;
					job->count = thread_count;
					job->success = &success;
					job->chain = chain;
					job->quality = quality;
					job->block_size = block_size;
					job->source = source;
					job->dest = dest;
					pool->AppendJob(job);
				}
				pool->EndSubmit();
				pool->Wait();
				return success;
			} else {
				while (length) {
					uint32 local_length = (length > block_size) ? block_size : uint32(length);
					SafePointer< Array<uint8> > block = new Array<uint8>(local_length);
					MethodChain block_chain = chain;
					block->SetLength(local_length);
					source->Read(block->GetBuffer(), local_length);
					SafePointer< Array<uint8> > com = Compress(block, block_chain, quality);
					if (!com) return false;
					uint32 new_length = com->Length();
					dest->Write(&new_length, 4);
					dest->Write(&local_length, 4);
					dest->Write(&block_chain.Code, 4);
					dest->Write(com->GetBuffer(), com->Length());
					length -= local_length;
				}
			}
			return true;
		}
		bool ChainDecompress(Streaming::Stream * dest, Streaming::Stream * source, Tasks::ThreadPool * pool)
		{
			uint32 block_count;
			source->Read(&block_count, 4);
			if (pool) {
				pool->Wait();
				ObjectArray<Semaphore> access_sem(0x10);
				for (int i = 0; i < pool->GetThreadCount(); i++) {
					SafePointer<Semaphore> sem = CreateSemaphore(i ? 0 : 1);
					access_sem.Append(sem);
				}
				int thread_count = pool->GetThreadCount();
				bool success = true;
				pool->BeginSubmit();
				for (int i = 0; i < thread_count; i++) {
					SafePointer<ChainDecompressJob> job = new ChainDecompressJob;
					job->access_sem = &access_sem;
					job->index = i;
					job->block_count = &block_count;
					job->count = thread_count;
					job->success = &success;
					job->source = source;
					job->dest = dest;
					pool->AppendJob(job);
				}
				pool->EndSubmit();
				pool->Wait();
				return success;
			} else {
				while (block_count) {
					uint32 com_length;
					uint32 native_length;
					MethodChain chain;
					source->Read(&com_length, 4);
					source->Read(&native_length, 4);
					source->Read(&chain.Code, 4);
					SafePointer< Array<uint8> > com = new Array<uint8>(com_length);
					com->SetLength(com_length);
					source->Read(com->GetBuffer(), com_length);
					SafePointer< Array<uint8> > data = Decompress(com, chain);
					if (!data) return false;
					dest->Write(data->GetBuffer(), data->Length());
					block_count--;
				}
			}
			return true;
		}
		uint32 DecompressionStream::block_at(uint64 offset) const
		{
			for (int i = 0; i < blocks.Length() - 1; i++) if (blocks[i + 1].real_offset > offset) return i;
			return blocks.Length() - 1;
		}
		DecompressionStream::DecompressionStream(Streaming::Stream * source) : blocks(0x100)
		{
			inner.SetRetain(source);
			pointer = 0;
			size = 0;
			current_block = 0;
			inner->Seek(0, Streaming::Begin);
			uint32 block_count;
			inner->Read(&block_count, 4);
			blocks.SetLength(block_count);
			int block = 0;
			while (block_count) {
				uint32 com_length;
				uint32 native_length;
				MethodChain chain;
				inner->Read(&com_length, 4);
				inner->Read(&native_length, 4);
				inner->Read(&chain.Code, 4);
				blocks[block].offset = inner->Seek(0, Streaming::Current);
				blocks[block].real_offset = size;
				blocks[block].length = native_length;
				blocks[block].com_length = com_length;
				blocks[block].chain = chain;
				size += uint64(native_length);
				inner->Seek(com_length, Streaming::Current);
				block++;
				block_count--;
			}
			Seek(0, Streaming::Begin);
		}
		DecompressionStream::~DecompressionStream(void) {}
		void DecompressionStream::Read(void * buffer, uint32 length)
		{
			uint8 * dest = reinterpret_cast<uint8 *>(buffer);
			uint32 read = 0;
			while (length) {
				uint32 can_read = uint32(blocks[current_block].real_offset + uint64(blocks[current_block].length) - pointer);
				if (!can_read) throw IO::FileReadEndOfFileException(read);
				uint32 vpos = uint32(pointer - blocks[current_block].real_offset);
				if (can_read > length) can_read = length;
				MemoryCopy(dest, blocks[current_block].data->GetBuffer() + vpos, can_read);
				length -= can_read;
				Seek(can_read, Streaming::Current);
				read += can_read;
				dest += can_read;
			}
		}
		void DecompressionStream::Write(const void * data, uint32 length) { throw Exception(); }
		int64 DecompressionStream::Seek(int64 position, Streaming::SeekOrigin origin)
		{
			int64 newpos = position;
			if (origin == Streaming::Current) newpos += pointer;
			else if (origin == Streaming::End) newpos += size;
			if (newpos < 0 || newpos > int64(size)) throw InvalidArgumentException();
			uint32 sb = current_block;
			uint32 nb = block_at(newpos);
			if (sb != nb || !blocks[nb].data) {
				blocks[sb].data.SetReference(0);
				SafePointer< Array<uint8> > src = new Array<uint8>(blocks[nb].com_length);
				src->SetLength(blocks[nb].com_length);
				inner->Seek(blocks[nb].offset, Streaming::Begin);
				inner->Read(src->GetBuffer(), blocks[nb].com_length);
				SafePointer< Array<uint8> > data = Decompress(src, blocks[nb].chain);
				blocks[nb].data.SetRetain(data);
				current_block = nb;
			}
			pointer = newpos;
			return pointer;
		}
		uint64 DecompressionStream::Length(void) { return size; }
		void DecompressionStream::SetLength(uint64 length) { throw Exception(); }
		void DecompressionStream::Flush(void) {}
	}
}