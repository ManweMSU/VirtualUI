#pragma once

#include "../EngineBase.h"

namespace Engine
{
	template <class V> class UndoBuffer : public Object
	{
	protected:
		SafeArray<V> VersionPool = SafeArray<V>(0x10);
		V & CurrentStorage;
		int PreviousPointer = -1;
	public:
		UndoBuffer(V & storage) : CurrentStorage(storage) {}
		~UndoBuffer(void) override {}

		bool CanUndo(void) const { return PreviousPointer >= 0; }
		bool CanRedo(void) const { return PreviousPointer < VersionPool.Length() - 1; }

		void RemoveFutureVersions(void) { for (int i = VersionPool.Length() - 1; i > PreviousPointer; i--) VersionPool.Remove(i); }
		void RemoveAllVersions(void) { VersionPool.Clear(); PreviousPointer = -1; }
		virtual void PushCurrentVersion(void)
		{
			if (PreviousPointer == -1 || CurrentStorage != VersionPool[PreviousPointer]) {
				RemoveFutureVersions();
				VersionPool << CurrentStorage;
				PreviousPointer++;
			}
		}

		void Undo(void)
		{
			if (PreviousPointer >= 0) {
				swap(VersionPool[PreviousPointer], CurrentStorage);
				PreviousPointer--;
			}
		}
		void Redo(void)
		{
			if (PreviousPointer < VersionPool.Length() - 1) {
				swap(VersionPool[PreviousPointer + 1], CurrentStorage);
				PreviousPointer++;
			}
		}
	};
	template <class V> class LimitedUndoBuffer : public UndoBuffer<V>
	{
		int MaxDepth;
	public:
		LimitedUndoBuffer(V & storage, int max_depth) : UndoBuffer<V>(storage), MaxDepth(max_depth) {}
		~LimitedUndoBuffer(void) override {}

		virtual void PushCurrentVersion(void) override
		{
			UndoBuffer<V>::PushCurrentVersion();
			if (VersionPool.Length() > MaxDepth) {
				VersionPool.RemoveFirst();
				PreviousPointer--;
			}
		}
	};
}