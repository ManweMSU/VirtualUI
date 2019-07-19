#include "TextRegistry.h"

#include "../Miscellaneous/DynamicString.h"
#include "TextRegistryGrammar.h"

namespace Engine
{
	namespace Storage
	{
		template <class O> void EncodeRegistryNodeToText(RegistryNode * node, O & output, string prefix, const Syntax::Spelling & spelling)
		{
			auto & values = node->GetValues();
			for (int i = 0; i < values.Length(); i++) {
				string name = values[i];
				bool illegal = false;
				if (name[0] >= L'0' && name[0] <= L'9') illegal = true;
				else if (name == L"float_infinity" || name == L"float_nan") illegal = true;
				else for (int i = 0; i < name.Length(); i++) {
					if ((name[i] < L'a' || name[i] > L'z') && (name[i] < L'A' || name[i] > L'Z') &&
						(name[i] < L'0' || name[i] > L'9') && name[i] != L'_') { illegal = true; break; }
				}
				if (illegal) name = L"\"" + Syntax::FormatStringToken(name) + L"\"";
				auto type = node->GetValueType(values[i]);
				if (type == RegistryValueType::Integer) {
					output << prefix + name + L" = " + string(node->GetValueInteger(values[i])) + IO::NewLineChar;
				} else if (type == RegistryValueType::Float) {
					output << prefix + name + L" = " + Syntax::FormatFloatToken(node->GetValueFloat(values[i]), spelling) + IO::NewLineChar;
				} else if (type == RegistryValueType::Boolean) {
					string notation = node->GetValueBoolean(values[i]) ? L"true" : L"false";
					output << prefix + name + L" = " + notation + IO::NewLineChar;
				} else if (type == RegistryValueType::String) {
					string notation = L"\"" + Syntax::FormatStringToken(node->GetValueString(values[i])) + L"\"";
					output << prefix + name + L" = " + notation + IO::NewLineChar;
				} else if (type == RegistryValueType::LongInteger) {
					output << prefix + name + L" = long " + string(node->GetValueLongInteger(values[i])) + IO::NewLineChar;
				} else if (type == RegistryValueType::LongFloat) {
					output << prefix + name + L" = long " + Syntax::FormatDoubleToken(node->GetValueLongFloat(values[i]), spelling) + IO::NewLineChar;
				} else if (type == RegistryValueType::Color) {
					output << prefix + name + L" = color 0x" + string(node->GetValueColor(values[i]), L"0123456789ABCDEF", 8) + IO::NewLineChar;
				} else if (type == RegistryValueType::Time) {
					auto value = node->GetValueTime(values[i]);
					uint32 y, m, d;
					value.GetDate(y, m, d);
					output << prefix + name + L" = time { " +
						string(y) + L" " + string(m, L"0123456789", 2) + L" " + string(d, L"0123456789", 2) + L" " +
						string(value.GetHour(), L"0123456789", 2) + L" " + string(value.GetMinute(), L"0123456789", 2) + L" " +
						string(value.GetSecond(), L"0123456789", 2) + L" " + string(value.GetMillisecond(), L"0123456789", 4) +
						L" }" + IO::NewLineChar;
				} else if (type == RegistryValueType::Binary) {
					output << prefix + name + L" = binary {";
					Array<uint8> data;
					data.SetLength(node->GetValueBinarySize(values[i]));
					node->GetValueBinary(values[i], data.GetBuffer());
					for (int i = 0; i < data.Length(); i++) {
						if (i % 16 == 0) output << IO::NewLineChar + prefix + L"\t"; else output << L" ";
						output << L"0x" + string(uint32(data[i]), L"0123456789ABCDEF", 2);
					}
					if (data.Length()) output << IO::NewLineChar;
					output << prefix + L"}" + IO::NewLineChar;
				}
			}
			auto & nodes = node->GetSubnodes();
			for (int i = 0; i < nodes.Length(); i++) {
				string name = nodes[i];
				bool illegal = false;
				if (name[0] >= L'0' && name[0] <= L'9') illegal = true;
				else if (name == L"float_infinity" || name == L"float_nan") illegal = true;
				else for (int i = 0; i < name.Length(); i++) {
					if ((name[i] < L'a' || name[i] > L'z') && (name[i] < L'A' || name[i] > L'Z') &&
						(name[i] < L'0' || name[i] > L'9') && name[i] != L'_') {
						illegal = true; break;
					}
				}
				if (illegal) name = L"\"" + Syntax::FormatStringToken(name) + L"\"";
				SafePointer<RegistryNode> entry = node->OpenNode(nodes[i]);
				output << prefix + name + L" {" + IO::NewLineChar;
				EncodeRegistryNodeToText(entry, output, prefix + L"\t", spelling);
				output << prefix + L"}" + IO::NewLineChar;
			}
		}
		string RegistryToText(Registry * registry)
		{
			DynamicString result;
			Syntax::Spelling spelling;
			Storage::CreateTextRegistrySpelling(spelling);
			EncodeRegistryNodeToText(registry, result, L"", spelling);
			return result.ToString();
		}
		void RegistryToText(Registry * registry, Streaming::TextWriter * output)
		{
			Syntax::Spelling spelling;
			Storage::CreateTextRegistrySpelling(spelling);
			EncodeRegistryNodeToText(registry, *output, L"", spelling);
		}
		void RegistryToText(Registry * registry, Streaming::Stream * output, Encoding encoding)
		{
			Streaming::TextWriter writer(output, encoding);
			writer.WriteEncodingSignature();
			RegistryToText(registry, &writer);
		}

		void FillNodeWithTreeData(Syntax::SyntaxTreeNode & tree_node, RegistryNode * reg_node)
		{
			for (int i = 0; i < tree_node.Subnodes.Length(); i += 2) {
				string entity_name;
				if (tree_node.Subnodes[i].Expands.Class == Syntax::TokenClass::Constant) {
					if (tree_node.Subnodes[i].Expands.ValueClass == Syntax::TokenConstantClass::Boolean) {
						entity_name = tree_node.Subnodes[i].Expands.AsBoolean() ? L"true" : L"false";
					} else if (tree_node.Subnodes[i].Expands.ValueClass == Syntax::TokenConstantClass::Numeric) {
						if (tree_node.Subnodes[i].Expands.NumericClass() == Syntax::NumericTokenClass::Integer) {
							entity_name = string(tree_node.Subnodes[i].Expands.AsInteger());
						} else {
							entity_name = string(tree_node.Subnodes[i].Expands.AsFloat(), L'.');
						}
					} else if (tree_node.Subnodes[i].Expands.ValueClass == Syntax::TokenConstantClass::String) {
						entity_name = tree_node.Subnodes[i].Expands.Content;
					}
				} else if (tree_node.Subnodes[i].Expands.Class == Syntax::TokenClass::Identifier) {
					entity_name = tree_node.Subnodes[i].Expands.Content;
				} else if (tree_node.Subnodes[i].Expands.Class == Syntax::TokenClass::Keyword) {
					entity_name = tree_node.Subnodes[i].Expands.Content;
				}
				if (!entity_name.Length()) throw Exception();
				if (tree_node.Subnodes[i + 1].Label == L"NODEDEF") {
					reg_node->CreateNode(entity_name);
					SafePointer<RegistryNode> subnode = reg_node->OpenNode(entity_name);
					FillNodeWithTreeData(tree_node.Subnodes[i + 1].Subnodes[1], subnode);
				} else {
					auto & valnode = tree_node.Subnodes[i + 1].Subnodes[1].Subnodes[0];
					if (valnode.Label == L"INTDEF") {
						bool neg = valnode.Subnodes[0].Expands.Class == Syntax::TokenClass::CharCombo;
						auto & absnode = neg ? valnode.Subnodes[1] : valnode.Subnodes[0];
						if (absnode.Expands.NumericClass() == Syntax::NumericTokenClass::Integer) {
							auto val = absnode.Expands.AsInteger();
							if (val > 0xFFFFFFFF) throw Exception();
							reg_node->CreateValue(entity_name, RegistryValueType::Integer);
							reg_node->SetValue(entity_name, int32(neg ? (-int64(val)) : int64(val)));
						} else {
							auto val = absnode.Expands.AsFloat();
							reg_node->CreateValue(entity_name, RegistryValueType::Float);
							reg_node->SetValue(entity_name, neg ? (-val) : val);
						}
					} else if (valnode.Label == L"BOOLDEF") {
						reg_node->CreateValue(entity_name, RegistryValueType::Boolean);
						reg_node->SetValue(entity_name, valnode.Subnodes[0].Expands.AsBoolean());
					} else if (valnode.Label == L"STRDEF") {
						reg_node->CreateValue(entity_name, RegistryValueType::String);
						reg_node->SetValue(entity_name, valnode.Subnodes[0].Expands.Content);
					} else if (valnode.Label == L"LINTDEF") {
						bool neg = valnode.Subnodes[1].Expands.Class == Syntax::TokenClass::CharCombo;
						auto & absnode = neg ? valnode.Subnodes[2] : valnode.Subnodes[1];
						if (absnode.Expands.NumericClass() == Syntax::NumericTokenClass::Integer) {
							auto val = absnode.Expands.AsInteger();
							reg_node->CreateValue(entity_name, RegistryValueType::LongInteger);
							reg_node->SetValue(entity_name, neg ? (-int64(val)) : int64(val));
						} else {
							auto val = absnode.Expands.AsDouble();
							reg_node->CreateValue(entity_name, RegistryValueType::LongFloat);
							reg_node->SetValue(entity_name, neg ? (-val) : val);
						}
					} else if (valnode.Label == L"COLORDEF") {
						auto val = valnode.Subnodes[1].Expands.AsInteger();
						if (val > 0xFFFFFFFF) throw Exception();
						reg_node->CreateValue(entity_name, RegistryValueType::Color);
						reg_node->SetValue(entity_name, UI::Color(uint32(val)));
					} else if (valnode.Label == L"TIMEDEF") {
						uint32 valarray[7] = { 0, 0, 0, 0, 0, 0, 0 };
						int r = 0;
						for (int j = valnode.Subnodes.Length() - 2; j > 1; j--) {
							auto val = valnode.Subnodes[j].Expands.AsInteger();
							if (val > 0xFFFFFFFF) throw Exception();
							valarray[r] = uint32(val);
							r++;
						}
						Time time(valarray[6], valarray[5], valarray[4], valarray[3], valarray[2], valarray[1], valarray[0]);
						reg_node->CreateValue(entity_name, RegistryValueType::Time);
						reg_node->SetValue(entity_name, time);
					} else if (valnode.Label == L"BINDEF") {
						Array<uint8> data(0x1000);
						for (int j = 2; j < valnode.Subnodes.Length() - 1; j++) {
							auto val = valnode.Subnodes[j].Expands.AsInteger();
							if (val > 0xFF) throw Exception();
							data << uint8(val);
						}
						reg_node->CreateValue(entity_name, RegistryValueType::Binary);
						reg_node->SetValue(entity_name, data.GetBuffer(), data.Length());
					}
				}
			}
		}
		Registry * CompileTextRegistry(const string & code)
		{
			SafePointer<Registry> registry = CreateRegistry();
			try {
				if (!registry) throw Exception();
				Syntax::Spelling spelling;
				Syntax::Grammar grammar;
				Storage::CreateTextRegistrySpelling(spelling);
				Storage::CreateTextRegistryGrammar(grammar);
				SafePointer< Array<Syntax::Token> > token_stream = Syntax::ParseText(code, spelling);
				Syntax::SyntaxTree tree(*token_stream, grammar);
				tree.Root.OptimizeNode();
				SafePointer<RegistryNode> root = registry->OpenNode(L"");
				if (!root) throw Exception();
				FillNodeWithTreeData(tree.Root, root);
			}
			catch (...) {
				return 0;
			}
			registry->Retain();
			return registry;
		}
		Registry * CompileTextRegistry(Streaming::TextReader * input) { return CompileTextRegistry(input->ReadAll()); }
		Registry * CompileTextRegistry(Streaming::Stream * input)
		{
			Streaming::TextReader reader(input);
			return CompileTextRegistry(&reader);
		}
		Registry * CompileTextRegistry(Streaming::Stream * input, Encoding encoding)
		{
			Streaming::TextReader reader(input, encoding);
			return CompileTextRegistry(&reader);
		}
	}
}