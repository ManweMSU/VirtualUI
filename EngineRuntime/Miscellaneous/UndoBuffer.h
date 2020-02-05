#pragma once

#include "../EngineBase.h"

namespace Engine
{
	template <class V> class IUndoProperty : public Object
	{
	public:
		virtual V GetCurrentValue(void) = 0;
		virtual void SetCurrentValue(const V & value) = 0;
	};
	template <class V> class VariableUndoProperty : public IUndoProperty<V>
	{
		V & CurrentStorage;
	public:
		VariableUndoProperty(V & storage) : CurrentStorage(storage) {}
		virtual V GetCurrentValue(void) override { return CurrentStorage; }
		virtual void SetCurrentValue(const V & value) override { CurrentStorage = value; }
	};
	template <class V> class UndoBuffer : public Object
	{
	protected:
		SafeArray<V> VersionPool = SafeArray<V>(0x10);
		SafePointer< IUndoProperty<V> > Property;
		int PreviousPointer = -1;
	public:
		UndoBuffer(V & storage) { Property = new VariableUndoProperty<V>(storage); }
		UndoBuffer(IUndoProperty<V> * property) { Property.SetRetain(property); }
		~UndoBuffer(void) override {}

		bool CanUndo(void) const { return PreviousPointer >= 0; }
		bool CanRedo(void) const { return PreviousPointer < VersionPool.Length() - 1; }

		void RemoveFutureVersions(void) { for (int i = VersionPool.Length() - 1; i > PreviousPointer; i--) VersionPool.Remove(i); }
		void RemoveAllVersions(void) { VersionPool.Clear(); PreviousPointer = -1; }
		virtual void PushCurrentVersion(void)
		{
			auto value = Property->GetCurrentValue();
			if (PreviousPointer == -1 || value != VersionPool[PreviousPointer]) {
				RemoveFutureVersions();
				VersionPool << value;
				PreviousPointer++;
			}
		}

		void Undo(void)
		{
			if (PreviousPointer >= 0) {
				auto value = Property->GetCurrentValue();
				Property->SetCurrentValue(VersionPool[PreviousPointer]);
				VersionPool[PreviousPointer] = value;
				PreviousPointer--;
			}
		}
		void Redo(void)
		{
			if (PreviousPointer < VersionPool.Length() - 1) {
				auto value = Property->GetCurrentValue();
				Property->SetCurrentValue(VersionPool[PreviousPointer + 1]);
				VersionPool[PreviousPointer + 1] = value;
				PreviousPointer++;
			}
		}
	};
	template <class V> class LimitedUndoBuffer : public UndoBuffer<V>
	{
		int MaxDepth;
	public:
		LimitedUndoBuffer(V & storage, int max_depth) : UndoBuffer<V>(storage), MaxDepth(max_depth) {}
		LimitedUndoBuffer(IUndoProperty<V> * property, int max_depth) : UndoBuffer<V>(property), MaxDepth(max_depth) {}
		~LimitedUndoBuffer(void) override {}

		virtual void PushCurrentVersion(void) override
		{
			UndoBuffer<V>::PushCurrentVersion();
			if (UndoBuffer<V>::VersionPool.Length() > MaxDepth) {
				UndoBuffer<V>::VersionPool.RemoveFirst();
				UndoBuffer<V>::PreviousPointer--;
			}
		}
	};
}