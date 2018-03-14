#include "Streaming.h"

namespace Engine
{
	namespace Streaming
	{
		using namespace ::Engine::IO;

		void Stream::CopyTo(Stream * to, uint64 length)
		{
			constexpr uint64 buflen = 0x100000;
			uint8 * buffer = new (std::nothrow) uint8[buflen];
			if (!buffer) throw OutOfMemoryException();
			try {
				uint64 pending = length;
				while (pending) {
					uint32 amount = uint32(min(buflen, pending));
					Read(buffer, amount);
					to->Write(buffer, amount);
					pending -= amount;
				}
			}
			catch (...) {
				delete[] buffer;
				throw;
			}
			delete[] buffer;
		}
		void Stream::CopyTo(Stream * to) { CopyTo(to, Length()); }
		FileStream::FileStream(const string & path, FileAccess access, FileCreationMode mode) : owned(true) { file = CreateFile(path, static_cast<IO::FileAccess>(access), static_cast<IO::FileCreationMode>(mode)); }
		FileStream::FileStream(handle file_handle, bool take_control) : owned(take_control) { file = file_handle; }
		void FileStream::Read(void * buffer, uint32 length) { ReadFile(file, buffer, length); }
		void FileStream::Write(const void * data, uint32 length) { WriteFile(file, data, length); }
		int64 FileStream::Seek(int64 position, SeekOrigin origin) { return IO::Seek(file, position, static_cast<IO::SeekOrigin>(origin)); }
		uint64 FileStream::Length(void) { return GetFileSize(file); }
		void FileStream::SetLength(uint64 length) { SetFileSize(file, length); }
		void FileStream::Flush(void) { IO::Flush(file); }
		FileStream::~FileStream(void) { if (owned) CloseFile(file); }
		string FileStream::ToString(void) const { return L"FileStream"; }

		MemoryStream::MemoryStream(const void * source, int length) : data(0x10000), pointer(0) { data.SetLength(length); MemoryCopy(data, source, length); }
		MemoryStream::MemoryStream(const void * source, int length, int block) : data(block), pointer(0) { data.SetLength(length); MemoryCopy(data, source, length); }
		MemoryStream::MemoryStream(Stream * source, int length) : data(0x10000), pointer(0) { data.SetLength(length); source->Read(data, length); }
		MemoryStream::MemoryStream(Stream * source, int length, int block) : data(block), pointer(0) { data.SetLength(length); source->Read(data, length); }
		MemoryStream::MemoryStream(int block) : data(block), pointer(0) {}
		void MemoryStream::Read(void * buffer, uint32 length)
		{
			if (length > uint32(data.Length() - pointer)) {
				length = (pointer >= data.Length()) ? 0 : (uint32(data.Length()) - pointer);
				MemoryCopy(buffer, data.GetBuffer() + pointer, length);
				pointer += length;
				throw FileReadEndOfFileException(length);
			}
			MemoryCopy(buffer, data.GetBuffer() + pointer, length);
			pointer += length;
		}
		void MemoryStream::Write(const void * _data, uint32 length)
		{
			if (length > uint32(data.Length() - pointer)) {
				if (length > 0x7FFFFFFF) throw InvalidArgumentException();
				if (uint32(pointer) + length > 0x7FFFFFFF) throw FileAccessException();
				data.SetLength(pointer + int(length));
			}
			MemoryCopy(data.GetBuffer() + pointer, _data, length);
			pointer += int(length);
		}
		int64 MemoryStream::Seek(int64 position, SeekOrigin origin)
		{
			int64 newpos = position;
			if (origin == Current) newpos += int64(pointer);
			else if (origin == End) newpos += int64(data.Length());
			if (newpos < 0 || newpos > int64(data.Length())) throw InvalidArgumentException();
			pointer = int32(newpos);
			return pointer;
		}
		uint64 MemoryStream::Length(void) { return uint64(data.Length()); }
		void MemoryStream::SetLength(uint64 length)
		{
			if (length > 0x7FFFFFFF) throw InvalidArgumentException();
			data.SetLength(int(length));
		}
		void MemoryStream::Flush(void) {}
		MemoryStream::~MemoryStream(void) {}
		string MemoryStream::ToString(void) const { return L"MemoryStream"; }
		TextWriter::TextWriter(Stream * Dest) { dest = Dest; dest->Retain(); coding = TextFileEncoding; }
		TextWriter::TextWriter(Stream * Dest, Encoding encoding) { dest = Dest; dest->Retain(); coding = encoding; }
		void TextWriter::Write(const string & text) const
		{
			auto data = text.EncodeSequence(coding, false);
			dest->Write(data->GetBuffer(), data->Length());
			data->Release();
		}
		void TextWriter::WriteLine(const string & text) const { Write(text + NewLineChar); }
		void TextWriter::WriteEncodingSignature(void) const
		{
			if (coding == Encoding::ANSI) {}
			else if (coding == Encoding::UTF8) { dest->Write("\xEF\xBB\xBF", 3); }
			else if (coding == Encoding::UTF16) { dest->Write("\xFF\xFE", 2); }
			else if (coding == Encoding::UTF32) { dest->Write("\xFF\xFE\x00\x00", 4); }
		}
		TextWriter::~TextWriter(void) { dest->Release(); }
		string TextWriter::ToString(void) const { return L"TextWriter"; }
		TextWriter & TextWriter::operator<<(const string & text) { Write(text); return *this; }
		const TextWriter & TextWriter::operator<<(const string & text) const { Write(text); return *this; }
		FragmentStream::FragmentStream(Stream * Inner, uint64 From, uint64 Length) : inner(Inner), begin(From), end(From + Length), pointer(From) { inner->Retain(); }
		void FragmentStream::Read(void * buffer, uint32 length)
		{
			if (uint64(length) > end - begin - pointer) {
				length = (uint64(pointer) >= end - begin) ? 0 : uint32(end - begin - pointer);
				auto pos = inner->Seek(0, Current);
				inner->Seek(begin + pointer, Begin);
				try {
					inner->Read(buffer, length);
				}
				catch (...) { inner->Seek(pos, Begin); throw; }
				inner->Seek(pos, Begin);
				pointer += length;
				throw FileReadEndOfFileException(length);
			}
			auto pos = inner->Seek(0, Current);
			inner->Seek(begin + pointer, Begin);
			try {
				inner->Read(buffer, length);
			} catch (...) { inner->Seek(pos, Begin); throw; }
			pointer += length;
		}
		void FragmentStream::Write(const void * data, uint32 length) { throw FileAccessException(); }
		int64 FragmentStream::Seek(int64 position, SeekOrigin origin)
		{
			int64 newpos = position;
			if (origin == Current) newpos += pointer;
			else if (origin == End) newpos += end - begin;
			if (newpos < 0 || uint64(newpos) > end - begin) throw InvalidArgumentException();
			pointer = newpos;
			return pointer;
		}
		uint64 FragmentStream::Length(void) { return end - begin; }
		void FragmentStream::SetLength(uint64 length) { throw FileAccessException(); }
		void FragmentStream::Flush(void) {}
		FragmentStream::~FragmentStream(void) { inner->Release(); }
		string FragmentStream::ToString(void) const { return L"FragmentStream"; }
	}
}