#include "ComInterop.h"

#undef min
#undef ZeroMemory

namespace Engine
{
	namespace Streaming
	{
		ComStream::ComStream(Stream * object) { inner = object; inner->Retain(); }
		ComStream::~ComStream(void) { inner->Release(); }
		HRESULT ComStream::QueryInterface(REFIID riid, void ** ppvObject)
		{
			if ((riid == __uuidof(IUnknown)) || (riid == __uuidof(IStream)) || (riid == __uuidof(ISequentialStream)))
			{
				*ppvObject = this;
				AddRef();
				return S_OK;
			} else return E_NOINTERFACE;
		}
		ULONG ComStream::AddRef(void) { return ::InterlockedIncrement(&_refcount); }
		ULONG ComStream::Release(void)
		{
			ULONG New = ::InterlockedDecrement(&_refcount);
			if (!New) delete this;
			return New;
		}
		HRESULT ComStream::Read(void * pv, ULONG cb, ULONG * pcbRead)
		{
			try {
				inner->Read(pv, cb);
				if (pcbRead) *pcbRead = cb;
				return S_OK;
			}
			catch (IO::FileReadEndOfFileException & rf) { if (pcbRead) *pcbRead = rf.DataRead; return S_FALSE; }
			catch (...) { if (pcbRead) *pcbRead = 0; return S_FALSE; }
		}
		HRESULT ComStream::Write(const void * pv, ULONG cb, ULONG * pcbWritten)
		{
			try {
				inner->Write(pv, cb);
				if (pcbWritten) *pcbWritten = cb;
				return S_OK;
			} catch (...) { return S_FALSE; }
		}
		HRESULT ComStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
		{
			try {
				int64 result;
				if (dwOrigin == SEEK_SET) result = inner->Seek(dlibMove.QuadPart, Begin);
				else if (dwOrigin == SEEK_CUR) result = inner->Seek(dlibMove.QuadPart, Current);
				else if (dwOrigin == SEEK_END) result = inner->Seek(dlibMove.QuadPart, End);
				else throw InvalidArgumentException();
				if (plibNewPosition) plibNewPosition->QuadPart = result;
				return S_OK;
			} catch (...) { return S_FALSE; }
		}
		HRESULT ComStream::SetSize(ULARGE_INTEGER libNewSize)
		{
			try {
				inner->SetLength(libNewSize.QuadPart);
				return S_OK;
			} catch (...) { return S_FALSE; }
		}
		HRESULT ComStream::CopyTo(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten)
		{
			try {
				constexpr uint64 buflen = 0x100000;
				uint8 * buffer = new (std::nothrow) uint8[buflen];
				if (!buffer) throw OutOfMemoryException();
				try {
					uint64 pending = cb.QuadPart;
					while (pending) {
						uint32 amount = uint32(min(buflen, pending));
						inner->Read(buffer, amount);
						if (pstm->Write(buffer, amount, 0) != S_OK) throw IO::FileAccessException();
						pending -= amount;
					}
				} catch (...) {
					delete[] buffer;
					throw;
				}
				delete[] buffer;
				if (pcbRead) pcbRead->QuadPart = cb.QuadPart;
				if (pcbWritten) pcbWritten->QuadPart = cb.QuadPart;
				return S_OK;
			} catch (...) { return S_FALSE; }
		}
		HRESULT ComStream::Commit(DWORD grfCommitFlags)
		{
			try {
				inner->Flush();
				return S_OK;
			} catch (...) { return S_FALSE; }
		}
		HRESULT ComStream::Revert(void) { return S_OK; }
		HRESULT ComStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) { return E_NOTIMPL; }
		HRESULT ComStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) { return E_NOTIMPL; }
		HRESULT ComStream::Stat(STATSTG * pstatstg, DWORD grfStatFlag)
		{
			try {
				ZeroMemory(pstatstg, sizeof(STATSTG));
				pstatstg->type = STGTY_STORAGE;
				pstatstg->cbSize.QuadPart = inner->Length();
				return S_OK;
			} catch (...) { return S_FALSE; }
		}
		HRESULT ComStream::Clone(IStream ** ppstm) { return E_NOTIMPL; }
	}
}

