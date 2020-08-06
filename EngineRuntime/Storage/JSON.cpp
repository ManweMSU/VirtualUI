#include "JSON.h"

#include "../Miscellaneous/DynamicString.h"
#include "../Syntax/Grammar.h"

using namespace Engine::Streaming;

namespace Engine
{
	namespace Reflection
	{
		namespace JsonInternal {
			string FormatFloatTokenJSON(float value)
			{
				if (Math::IsInfinity(value) || Math::IsNaN(value)) return L"0";
				else return string(value);
			}
			string FormatFloatTokenJSON(double value)
			{
				if (Math::IsInfinity(value) || Math::IsNaN(value)) return L"0";
				else return string(value);
			}
			string FormatStringTokenJSON(const string & input)
			{
				int ucslen = input.GetEncodedLength(Encoding::UTF32);
				Array<uint32> ucs;
				ucs.SetLength(ucslen);
				input.Encode(ucs, Encoding::UTF32, false);
				DynamicString result;
				for (int i = 0; i < ucs.Length(); i++) {
					if (ucs[i] < 0x20 || ucs[i] == L'\\' || ucs[i] == L'\"') {
						if (ucs[i] == L'\\') {
							result += L"\\\\";
						} else if (ucs[i] == L'\"') {
							result += L"\\\"";
						} else if (ucs[i] == L'\n') {
							result += L"\\n";
						} else if (ucs[i] == L'\r') {
							result += L"\\r";
						} else if (ucs[i] == L'\t') {
							result += L"\\t";
						} else {
							result += L"\\u" + string(ucs[i], HexadecimalBase, 4);
						}
					} else {
						result += widechar(ucs[i]);
					}
				}
				return result.ToString();
			}
			class JsonPropertyWriter : public IPropertyEnumerator
			{
			public:
				DynamicString ps;
				const string * pref;
				bool first = true;
				void WriteSimpleProperty(PropertyInfo & pi)
				{
					if (pi.Type == PropertyType::UInt8) {
						ps << string(int32(pi.Get<uint8>()));
					} else if (pi.Type == PropertyType::Int8) {
						ps << string(int32(pi.Get<int8>()));
					} else if (pi.Type == PropertyType::UInt16) {
						ps << string(int32(pi.Get<uint16>()));
					} else if (pi.Type == PropertyType::Int16) {
						ps << string(int32(pi.Get<int16>()));
					} else if (pi.Type == PropertyType::UInt32) {
						ps << string(pi.Get<uint32>());
					} else if (pi.Type == PropertyType::Int32) {
						ps << string(pi.Get<int32>());
					} else if (pi.Type == PropertyType::UInt64) {
						ps << string(pi.Get<uint64>());
					} else if (pi.Type == PropertyType::Int64) {
						ps << string(pi.Get<int64>());
					} else if (pi.Type == PropertyType::Float) {
						ps << FormatFloatTokenJSON(pi.Get<float>());
					} else if (pi.Type == PropertyType::Double) {
						ps << FormatFloatTokenJSON(pi.Get<double>());
					} else if (pi.Type == PropertyType::Complex) {
						auto & v = pi.Get<Math::Complex>();
						ps << L"{ \"re\": " << FormatFloatTokenJSON(v.re) << L", \"im\": " << FormatFloatTokenJSON(v.im) << L" }";
					} else if (pi.Type == PropertyType::Boolean) {
						auto v = pi.Get<bool>();
						ps << (v ? L"true" : L"false");
					} else if (pi.Type == PropertyType::String) {
						ps << L"\"" << FormatStringTokenJSON(pi.Get<string>()) << L"\"";
					} else if (pi.Type == PropertyType::Time) {
						ps << string(pi.Get<Time>());
					} else if (pi.Type == PropertyType::Color) {
						auto & v = pi.Get<UI::Color>();
						ps << L"{ \"r\": " << string(int32(v.r)) << L", \"g\": " << string(int32(v.g)) << L", \"b\": " << string(int32(v.b)) << L", \"a\": " << string(int32(v.a)) << L" }";
					} else if (pi.Type == PropertyType::Rectangle) {
						auto & v = pi.Get<UI::Rectangle>();
						ps << L"{ \"la\": " << string(v.Left.Absolute) << L", \"ls\": " << FormatFloatTokenJSON(v.Left.Zoom) << L", \"lm\": " << FormatFloatTokenJSON(v.Left.Anchor) <<
							L", \"ta\": " << string(v.Top.Absolute) << L", \"ts\": " << FormatFloatTokenJSON(v.Top.Zoom) << L", \"tm\": " << FormatFloatTokenJSON(v.Top.Anchor) <<
							L", \"ra\": " << string(v.Right.Absolute) << L", \"rs\": " << FormatFloatTokenJSON(v.Right.Zoom) << L", \"rm\": " << FormatFloatTokenJSON(v.Right.Anchor) <<
							L", \"ba\": " << string(v.Bottom.Absolute) << L", \"bs\": " << FormatFloatTokenJSON(v.Bottom.Zoom) << L", \"bm\": " << FormatFloatTokenJSON(v.Bottom.Anchor) << L" }";
					} else {
						ps << L"null";
					}
				}
				virtual void EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size) override;
			};
			void JsonWriteStructure(TextWriter * writer, const string & pref, Reflected & obj)
			{
				obj.WillBeSerialized();
				string inner_pref = pref + L"    ";
				writer->WriteLine(L"{");
				JsonPropertyWriter pw;
				pw.pref = &inner_pref;
				obj.EnumerateProperties(pw);
				pw.ps << IO::NewLineChar;
				writer->Write(pw.ps);
				writer->Write(pref + L"}");
				obj.WasSerialized();
			}
			void JsonWriteStructure(DynamicString & writer, const string & pref, Reflected & obj)
			{
				obj.WillBeSerialized();
				string inner_pref = pref + L"    ";
				writer << L"{" << IO::NewLineChar;
				JsonPropertyWriter pw;
				pw.pref = &inner_pref;
				obj.EnumerateProperties(pw);
				pw.ps << IO::NewLineChar;
				writer << pw.ps;
				writer << pref << L"}";
				obj.WasSerialized();
			}
			void JsonPropertyWriter::EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size)
			{
				if (first) {
					first = false;
				} else {
					ps << L"," << IO::NewLineChar;
				}
				ps << *pref;
				ps << L"\"" << name << L"\": ";
				PropertyInfo pi = { address, type, inner, volume, element_size, name };
				if (volume == 1) {
					if (pi.Type == PropertyType::Array) {
						if (pi.InnerType == PropertyType::UInt8) {
							auto & v = pi.Get< Array<uint8> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Int8) {
							auto & v = pi.Get< Array<int8> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::UInt16) {
							auto & v = pi.Get< Array<uint16> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Int16) {
							auto & v = pi.Get< Array<int16> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::UInt32) {
							auto & v = pi.Get< Array<uint32> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Int32) {
							auto & v = pi.Get< Array<int32> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::UInt64) {
							auto & v = pi.Get< Array<uint64> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Int64) {
							auto & v = pi.Get< Array<int64> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Float) {
							auto & v = pi.Get< Array<float> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Double) {
							auto & v = pi.Get< Array<double> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Complex) {
							auto & v = pi.Get< Array<Math::complex> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Boolean) {
							auto & v = pi.Get< Array<bool> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::String) {
							auto & v = pi.Get< Array<string> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Color) {
							auto & v = pi.Get< Array<UI::Color> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Time) {
							auto & v = pi.Get< Array<Time> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Rectangle) {
							auto & v = pi.Get< Array<UI::Rectangle> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { v.GetBuffer() + i, inner, PropertyType::Unknown, 1, sizeof(*v.GetBuffer()), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Structure) {
							auto & v = pi.Get< ReflectedArray >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; auto & obj = v.ElementAt(i); JsonWriteStructure(ps, *pref + L"    ", obj); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else {
							ps << L"null";
						}
					} else if (pi.Type == PropertyType::SafeArray) {
						if (pi.InnerType == PropertyType::UInt8) {
							auto & v = pi.Get< SafeArray<uint8> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Int8) {
							auto & v = pi.Get< SafeArray<int8> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::UInt16) {
							auto & v = pi.Get< SafeArray<uint16> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Int16) {
							auto & v = pi.Get< SafeArray<int16> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::UInt32) {
							auto & v = pi.Get< SafeArray<uint32> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Int32) {
							auto & v = pi.Get< SafeArray<int32> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::UInt64) {
							auto & v = pi.Get< SafeArray<uint64> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Int64) {
							auto & v = pi.Get< SafeArray<int64> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Float) {
							auto & v = pi.Get< SafeArray<float> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Double) {
							auto & v = pi.Get< SafeArray<double> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Complex) {
							auto & v = pi.Get< SafeArray<Math::complex> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Boolean) {
							auto & v = pi.Get< SafeArray<bool> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::String) {
							auto & v = pi.Get< SafeArray<string> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Color) {
							auto & v = pi.Get< SafeArray<UI::Color> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Time) {
							auto & v = pi.Get< SafeArray<Time> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Rectangle) {
							auto & v = pi.Get< SafeArray<UI::Rectangle> >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; PropertyInfo spi = { &v[i], inner, PropertyType::Unknown, 1, sizeof(v[i]), L"" }; WriteSimpleProperty(spi); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else if (pi.InnerType == PropertyType::Structure) {
							auto & v = pi.Get< ReflectedArray >();
							ps << L"[" << IO::NewLineChar;
							for (int i = 0; i < v.Length(); i++) { ps << *pref << L"    "; auto & obj = v.ElementAt(i); JsonWriteStructure(ps, *pref + L"    ", obj); if (i < v.Length() - 1) ps << L","; ps << IO::NewLineChar; }
							ps << *pref << L"]";
						} else {
							ps << L"null";
						}
					} else if (pi.Type == PropertyType::Structure) {
						auto & obj = pi.Get<Reflected>();
						JsonWriteStructure(ps, *pref, obj);
					} else WriteSimpleProperty(pi);
				} else {
					ps << L"[" << IO::NewLineChar;
					for (int i = 0; i < volume; i++) {
						ps << *pref << L"    ";
						auto pl = pi.VolumeElement(i);
						WriteSimpleProperty(pl);
						if (i < volume - 1) ps << L",";
						ps << IO::NewLineChar;
					}
					ps << *pref << L"]";
				}
			}
			ENGINE_REFLECTED_CLASS(ComplexWrapper, Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, re)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, im)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(ColorWrapper, Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(UINT8, r)
				ENGINE_DEFINE_REFLECTED_PROPERTY(UINT8, g)
				ENGINE_DEFINE_REFLECTED_PROPERTY(UINT8, b)
				ENGINE_DEFINE_REFLECTED_PROPERTY(UINT8, a)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(RectangleWrapper, Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, la)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, ls)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, lm)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ta)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, ts)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, tm)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ra)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, rs)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, rm)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ba)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, bs)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, bm)
			ENGINE_END_REFLECTED_CLASS
			void JsonAnalyzeStructure(Reflected & obj, Syntax::SyntaxTreeNode & node);
			uint64 JsonAnalyzeInteger(Syntax::SyntaxTreeNode & node)
			{
				if (node.Subnodes[0].Label == L"NUMBER") {
					bool neg = false;
					uint64 val = 0;
					if (node.Subnodes[0].Subnodes[0].Expands.Class == Syntax::TokenClass::CharCombo) {
						neg = true;
						val = node.Subnodes[0].Subnodes[1].Expands.AsInteger();
					} else val = node.Subnodes[0].Subnodes[0].Expands.AsInteger();
					return neg ? uint64(-int64(val)) : val;
				} else return 0;
			}
			float JsonAnalyzeFloat(Syntax::SyntaxTreeNode & node)
			{
				if (node.Subnodes[0].Label == L"NUMBER") {
					bool neg = false;
					float val = 0.0f;
					if (node.Subnodes[0].Subnodes[0].Expands.Class == Syntax::TokenClass::CharCombo) {
						neg = true;
						val = node.Subnodes[0].Subnodes[1].Expands.AsFloat();
					} else val = node.Subnodes[0].Subnodes[0].Expands.AsFloat();
					return neg ? (-val) : val;
				} else return 0.0f;
			}
			Math::Complex JsonAnalyzeDouble(Syntax::SyntaxTreeNode & node)
			{
				if (node.Subnodes[0].Label == L"NUMBER") {
					bool neg = false;
					double val = 0.0;
					if (node.Subnodes[0].Subnodes[0].Expands.Class == Syntax::TokenClass::CharCombo) {
						neg = true;
						val = node.Subnodes[0].Subnodes[1].Expands.AsFloat();
					} else val = node.Subnodes[0].Subnodes[0].Expands.AsFloat();
					return neg ? (-val) : val;
				} else if (node.Subnodes[0].Label == L"STRUCT") {
					ComplexWrapper val;
					JsonAnalyzeStructure(val, node.Subnodes[0]);
					return Math::Complex(val.re, val.im);
				} else return 0.0;
			}
			UI::Color JsonAnalyzeColor(Syntax::SyntaxTreeNode & node)
			{
				if (node.Subnodes[0].Label == L"STRUCT") {
					ColorWrapper val;
					JsonAnalyzeStructure(val, node.Subnodes[0]);
					return UI::Color(val.r, val.g, val.b, val.a);
				} else return 0;
			}
			UI::Rectangle JsonAnalyzeRectangle(Syntax::SyntaxTreeNode & node)
			{
				if (node.Subnodes[0].Label == L"STRUCT") {
					RectangleWrapper val;
					JsonAnalyzeStructure(val, node.Subnodes[0]);
					return UI::Rectangle(UI::Coordinate(val.la, val.ls, val.lm), UI::Coordinate(val.ta, val.ts, val.tm), UI::Coordinate(val.ra, val.rs, val.rm), UI::Coordinate(val.ba, val.bs, val.bm));
				} else return UI::Rectangle::Invalid();
			}
			bool JsonAnalyzeBoolean(Syntax::SyntaxTreeNode & node)
			{
				if (node.Subnodes[0].Label == L"BOOLEAN") {
					return node.Subnodes[0].Expands.AsBoolean();
				} else return false;
			}
			string JsonAnalyzeString(Syntax::SyntaxTreeNode & node)
			{
				if (node.Subnodes[0].Label == L"STRING") {
					return node.Subnodes[0].Expands.Content;
				} else return L"";
			}
#define ENUM_DYNAMIC_ARRAY_ITEMS(a) for (int i = 0; i < a.Subnodes.Length(); i++) if (a.Subnodes[i].Label == L"VALUE")
#define ENUM_STATIC_ARRAY_ITEMS(vol, a) { int i = 0, j = 0; while(j < vol && i < a.Subnodes.Length()) { if (a.Subnodes[i].Label == L"VALUE") {
#define END_ENUM_STATIC_ARRAY_ITEMS j++; } i++; } }
#define GET_DYNAMIC_ARRAY_OBJECT(a) a.Subnodes[i]
#define GET_STATIC_ARRAY_FIELD(p) p.VolumeElement(j)
			void JsonAnalyzeStructure(Reflected & obj, Syntax::SyntaxTreeNode & node)
			{
				obj.WillBeDeserialized();
				int ni = -1;
				int vi = -1;
				while (true) {
					do { ni++; } while (ni < node.Subnodes.Length() && node.Subnodes[ni].Label != L"NAME");
					do { vi++; } while (vi < node.Subnodes.Length() && node.Subnodes[vi].Label != L"VALUE");
					if (ni >= node.Subnodes.Length()) break;
					string name = node.Subnodes[ni].Expands.Content;
					Syntax::SyntaxTreeNode & value = node.Subnodes[vi];
					PropertyInfo pi = obj.GetProperty(name);
					if (pi.Address) {
						if (pi.Volume == 1) {
							if (pi.Type == PropertyType::UInt8) {
								pi.Set<uint8>(uint8(JsonAnalyzeInteger(value)));
							} else if (pi.Type == PropertyType::Int8) {
								pi.Set<int8>(int8(JsonAnalyzeInteger(value)));
							} else if (pi.Type == PropertyType::UInt16) {
								pi.Set<uint16>(uint16(JsonAnalyzeInteger(value)));
							} else if (pi.Type == PropertyType::Int16) {
								pi.Set<int16>(int16(JsonAnalyzeInteger(value)));
							} else if (pi.Type == PropertyType::UInt32) {
								pi.Set<uint32>(uint32(JsonAnalyzeInteger(value)));
							} else if (pi.Type == PropertyType::Int32) {
								pi.Set<int32>(int32(JsonAnalyzeInteger(value)));
							} else if (pi.Type == PropertyType::UInt64) {
								pi.Set<uint64>(uint64(JsonAnalyzeInteger(value)));
							} else if (pi.Type == PropertyType::Int64) {
								pi.Set<int64>(int64(JsonAnalyzeInteger(value)));
							} else if (pi.Type == PropertyType::Float) {
								pi.Set<float>(JsonAnalyzeFloat(value));
							} else if (pi.Type == PropertyType::Double) {
								pi.Set<double>(JsonAnalyzeDouble(value).re);
							} else if (pi.Type == PropertyType::Complex) {
								pi.Set<Math::Complex>(JsonAnalyzeDouble(value));
							} else if (pi.Type == PropertyType::Boolean) {
								pi.Set<bool>(JsonAnalyzeBoolean(value));
							} else if (pi.Type == PropertyType::String) {
								pi.Set<string>(JsonAnalyzeString(value));
							} else if (pi.Type == PropertyType::Color) {
								pi.Set<UI::Color>(JsonAnalyzeColor(value));
							} else if (pi.Type == PropertyType::Time) {
								pi.Set<Time>(uint64(JsonAnalyzeInteger(value)));
							} else if (pi.Type == PropertyType::Rectangle) {
								pi.Set<UI::Rectangle>(JsonAnalyzeRectangle(value));
							} else if (pi.Type == PropertyType::Array) {
								if (value.Subnodes[0].Label == L"ARRAY") {
									if (pi.InnerType == PropertyType::UInt8) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<uint8> >() << uint8(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Int8) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<int8> >() << int8(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::UInt16) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<uint16> >() << uint16(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Int16) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<int16> >() << int16(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::UInt32) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<uint32> >() << uint32(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Int32) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<int32> >() << int32(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::UInt64) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<uint64> >() << uint64(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Int64) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<int64> >() << int64(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Float) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<float> >() << JsonAnalyzeFloat(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Double) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<double> >() << JsonAnalyzeDouble(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])).re;
									} else if (pi.InnerType == PropertyType::Complex) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<Math::Complex> >() << JsonAnalyzeDouble(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Boolean) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<bool> >() << JsonAnalyzeBoolean(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::String) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<string> >() << JsonAnalyzeString(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Color) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<UI::Color> >() << JsonAnalyzeColor(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Time) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<Time> >() << uint64(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Rectangle) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< Array<UI::Rectangle> >() << JsonAnalyzeRectangle(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Structure) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) {
											pi.Get< ReflectedArray >().AppendNew();
											if (GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]).Subnodes[0].Label == L"STRUCT") {
												JsonAnalyzeStructure(pi.Get< ReflectedArray >().LastElement(), GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]).Subnodes[0]);
											}
										}
									}
								}
							} else if (pi.Type == PropertyType::SafeArray) {
								if (value.Subnodes[0].Label == L"ARRAY") {
									if (pi.InnerType == PropertyType::UInt8) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<uint8> >() << uint8(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Int8) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<int8> >() << int8(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::UInt16) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<uint16> >() << uint16(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Int16) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<int16> >() << int16(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::UInt32) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<uint32> >() << uint32(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Int32) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<int32> >() << int32(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::UInt64) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<uint64> >() << uint64(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Int64) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<int64> >() << int64(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Float) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<float> >() << JsonAnalyzeFloat(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Double) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<double> >() << JsonAnalyzeDouble(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])).re;
									} else if (pi.InnerType == PropertyType::Complex) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<Math::Complex> >() << JsonAnalyzeDouble(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Boolean) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<bool> >() << JsonAnalyzeBoolean(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::String) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<string> >() << JsonAnalyzeString(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Color) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<UI::Color> >() << JsonAnalyzeColor(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Time) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<Time> >() << uint64(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									} else if (pi.InnerType == PropertyType::Rectangle) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) pi.Get< SafeArray<UI::Rectangle> >() << JsonAnalyzeRectangle(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]));
									} else if (pi.InnerType == PropertyType::Structure) {
										ENUM_DYNAMIC_ARRAY_ITEMS(value.Subnodes[0]) {
											pi.Get< ReflectedArray >().AppendNew();
											if (GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]).Subnodes[0].Label == L"STRUCT") {
												JsonAnalyzeStructure(pi.Get< ReflectedArray >().LastElement(), GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]).Subnodes[0]);
											}
										}
									}
								}
							} else if (pi.Type == PropertyType::Structure) {
								if (value.Subnodes[0].Label == L"STRUCT") JsonAnalyzeStructure(pi.Get<Reflected>(), value.Subnodes[0]);
							}
						} else {
							if (value.Subnodes[0].Label == L"ARRAY") {
								if (pi.Type == PropertyType::UInt8) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<uint8>(uint8(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]))));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Int8) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<int8>(int8(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]))));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::UInt16) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<uint16>(uint16(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]))));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Int16) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<int16>(int16(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]))));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::UInt32) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<uint32>(uint32(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]))));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Int32) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<int32>(int32(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]))));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::UInt64) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<uint64>(uint64(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]))));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Int64) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<int64>(int64(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]))));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Float) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<float>(JsonAnalyzeFloat(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Double) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<double>(JsonAnalyzeDouble(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])).re);
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Complex) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<Math::Complex>(JsonAnalyzeDouble(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Boolean) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<bool>(JsonAnalyzeBoolean(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::String) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<string>(JsonAnalyzeString(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Color) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<UI::Color>(JsonAnalyzeColor(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Time) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<Time>(uint64(JsonAnalyzeInteger(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0]))));
									END_ENUM_STATIC_ARRAY_ITEMS
								} else if (pi.Type == PropertyType::Rectangle) {
									ENUM_STATIC_ARRAY_ITEMS(pi.Volume, value.Subnodes[0])
										GET_STATIC_ARRAY_FIELD(pi).Set<UI::Rectangle>(JsonAnalyzeRectangle(GET_DYNAMIC_ARRAY_OBJECT(value.Subnodes[0])));
									END_ENUM_STATIC_ARRAY_ITEMS
								}
							}
						}
					}
				}
				obj.WasDeserialized();
			}
		}
		JsonSerializer::JsonSerializer(Streaming::Stream * stream) : throw_on_syntax_error(false)
		{
			reader = new TextReader(stream, Encoding::UTF8);
			writer = new TextWriter(stream, Encoding::UTF8);
		}
		JsonSerializer::JsonSerializer(Streaming::Stream * stream, bool throw_on_syntax) : throw_on_syntax_error(throw_on_syntax)
		{
			reader = new TextReader(stream, Encoding::UTF8);
			writer = new TextWriter(stream, Encoding::UTF8);
		}
		JsonSerializer::~JsonSerializer(void) {}
		void JsonSerializer::SerializeObject(Reflected & obj)
		{
			JsonInternal::JsonWriteStructure(writer, L"", obj);
		}
		void JsonSerializer::DeserializeObject(Reflected & obj)
		{
			try {
				// Preparing
				PropertyZeroInitializer initializer;
				obj.EnumerateProperties(initializer);
				// Producing JSON grammar to analyze the input
				Syntax::Spelling json_spelling;
				json_spelling.BooleanFalseLiteral = L"false";
				json_spelling.BooleanTrueLiteral = L"true";
				json_spelling.Keywords << L"null";
				json_spelling.IsolatedChars << L'{';
				json_spelling.IsolatedChars << L'}';
				json_spelling.IsolatedChars << L'[';
				json_spelling.IsolatedChars << L']';
				json_spelling.IsolatedChars << L',';
				json_spelling.IsolatedChars << L':';
				json_spelling.IsolatedChars << L'-';
				Syntax::Grammar json_grammar;
				SafePointer<Syntax::Grammar::GrammarRule> struct_rule = new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(L"STRUCT"));
				struct_rule->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"{"));
				struct_rule->Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"", 0, 1);
				struct_rule->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"NAME", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::String));
				struct_rule->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L":"));
				struct_rule->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"VALUE");
				struct_rule->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"", 0, -1);
				struct_rule->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L","));
				struct_rule->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"NAME", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::String));
				struct_rule->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L":"));
				struct_rule->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"VALUE");
				struct_rule->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"}"));
				SafePointer<Syntax::Grammar::GrammarRule> value_rule = new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::VariantRule(L"VALUE"));
				value_rule->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"BOOLEAN", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Boolean));
				value_rule->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"UNSET", Syntax::Token::KeywordToken(L"null"));
				value_rule->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"STRING", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::String));
				value_rule->Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"NUMBER");
				value_rule->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"-"), 0, 1);
				value_rule->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
				value_rule->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"STRUCT");
				value_rule->Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"ARRAY");
				value_rule->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"["));
				value_rule->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"", 0, 1);
				value_rule->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"VALUE");
				value_rule->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"", 0, -1);
				value_rule->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L","));
				value_rule->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"VALUE");
				value_rule->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"]"));
				json_grammar.Rules.Append(struct_rule->Label, struct_rule);
				json_grammar.Rules.Append(value_rule->Label, value_rule);
				json_grammar.EntranceRule = L"STRUCT";
				// Parsing
				SafePointer< Array<Syntax::Token> > tokens = Syntax::ParseText(reader->ReadAll(), json_spelling);
				Syntax::SyntaxTree tree(*tokens, json_grammar);
				tree.Root.OptimizeNode();
				tokens.SetReference(0);
				// Handling
				JsonInternal::JsonAnalyzeStructure(obj, tree.Root);
			}
			catch (...) { if (throw_on_syntax_error) throw InvalidFormatException(); }
		}
	}
	constexpr widechar * Base64Digits = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	string ConvertToBase64(const void * data, int length)
	{
		auto src = reinterpret_cast<const uint8 *>(data);
		DynamicString result;
		for (int i = 0; i < length; i += 3) {
			uint32 number = uint32(src[i]) << 16;
			if (i + 1 < length) number |= uint32(src[i + 1]) << 8;
			if (i + 2 < length) number |= uint32(src[i + 2]);
			result += string(number, Base64Digits, 4);
			if (length - i == 2) {
				result[result.Length() - 1] = L'=';
			} else if (length - i == 1) {
				result[result.Length() - 1] = L'=';
				result[result.Length() - 2] = L'=';
			}
		}
		return result.ToString();
	}
	Array<uint8>* ConvertFromBase64(const string & data)
	{
		SafePointer< Array<uint8> > Result = new Array<uint8>(0x100);
		if ((data.Length() & 0x3) == 0) {
			for (int i = 0; i < data.Length(); i += 4) {
				string frag = data.Fragment(i, 4);
				uint32 number = 0;
				uint32 bytes = 3;
				for (int j = 0; j < frag.Length(); j++) if (frag[j] == L'=') bytes--;
				frag = frag.Replace(L'=', L'A');
				number = frag.ToUInt32(Base64Digits, true);
				if (bytes > 0) {
					Result->Append(uint8(number >> 16));
				}
				if (bytes > 1) {
					Result->Append(uint8(number >> 8));
				}
				if (bytes > 2) {
					Result->Append(uint8(number));
				}
			}
		}
		Result->Retain();
		return Result;
	}
}