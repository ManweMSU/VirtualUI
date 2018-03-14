#pragma once

#include "../Streaming.h"
#include <ObjIdl.h>

namespace Engine
{
	namespace Streaming
	{
		class ComStream : public IStream
		{
			unsigned long _refcount = 1;
			Stream * inner;
		public:
			ComStream(Stream * object);
			~ComStream(void);
			virtual HRESULT __stdcall QueryInterface(REFIID riid, void ** ppvObject) override;
			virtual ULONG __stdcall AddRef(void) override;
			virtual ULONG __stdcall Release(void) override;
			virtual HRESULT __stdcall Read(void * pv, ULONG cb, ULONG * pcbRead) override;
			virtual HRESULT __stdcall Write(const void * pv, ULONG cb, ULONG * pcbWritten) override;
			virtual HRESULT __stdcall Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition) override;
			virtual HRESULT __stdcall SetSize(ULARGE_INTEGER libNewSize) override;
			virtual HRESULT __stdcall CopyTo(IStream * pstm, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten) override;
			virtual HRESULT __stdcall Commit(DWORD grfCommitFlags) override;
			virtual HRESULT __stdcall Revert(void) override;
			virtual HRESULT __stdcall LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
			virtual HRESULT __stdcall UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
			virtual HRESULT __stdcall Stat(STATSTG * pstatstg, DWORD grfStatFlag) override;
			virtual HRESULT __stdcall Clone(IStream ** ppstm) override;
		};
	}
}