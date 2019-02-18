#include "Object.h"

namespace Engine
{
	namespace Reflection
	{
		namespace EngineObjectInternal {
			struct StringCacheEntry
			{
				const Array<uint8> * src;
				uint32 offset;
				int32 length;
				const char * GetText(void) { return reinterpret_cast<const char *>(src->GetBuffer() + offset); }
			};
			class PropertyCounter : public IPropertyEnumerator
			{
			public:
				Array<PropertyInfo> arr = Array<PropertyInfo>(0x10);
				virtual void EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size) override
				{
					if (type == PropertyType::Texture || type == PropertyType::Font || type == PropertyType::Application || type == PropertyType::Dialog) return;
					arr << PropertyInfo{ address, type, inner, volume, element_size, name };
				}
			};
			ENGINE_PACKED_STRUCTURE(Rectangle)
				int LeftAbsolute;
				double LeftScale;
				double LeftAnchor;
				int TopAbsolute;
				double TopScale;
				double TopAnchor;
				int RightAbsolute;
				double RightScale;
				double RightAnchor;
				int BottomAbsolute;
				double BottomScale;
				double BottomAnchor;
				Rectangle(void) {}
				Rectangle(const UI::Rectangle & rect) : LeftAbsolute(rect.Left.Absolute), LeftScale(rect.Left.Zoom), LeftAnchor(rect.Left.Anchor),
					TopAbsolute(rect.Top.Absolute), TopScale(rect.Top.Zoom), TopAnchor(rect.Top.Anchor), RightAbsolute(rect.Right.Absolute), RightScale(rect.Right.Zoom), RightAnchor(rect.Right.Anchor),
					BottomAbsolute(rect.Bottom.Absolute), BottomScale(rect.Bottom.Zoom), BottomAnchor(rect.Bottom.Anchor) {}
				UI::Rectangle ToRectangle(void) const
				{
					return UI::Rectangle(
						UI::Coordinate(LeftAbsolute, LeftScale, LeftAnchor),
						UI::Coordinate(TopAbsolute, TopScale, TopAnchor),
						UI::Coordinate(RightAbsolute, RightScale, RightAnchor),
						UI::Coordinate(BottomAbsolute, BottomScale, BottomAnchor)
					);
				}
			ENGINE_END_PACKED_STRUCTURE
			void WriteData(Array<uint8> & dest, const void * data, int length)
			{
				auto base = dest.Length();
				dest.SetLength(dest.Length() + length);
				MemoryCopy(dest.GetBuffer() + base, data, length);
			}
			uint32 WriteString(Array<uint8> & dest, Array<StringCacheEntry> & cache, const string & text)
			{
				Array<char> utf8(0x10);
				int len = text.GetEncodedLength(Encoding::UTF8) + 1;
				utf8.SetLength(len);
				text.Encode(utf8.GetBuffer(), Encoding::UTF8, true);
				for (int i = 0; i < cache.Length(); i++) {
					if (cache[i].length == len && MemoryCompare(cache[i].GetText(), utf8.GetBuffer(), len) == 0) return cache[i].offset;
				}
				StringCacheEntry entry{ &dest, uint32(dest.Length()), len };
				WriteData(dest, utf8.GetBuffer(), len);
				cache << entry;
				return entry.offset;
			}
			uint32 SerializeStructure(Reflected & obj, Array<uint8> & dest, Array<StringCacheEntry> & cache);
			uint32 WriteValue(PropertyInfo & prop, Array<uint8> & dest, Array<StringCacheEntry> & cache)
			{
				if (prop.Type == PropertyType::String) {
					return WriteString(dest, cache, prop.Get<string>());
				} else if (prop.Type == PropertyType::Rectangle) {
					uint32 offset = uint32(dest.Length());
					Rectangle rect(prop.Get<UI::Rectangle>());
					WriteData(dest, &rect, sizeof(rect));
					return offset;
				} else if (prop.Type == PropertyType::Structure) {
					return SerializeStructure(prop.Get<Reflected>(), dest, cache);
				} else {
					uint32 offset = uint32(dest.Length());
					WriteData(dest, prop.Address, prop.ElementSize);
					return offset;
				}
			}
			template <class S> void WriteSimpleArray(PropertyInfo & prop, Array<uint8> & dest, uint32 & volume)
			{
				auto & arr = prop.Get< Array<S> >();
				volume = arr.Length();
				WriteData(dest, arr.GetBuffer(), arr.Length() * sizeof(S));
			}
			template <class S> void WriteSimpleSafeArray(PropertyInfo & prop, Array<uint8> & dest, uint32 & volume)
			{
				auto & arr = prop.Get< SafeArray<S> >();
				volume = arr.Length();
				for (int i = 0; i < arr.Length(); i++) WriteData(dest, &arr[i], sizeof(S));
			}
			void WriteArray(PropertyType actual, PropertyInfo & prop, Array<uint8> & dest, Array<StringCacheEntry> & cache, uint32 & volume, uint32 & data)
			{
				data = uint32(dest.Length());
				if (actual == PropertyType::String) {
					uint32 val = 0;
					if (prop.Type == PropertyType::Array) {
						auto & arr = prop.Get< Array<string> >();
						volume = arr.Length();
						for (uint i = 0; i < volume; i++) WriteData(dest, &val, 4);
						for (uint i = 0; i < volume; i++) {
							uint32 text_offset = WriteString(dest, cache, arr[i]);
							*reinterpret_cast<uint32 *>(dest.GetBuffer() + data + 4 * i) = text_offset;
						}
					} else if (prop.Type == PropertyType::SafeArray) {
						auto & arr = prop.Get< SafeArray<string> >();
						volume = arr.Length();
						for (uint i = 0; i < volume; i++) WriteData(dest, &val, 4);
						for (uint i = 0; i < volume; i++) {
							uint32 text_offset = WriteString(dest, cache, arr[i]);
							*reinterpret_cast<uint32 *>(dest.GetBuffer() + data + 4 * i) = text_offset;
						}
					} else {
						volume = uint32(prop.Volume);
						for (uint i = 0; i < volume; i++) WriteData(dest, &val, 4);
						for (uint i = 0; i < volume; i++) {
							uint32 text_offset = WriteString(dest, cache, prop.VolumeElement(i).Get<string>());
							*reinterpret_cast<uint32 *>(dest.GetBuffer() + data + 4 * i) = text_offset;
						}
					}
				} else if (actual == PropertyType::Rectangle) {
					if (prop.Type == PropertyType::Array) {
						auto & arr = prop.Get< Array<UI::Rectangle> >();
						volume = arr.Length();
						for (uint i = 0; i < volume; i++) {
							Rectangle rect(arr[i]);
							WriteData(dest, &rect, sizeof(rect));
						}
					} else if (prop.Type == PropertyType::SafeArray) {
						auto & arr = prop.Get< SafeArray<UI::Rectangle> >();
						volume = arr.Length();
						for (uint i = 0; i < volume; i++) {
							Rectangle rect(arr[i]);
							WriteData(dest, &rect, sizeof(rect));
						}
					} else {
						volume = uint32(prop.Volume);
						for (uint i = 0; i < volume; i++) {
							Rectangle rect(prop.VolumeElement(i).Get<UI::Rectangle>());
							WriteData(dest, &rect, sizeof(rect));
						}
					}
				} else if (actual == PropertyType::Structure) {
					auto & arr = prop.Get<ReflectedArray>();
					volume = arr.Length();
					uint32 val = 0;
					for (uint i = 0; i < volume; i++) WriteData(dest, &val, 4);
					for (uint i = 0; i < volume; i++) {
						uint32 struct_offset = SerializeStructure(arr.ElementAt(i), dest, cache);
						*reinterpret_cast<uint32 *>(dest.GetBuffer() + data + 4 * i) = struct_offset;
					}
				} else {
					if (prop.Type == PropertyType::Array) {
						if (prop.InnerType == PropertyType::UInt8) WriteSimpleArray<uint8>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Int8) WriteSimpleArray<int8>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::UInt16) WriteSimpleArray<uint16>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Int16) WriteSimpleArray<int16>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::UInt32) WriteSimpleArray<uint32>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Int32) WriteSimpleArray<int32>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::UInt64) WriteSimpleArray<uint64>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Int64) WriteSimpleArray<int64>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Float) WriteSimpleArray<float>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Double) WriteSimpleArray<double>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Complex) WriteSimpleArray<Math::Complex>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Boolean) WriteSimpleArray<bool>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Color) WriteSimpleArray<UI::Color>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Time) WriteSimpleArray<Time>(prop, dest, volume);
					} else if (prop.Type == PropertyType::SafeArray) {
						if (prop.InnerType == PropertyType::UInt8) WriteSimpleSafeArray<uint8>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Int8) WriteSimpleSafeArray<int8>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::UInt16) WriteSimpleSafeArray<uint16>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Int16) WriteSimpleSafeArray<int16>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::UInt32) WriteSimpleSafeArray<uint32>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Int32) WriteSimpleSafeArray<int32>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::UInt64) WriteSimpleSafeArray<uint64>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Int64) WriteSimpleSafeArray<int64>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Float) WriteSimpleSafeArray<float>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Double) WriteSimpleSafeArray<double>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Complex) WriteSimpleSafeArray<Math::Complex>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Boolean) WriteSimpleSafeArray<bool>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Color) WriteSimpleSafeArray<UI::Color>(prop, dest, volume);
						else if (prop.InnerType == PropertyType::Time) WriteSimpleSafeArray<Time>(prop, dest, volume);
					} else {
						volume = uint32(prop.Volume);
						WriteData(dest, prop.Address, prop.ElementSize * prop.Volume);
					}
				}
				if (!volume) {
					volume = 0xFFFFFFFF;
					data = 0;
				}
			}
			uint32 SerializeStructure(Reflected & obj, Array<uint8> & dest, Array<StringCacheEntry> & cache)
			{
				obj.WillBeSerialized();
				PropertyCounter props;
				obj.EnumerateProperties(props);
				uint32 base_offset = dest.Length();
				uint32 prop_count = props.arr.Length();
				uint32 value = 0;
				WriteData(dest, &prop_count, 4);
				for (uint i = 0; i < prop_count * 4; i++) WriteData(dest, &value, 4);
				for (uint i = 0; i < prop_count; i++) {
					uint32 prop_offset = base_offset + 4 + 16 * i;
					*reinterpret_cast<uint32 *>(dest.GetBuffer() + prop_offset) = WriteString(dest, cache, props.arr[i].Name);
					PropertyType actual = props.arr[i].Type;
					uint32 volume = 0;
					uint32 data = 0;
					if (actual == PropertyType::Array || actual == PropertyType::SafeArray) {
						actual = props.arr[i].InnerType;
						WriteArray(actual, props.arr[i], dest, cache, volume, data);
					} else {
						if (props.arr[i].Volume > 1) volume = props.arr[i].Volume;
						if (volume) {
							WriteArray(actual, props.arr[i], dest, cache, volume, data);
						} else {
							data = WriteValue(props.arr[i], dest, cache);
						}
					}
					*reinterpret_cast<uint32 *>(dest.GetBuffer() + prop_offset + 4) = uint32(actual);
					*reinterpret_cast<uint32 *>(dest.GetBuffer() + prop_offset + 8) = volume;
					*reinterpret_cast<uint32 *>(dest.GetBuffer() + prop_offset + 12) = data;
				}
				obj.WasSerialized();
				return base_offset;
			}
			template <class S> void ReadSimpleArray(PropertyInfo & prop, const uint8 * data, uint32 volume)
			{
				auto & arr = prop.Get< Array<S> >();
				arr.SetLength(volume);
				MemoryCopy(arr.GetBuffer(), data, volume * sizeof(S));
			}
			template <class S> void ReadSimpleSafeArray(PropertyInfo & prop, const uint8 * data, uint32 volume)
			{
				auto & arr = prop.Get< SafeArray<S> >();
				arr.Clear();
				for (uint i = 0; i < volume; i++) arr << *reinterpret_cast<const S*>(data + i * sizeof(S));
			}
			void DeserializeStructure(Reflected & obj, const uint8 * from, uint from_length, uint at)
			{
				obj.WillBeDeserialized();
				if (at + 4 <= from_length) {
					uint32 setters_count = *reinterpret_cast<const uint32 *>(from + at);
					if (at + 4 + setters_count * 16 <= from_length) for (uint i = 0; i < setters_count; i++) {
						uint32 name_ref = *reinterpret_cast<const uint32 *>(from + at + 4 + 16 * i);
						uint32 type_code = *reinterpret_cast<const uint32 *>(from + at + 8 + 16 * i);
						uint32 volume = *reinterpret_cast<const uint32 *>(from + at + 12 + 16 * i);
						uint32 value_ref = *reinterpret_cast<const uint32 *>(from + at + 16 + 16 * i);
						string name = string(from + name_ref, -1, Encoding::UTF8);
						PropertyInfo prop = obj.GetProperty(name);
						PropertyType type = static_cast<PropertyType>(type_code);
						PropertyType actual = prop.Type;
						if (actual == PropertyType::Array || actual == PropertyType::SafeArray) actual = prop.InnerType;
						if (prop.Address && type == actual) {
							if (volume) {
								if (volume == 0xFFFFFFFF) volume = 0;
								if (prop.Type == PropertyType::Array) {
									if (actual == PropertyType::String) {
										auto & arr = prop.Get< Array<string> >();
										arr.Clear();
										for (uint i = 0; i < volume; i++) {
											uint32 str_ref = *reinterpret_cast<const uint32 *>(from + value_ref + 4 * i);
											arr << string(from + str_ref, -1, Encoding::UTF8);
										}
									} else if (actual == PropertyType::Rectangle) {
										auto & arr = prop.Get< Array<UI::Rectangle> >();
										arr.Clear();
										for (uint i = 0; i < volume; i++) arr << reinterpret_cast<const Rectangle *>(from + value_ref + sizeof(Rectangle) * i)->ToRectangle();
									} else if (actual == PropertyType::Structure) {
										auto & arr = prop.Get<ReflectedArray>();
										arr.Clear();
										for (uint i = 0; i < volume; i++) {
											uint32 struct_ref = *reinterpret_cast<const uint32 *>(from + value_ref + 4 * i);
											arr.AppendNew();
											DeserializeStructure(arr.LastElement(), from, from_length, struct_ref);
										}
									} else {
										if (actual == PropertyType::UInt8) ReadSimpleArray<uint8>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Int8) ReadSimpleArray<int8>(prop, from + value_ref, volume);
										else if (actual == PropertyType::UInt16) ReadSimpleArray<uint16>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Int16) ReadSimpleArray<int16>(prop, from + value_ref, volume);
										else if (actual == PropertyType::UInt32) ReadSimpleArray<uint32>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Int32) ReadSimpleArray<int32>(prop, from + value_ref, volume);
										else if (actual == PropertyType::UInt64) ReadSimpleArray<uint64>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Int64) ReadSimpleArray<int64>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Float) ReadSimpleArray<float>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Double) ReadSimpleArray<double>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Complex) ReadSimpleArray<Math::Complex>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Boolean) ReadSimpleArray<bool>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Color) ReadSimpleArray<UI::Color>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Time) ReadSimpleArray<Time>(prop, from + value_ref, volume);
									}
								} else if (prop.Type == PropertyType::SafeArray) {
									if (actual == PropertyType::String) {
										auto & arr = prop.Get< SafeArray<string> >();
										arr.Clear();
										for (uint i = 0; i < volume; i++) {
											uint32 str_ref = *reinterpret_cast<const uint32 *>(from + value_ref + 4 * i);
											arr << string(from + str_ref, -1, Encoding::UTF8);
										}
									} else if (actual == PropertyType::Rectangle) {
										auto & arr = prop.Get< SafeArray<UI::Rectangle> >();
										arr.Clear();
										for (uint i = 0; i < volume; i++) arr << reinterpret_cast<const Rectangle *>(from + value_ref + sizeof(Rectangle) * i)->ToRectangle();
									} else if (actual == PropertyType::Structure) {
										auto & arr = prop.Get<ReflectedArray>();
										arr.Clear();
										for (uint i = 0; i < volume; i++) {
											uint32 struct_ref = *reinterpret_cast<const uint32 *>(from + value_ref + 4 * i);
											arr.AppendNew();
											DeserializeStructure(arr.LastElement(), from, from_length, struct_ref);
										}
									} else {
										if (actual == PropertyType::UInt8) ReadSimpleSafeArray<uint8>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Int8) ReadSimpleSafeArray<int8>(prop, from + value_ref, volume);
										else if (actual == PropertyType::UInt16) ReadSimpleSafeArray<uint16>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Int16) ReadSimpleSafeArray<int16>(prop, from + value_ref, volume);
										else if (actual == PropertyType::UInt32) ReadSimpleSafeArray<uint32>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Int32) ReadSimpleSafeArray<int32>(prop, from + value_ref, volume);
										else if (actual == PropertyType::UInt64) ReadSimpleSafeArray<uint64>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Int64) ReadSimpleSafeArray<int64>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Float) ReadSimpleSafeArray<float>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Double) ReadSimpleSafeArray<double>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Complex) ReadSimpleSafeArray<Math::Complex>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Boolean) ReadSimpleSafeArray<bool>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Color) ReadSimpleSafeArray<UI::Color>(prop, from + value_ref, volume);
										else if (actual == PropertyType::Time) ReadSimpleSafeArray<Time>(prop, from + value_ref, volume);
									}
								} else {
									uint32 vol_read = min(volume, uint32(prop.Volume));
									if (actual == PropertyType::String) {
										for (uint i = 0; i < vol_read; i++) {
											uint32 str_ref = *reinterpret_cast<const uint32 *>(from + value_ref + 4 * i);
											prop.VolumeElement(i).Set<string>(string(from + str_ref, -1, Encoding::UTF8));
										}
									} else if (actual == PropertyType::Rectangle) {
										for (uint i = 0; i < vol_read; i++) {
											prop.VolumeElement(i).Set<UI::Rectangle>(reinterpret_cast<const Rectangle *>(from + value_ref + sizeof(Rectangle) * i)->ToRectangle());
										}
									} else {
										MemoryCopy(prop.Address, from + value_ref, vol_read * prop.ElementSize);
									}
								}
							} else {
								if (actual == PropertyType::String) {
									prop.Set<string>(string(from + value_ref, -1, Encoding::UTF8));
								} else if (actual == PropertyType::Rectangle) {
									prop.Set<UI::Rectangle>(reinterpret_cast<const Rectangle *>(from + value_ref)->ToRectangle());
								} else if (actual == PropertyType::Structure) {
									DeserializeStructure(prop.Get<Reflected>(), from, from_length, value_ref);
								} else {
									MemoryCopy(prop.Address, from + value_ref, prop.ElementSize);
								}
							}
						}
					}
				}
				obj.WasDeserialized();
			}
		}
		BinarySerializer::BinarySerializer(Streaming::Stream * stream, int length) : BinarySerializer() { SetData(stream, length); }
		BinarySerializer::BinarySerializer(const void * data, int length) : BinarySerializer() { SetData(data, length); }
		BinarySerializer::BinarySerializer(void) { _data = new Array<uint8>(0x10000); }
		BinarySerializer::~BinarySerializer(void) {}
		void BinarySerializer::SerializeObject(Reflected & obj)
		{
			_data->SetLength(0);
			Array<EngineObjectInternal::StringCacheEntry> cache(0x1000);
			EngineObjectInternal::SerializeStructure(obj, *_data, cache);
		}
		void BinarySerializer::DeserializeObject(Reflected & obj) { EngineObjectInternal::DeserializeStructure(obj, _data->GetBuffer(), _data->Length(), 0); }
		Array<uint8>* BinarySerializer::GetData(void) { return _data; }
		void BinarySerializer::SetData(Streaming::Stream * stream, int length)
		{
			_data->SetLength(length);
			stream->Read(_data->GetBuffer(), length);
		}
		void BinarySerializer::SetData(const void * data, int length)
		{
			_data->SetLength(length);
			if (length) MemoryCopy(_data->GetBuffer(), data, length);
		}
		void SerializeToBinaryObject(Reflected & object, Streaming::Stream * dest)
		{
			BinarySerializer serializer;
			object.Serialize(&serializer);
			dest->Write("eobject", 8);
			uint32 value = 0;
			dest->Write(&value, 4);
			value = serializer.GetData()->Length();
			dest->Write(&value, 4);
			dest->WriteArray(serializer.GetData());
		}
		void RestoreFromBinaryObject(Reflected & object, Streaming::Stream * from)
		{
			char sign[8];
			from->Read(&sign, 8);
			if (MemoryCompare(&sign, "eobject", 8)) throw InvalidFormatException();
			uint32 value;
			from->Read(&value, 4);
			if (value) throw InvalidFormatException();
			from->Read(&value, 4);
			BinarySerializer serializer(from, int(value));
			object.Deserialize(&serializer);
		}
	}
}