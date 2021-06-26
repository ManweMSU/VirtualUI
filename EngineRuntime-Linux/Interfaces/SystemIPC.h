#pragma once

#include "SystemIO.h"

namespace Engine
{
	namespace IPC
	{
		enum CreatePipeFlags {
			PipeAccessModeRead	= 0x01,
			PipeAccessModeWrite	= 0x02,
			PipeCreateNew		= 0x80,
		};
		enum CreateSharedMemoryFlags {
			SharedMemoryOpenExisting	= 0x00,
			SharedMemoryCreateNew		= 0x80,
		};
		enum SharedMemoryMapFlags {
			SharedMemoryMapNone			= 0x00,
			SharedMemoryMapRead			= 0x01,
			SharedMemoryMapWrite		= 0x02,
			SharedMemoryMapReadWrite	= SharedMemoryMapRead | SharedMemoryMapWrite
		};
		enum IPCErrors {
			ErrorSuccess			= 0x00,
			ErrorAlreadyExists		= 0x01,
			ErrorNotExists			= 0x02,
			ErrorWrongMode			= 0x03,
			ErrorBusyNow			= 0x04,
			ErrorAllocation			= 0x05,
			ErrorInvalidArgument	= 0x06,
		};

		class IPipe : public Object
		{
		public:
			virtual handle GetIOHandle(void) noexcept = 0;
			virtual string GetOSPath(void) = 0;
			virtual string GetPipeName(void) = 0;
			virtual uint GetAccessMode(void) noexcept = 0;
		};
		class ISharedMemory : public Object
		{
		public:
			virtual string GetSegmentName(void) = 0;
			virtual uint GetLength(void) noexcept = 0;
			virtual bool Map(void ** pdata, uint map_flags) noexcept = 0;
			virtual void Unmap(void) noexcept = 0;
		};
		class ISharedSemaphore : public Object
		{
		public:
			virtual string GetSemaphoreName(void) = 0;
			virtual bool TryWait(void) noexcept = 0;
			virtual void Open(void) noexcept = 0;
		};

		IPipe * CreateNamedPipe(const string & pipe_name, uint pipe_flags, uint * error = 0);
		ISharedMemory * CreateSharedMemory(const string & segment_name, uint length, uint flags, uint * error = 0);
		ISharedSemaphore * CreateSharedSemaphore(const string & semaphore_name, uint * error = 0);

		void DestroyNamedPipe(const string & name);
		void DestroySharedMemory(const string & name);
		void DestroySharedSemaphore(const string & name);
	}
}