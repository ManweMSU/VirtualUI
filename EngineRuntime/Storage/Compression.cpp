#include "Compression.h"

#include "SubbyteCoding.h"

namespace Engine
{
	namespace Storage
	{
		namespace Compression
		{
			class HuffmanTreeNode
			{
			public:
				uint32 value;
				uint32 frequency;
				HuffmanTreeNode * on0;
				HuffmanTreeNode * on1;
				Code sequence;
				HuffmanTreeNode(void) {}

				void Release(void) { if (on0) on0->Release(); if (on1) on1->Release(); delete this; }
				void BuildSubcodes(const Code & base)
				{
					sequence = base;
					Code code0 = sequence, code1 = sequence;
					code0.Append(0); code1.Append(1);
					if (on0) on0->BuildSubcodes(code0);
					if (on1) on1->BuildSubcodes(code1);
				}
				void BuildCodes()
				{
					sequence = Code();
					Code code0 = Code(), code1 = Code();
					code0.Append(0); code1.Append(1);
					if (on0) on0->BuildSubcodes(code0);
					if (on1) on1->BuildSubcodes(code1);
				}
				void WriteTree(BitStream & out, int depth = -1) const
				{
					if (on0) { on0->WriteTree(out, depth + 1); if (on1) on1->WriteTree(out, depth + 1); }
					else { out.Write(&value, 8); out.Write(&depth, 8); }
				}
				int IndeterminationLevel(void) const { if (!on0 && !on1) return 0; if (on0 && on1) return on0->IndeterminationLevel() + on1->IndeterminationLevel(); return 1; }
				void static AddNode(HuffmanTreeNode ** root, int code, int depth, int current_depth = -1)
				{
					if (*root == 0) {
						*root = new HuffmanTreeNode;
						(*root)->frequency = 0;
						(*root)->on0 = (*root)->on1 = 0;
						(*root)->value = 0;
					}
					if (depth == current_depth) {
						(*root)->value = code;
					} else {
						if (!(*root)->on0 || (*root)->on0->IndeterminationLevel()) AddNode(&(*root)->on0, code, depth, current_depth + 1);
						else AddNode(&(*root)->on1, code, depth, current_depth + 1);
					}
				}
			};
			Array<uint8> * HuffmanCompress(Array<uint8> * input)
			{
				HuffmanTreeNode ** Leaves = new (std::nothrow) HuffmanTreeNode * [256];
				HuffmanTreeNode ** Dictionary = new (std::nothrow) HuffmanTreeNode * [256];
				if (!Leaves || !Dictionary) { delete[] Leaves; delete[] Dictionary; throw OutOfMemoryException(); }
				for (int i = 0; i < 256; i++) {
					Leaves[i] = new (std::nothrow) HuffmanTreeNode;
					if (!Leaves[i]) {
						for (int j = 0; j < i; j++) delete Leaves[j];
						delete[] Leaves; delete[] Dictionary;
						throw OutOfMemoryException();
					}
					Leaves[i]->on0 = 0;
					Leaves[i]->on1 = 0;
					Leaves[i]->value = i;
					Leaves[i]->frequency = 0;
					Dictionary[i] = Leaves[i];
				}
				uint CodeNumber = 0;
				for (int i = 0; i < input->Length(); i++) Leaves[input->ElementAt(i)]->frequency++;
				for (int i = 0; i < 256; i++) if (!Leaves[i]->frequency) { delete Leaves[i]; Leaves[i] = 0; } else CodeNumber++;
				BitStream * Stream = new (std::nothrow) BitStream;
				if (!Stream) {
					for (int j = 0; j < 256; j++) delete Leaves[j];
					delete[] Leaves; delete[] Dictionary;
					throw OutOfMemoryException();
				}
				uint LeafCount = CodeNumber;
				HuffmanTreeNode * Root = 0;
				if (LeafCount > 1) {
					while (LeafCount > 1) {
						int min1 = -1, min2 = -1;
						uint freq1 = 0, freq2 = 0;
						for (int i = 0; i < 256; i++) {
							if (!Leaves[i]) continue;
							if (min1 == -1) {
								min1 = i;
								freq1 = Leaves[i]->frequency;
							} else if (min2 == -1) {
								min2 = i;
								freq2 = Leaves[i]->frequency;
							} else if (Leaves[i]->frequency < freq1) {
								if (freq1 < freq2) {
									min2 = min1;
									freq2 = freq1;
								}
								min1 = i;
								freq1 = Leaves[i]->frequency;
							} else if (Leaves[i]->frequency < freq2) {
								if (freq2 < freq1) {
									min1 = min2;
									freq1 = freq2;
								}
								min2 = i;
								freq2 = Leaves[i]->frequency;
							}
						}
						HuffmanTreeNode * New = new (std::nothrow) HuffmanTreeNode;
						if (!New) {
							for (int j = 0; j < 256; j++) if (Leaves[j]) Leaves[j]->Release();
							delete[] Leaves; delete[] Dictionary; Stream->Release();
							throw OutOfMemoryException();
						}
						New->frequency = freq1 + freq2;
						New->value = 0;
						New->on0 = Leaves[min1];
						New->on1 = Leaves[min2];
						Leaves[min1] = New;
						Leaves[min2] = 0;
						LeafCount--;
						if (LeafCount == 1) Root = New;
					}
				} else {
					for (int i = 0; i < 256; i++) if (Leaves[i]) {
						Root = Leaves[i];
						break;
					}
				}
				delete[] Leaves;
				try {
					uint32 length = input->Length();
					Root->BuildCodes();
					Stream->Write(&length, 32);
					Stream->Write(&CodeNumber, 8);
					Root->WriteTree(*Stream);
					for (int i = 0; i < input->Length(); i++) Stream->Write(Dictionary[input->ElementAt(i)]->sequence);
				} catch (...) { Root->Release(); delete[] Dictionary; Stream->Release(); throw; }
				Root->Release();
				delete[] Dictionary;
				auto Result = Stream->GetRetainedStorage();
				Stream->Release();
				return Result;
			}
			Array<uint8> * HuffmanDecompress(Array<uint8> * input)
			{
				BitStream Source(input);
				BitStream * Result = new (std::nothrow) BitStream;
				if (!Result) throw OutOfMemoryException();
				uint Length = Source.ReadDWord();
				uint64 LongLength = uint64(Length) * 8;
				uint CodeNumber = Source.ReadByte();
				if (CodeNumber == 0) CodeNumber = 256;
				HuffmanTreeNode * Root = 0;
				try {
					if (CodeNumber > 1) {
						for (uint i = 0; i < CodeNumber; i++) {
							uint Code = Source.ReadByte();
							uint Depth = Source.ReadByte();
							HuffmanTreeNode::AddNode(&Root, Code, Depth);
						}
					} else {
						uint Code = Source.ReadByte();
						uint Depth = Source.ReadByte();
						Root = new (std::nothrow) HuffmanTreeNode;
						if (!Root) throw OutOfMemoryException();
						Root->on0 = Root->on1 = 0;
						Root->frequency = 0;
						Root->value = Code;
					}
					if (CodeNumber == 1) {
						for (uint i = 0; i < Length; i++) Result->Write(&Root->value, 8);
					} else {
						HuffmanTreeNode * Current = Root;
						while (Result->Length() < LongLength) {
							if (!Current->on0) {
								Result->Write(&Current->value, 8);
								Current = Root;
							}
							if (Result->Length() < LongLength) {
								if (Source.Pointer() >= Source.Length()) break;
								int Bit = Source.ReadBit();
								Current = Bit ? Current->on1 : Current->on0;
							}
						}
					}
				}
				catch (...) { Result->Release(); if (Root) Root->Release(); throw; }
				Root->Release();
				auto Data = Result->GetRetainedStorage();
				Result->Release();
				return Data;
			}
			class LempelZivWelchDictionaryItem
			{
			public:
				Code Word;
				Code ReplaceWith;
			};
			class LempelZivWelchEncodingTree
			{
			public:
				LempelZivWelchDictionaryItem This;
				LempelZivWelchEncodingTree ** Hash;

				LempelZivWelchEncodingTree(void) : Hash(0) { Hash = 0; }
				LempelZivWelchEncodingTree(const LempelZivWelchEncodingTree & src) = delete;
				~LempelZivWelchEncodingTree(void) { if (Hash) { for (int i = 0; i < 256; i++) delete Hash[i]; delete[] Hash; } }
				void operator = (const LempelZivWelchEncodingTree & src) = delete;

				void AddItem(const LempelZivWelchDictionaryItem & Item, int Byte)
				{
					if (Byte >= Item.Word.Length() / 8) { This = Item; return; }
					int At = Item.Word.GetByte(Byte);
					if (!Hash) { Hash = new (std::nothrow) LempelZivWelchEncodingTree * [256]; if (!Hash) throw OutOfMemoryException(); ZeroMemory(Hash, sizeof(LempelZivWelchEncodingTree *) * 256); }
					if (!Hash[At]) { Hash[At] = new (std::nothrow) LempelZivWelchEncodingTree; if (!Hash[At]) throw OutOfMemoryException(); }
					Hash[At]->AddItem(Item, Byte + 1);
				}
				LempelZivWelchEncodingTree & operator << (const LempelZivWelchDictionaryItem & Item) { AddItem(Item, 0); return *this; }
				const LempelZivWelchDictionaryItem * ContainsWord(const Code & code, int ByteTest) const
				{
					if (ByteTest >= code.Length() / 8) {
						if (code == This.Word) return &This; else return 0;
					} else {
						int At = code.GetByte(ByteTest);
						if (!Hash) return 0;
						if (!Hash[At]) return 0;
						return Hash[At]->ContainsWord(code, ByteTest + 1);
					}
				}
				const LempelZivWelchDictionaryItem * ContainsWord(const Code & code) const { return ContainsWord(code, 0); }
				void Append(bool Bit)
				{
					This.ReplaceWith.Append(Bit);
					if (Hash) for (int i = 0; i < 256; i++) if (Hash[i]) Hash[i]->Append(Bit);
				}
				void Reset(void)
				{
					if (Hash) { for (int i = 0; i < 256; i++) delete Hash[i]; delete[] Hash; }
					Hash = 0;
					This = LempelZivWelchDictionaryItem();
				}
			};
			struct LempelZivWelchDecodingTree
			{
				Code Word;
				LempelZivWelchDecodingTree * On0;
				LempelZivWelchDecodingTree * On1;
				void Release() { if (On0) On0->Release(); if (On1) On1->Release(); delete this; }
				void Construct(int Length)
				{
					if (Length) {
						On0 = new (std::nothrow) LempelZivWelchDecodingTree;
						On1 = new (std::nothrow) LempelZivWelchDecodingTree;
						if (!On0 || !On1) { delete On0; delete On1; throw OutOfMemoryException(); }
						On0->Construct(Length - 1);
						On1->Construct(Length - 1);
					} else {
						On0 = On1 = 0;
					}
				}
				void Construct(void)
				{
					On0 = new (std::nothrow) LempelZivWelchDecodingTree;
					On1 = new (std::nothrow) LempelZivWelchDecodingTree;
					if (!On0 || !On1) { delete On0; delete On1; throw OutOfMemoryException(); }
					On0->Construct(8);
					On1->Construct(8);
				}
				void SetWord(const Code & Path, const Code & WordSet, int BitCheck = -2)
				{
					if (BitCheck == -2) BitCheck = Path.Length() - 1;
					if (BitCheck == -1) { Word = WordSet; return; }
					if (Path[BitCheck]) On1->SetWord(Path, WordSet, BitCheck - 1);
					else On0->SetWord(Path, WordSet, BitCheck - 1);
				}
				LempelZivWelchDecodingTree * GetWord(const Code & Path, int BitCheck = -2)
				{
					if (BitCheck == -2) BitCheck = Path.Length() - 1;
					if (BitCheck == -1) return this;
					if (Path[BitCheck]) return On1->GetWord(Path, BitCheck - 1);
					else return On0->GetWord(Path, BitCheck - 1);
				}
			};
			Array<uint8> * LempelZivWelchCompress(Array<uint8> * input)
			{
				LempelZivWelchEncodingTree Dictionary;
				uint Dimension = 512;
				uint Length = 256;
				uint BitsPerWord = 9;
				for (int i = 0; i < 256; i++) Dictionary << LempelZivWelchDictionaryItem{ Code(i, 8), Code(i, 9) };
				BitStream Source(input);
				BitStream * Result = new (std::nothrow) BitStream;
				if (!Result) throw OutOfMemoryException();
				try {
					Code Input;
					LempelZivWelchDictionaryItem Deployed;
					while (Source.Pointer() < Source.Length()) {
						const LempelZivWelchDictionaryItem * Node = 0, * Last = 0;
						Code Read;
						Node = Dictionary.ContainsWord(Input);
						do {
							Read = Code();
							Source.ReadCode(Read, 8);
							Input.Append(Read);
							Last = Node;
							Node = Dictionary.ContainsWord(Input);
						} while (Node && Source.Pointer() < Source.Length());
						if (Source.Pointer() < Source.Length()) {
							Result->Write(Last->ReplaceWith);
							if (Deployed.Word.Length()) {
								Dictionary << Deployed;
								Length++;
								if (Length == Dimension) {
									Dimension <<= 1;
									BitsPerWord++;
									Dictionary.Append(0);
								}
							}
							Deployed = LempelZivWelchDictionaryItem{ Input, Code(Length, BitsPerWord) };
						} else {
							if (Node != 0) {
								Result->Write(Node->ReplaceWith);
							} else {
								Result->Write(Last->ReplaceWith);
								if (Deployed.Word.Length()) {
									Dictionary << Deployed;
									Length++;
									if (Length == Dimension) {
										Dimension <<= 1;
										BitsPerWord++;
										Dictionary.Append(0);
									}
								}
								Node = Dictionary.ContainsWord(Read);
								Result->Write(Node->ReplaceWith);
							}
						}
						Input = Read;
					}
				}
				catch (...) { Result->Release(); throw; }
				auto Data = Result->GetRetainedStorage();
				Result->Release();
				return Data;
			}
			Array<uint8> * LempelZivWelchDecompress(Array<uint8> * input)
			{
				BitStream Source(input);
				BitStream * Result = new (std::nothrow) BitStream;
				if (!Result) throw OutOfMemoryException();
				uint Dimension = 512;
				uint Length = 256;
				uint BitsPerWord = 9;
				LempelZivWelchDecodingTree * Root = new (std::nothrow) LempelZivWelchDecodingTree();
				if (!Root) { Result->Release(); throw OutOfMemoryException(); }
				try {
					Root->Construct();
					for (int i = 0; i < 256; i++) Root->SetWord(Code(i, 9), Code(i, 8));
					Code Last;
					while (Source.Pointer() <= Source.Length() - BitsPerWord) {
						Code Read;
						Source.ReadCode(Read, BitsPerWord);
						auto Word = Root->GetWord(Read);
						Result->Write(Word->Word);
						if (Last.Length()) {
							Code New = Last;
							New.Append(Code(Word->Word.GetByte(0), 8));
							Root->SetWord(Code(Length, BitsPerWord), New);
							Length++;
							if (Length == Dimension) {
								Dimension <<= 1;
								LempelZivWelchDecodingTree * NewRoot = new (std::nothrow) LempelZivWelchDecodingTree;
								if (!NewRoot) throw OutOfMemoryException();
								NewRoot->On1 = new (std::nothrow) LempelZivWelchDecodingTree;
								NewRoot->On1->Construct(BitsPerWord);
								if (!NewRoot->On1) { delete NewRoot; throw OutOfMemoryException(); }
								NewRoot->On0 = Root;
								BitsPerWord++;
								Root = NewRoot;
							}
						}
						Last = Word->Word;
					}
				}
				catch (...) { Result->Release(); Root->Release(); throw; }
				Root->Release();
				auto Data = Result->GetRetainedStorage();
				Result->Release();
				return Data;
			}
			ENGINE_PACKED_STRUCTURE(RleWord8)
				static constexpr uint64 MaxRepeat = 0x80;
				union {
					struct {
						uint8 RepeatCount : 7;
						uint8 Collapsed : 1;
					};
					struct {
						uint8 Data;
					};
				};
				bool friend operator == (const RleWord8 & a, const RleWord8 & b) { return a.Data == b.Data; }
				bool friend operator != (const RleWord8 & a, const RleWord8 & b) { return a.Data != b.Data; }
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(RleWord16)
				static constexpr uint64 MaxRepeat = 0x8000;
				union {
					struct {
						uint16 RepeatCount : 15;
						uint16 Collapsed : 1;
					};
					struct {
						uint16 Data;
					};
				};
				bool friend operator == (const RleWord16 & a, const RleWord16 & b) { return a.Data == b.Data; }
				bool friend operator != (const RleWord16 & a, const RleWord16 & b) { return a.Data != b.Data; }
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(RleWord32)
				static constexpr uint64 MaxRepeat = 0x80000000;
				union {
					struct {
						uint32 RepeatCount : 31;
						uint32 Collapsed : 1;
					};
					struct {
						uint32 Data;
					};
				};
				bool friend operator == (const RleWord32 & a, const RleWord32 & b) { return a.Data == b.Data; }
				bool friend operator != (const RleWord32 & a, const RleWord32 & b) { return a.Data != b.Data; }
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(RleWord64)
				static constexpr uint64 MaxRepeat = 0x8000000000000000;
				union {
					struct {
						uint64 RepeatCount : 63;
						uint64 Collapsed : 1;
					};
					struct {
						uint64 Data;
					};
				};
				bool friend operator == (const RleWord64 & a, const RleWord64 & b) { return a.Data == b.Data; }
				bool friend operator != (const RleWord64 & a, const RleWord64 & b) { return a.Data != b.Data; }
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(RleWord128)
				static constexpr uint64 MaxRepeat = 0xFFFFFFFFFFFFFFFF;
				union {
					struct {
						uint64 RepeatCount : 64;
						uint64 Unused : 63;
						uint64 Collapsed : 1;
					};
					struct {
						uint64 Lo;
						uint64 Hi;
					} Data;
				};
				bool friend operator == (const RleWord128 & a, const RleWord128 & b) { return a.Data.Lo == b.Data.Lo && a.Data.Hi == b.Data.Hi; }
				bool friend operator != (const RleWord128 & a, const RleWord128 & b) { return a.Data.Lo != b.Data.Lo || a.Data.Hi != b.Data.Hi; }
			ENGINE_END_PACKED_STRUCTURE
			template<class RleWord> Array<uint8> * RunLengthCompress(Array<uint8> * input)
			{
				if (input->Length() % sizeof(RleWord)) throw InvalidFormatException();
				uint Pointer = 0;
				uint Length = input->Length() / sizeof(RleWord);
				RleWord * Source = reinterpret_cast<RleWord *>(input->GetBuffer());
				SafePointer<BitStream> Result = new (std::nothrow) BitStream;
				if (!Result) throw OutOfMemoryException();
				while (Pointer < Length) {
					if (Length - Pointer < 2 || Source[Pointer] != Source[Pointer + 1]) {
						uint Count = 1;
						while (Pointer + Count < Length && Count < RleWord::MaxRepeat && (Length - Pointer - Count <= 2 || !(Source[Pointer + Count] == Source[Pointer + Count + 1] && Source[Pointer + Count + 1] == Source[Pointer + Count + 2]))) Count++;
						RleWord Header;
						ZeroMemory(&Header, sizeof(Header));
						Header.RepeatCount = Count;
						Result->Write(&Header, sizeof(Header) * 8);
						for (uint i = 0; i < Count; i++) {
							Result->Write(Source + Pointer, sizeof(Header) * 8);
							Pointer++;
						}
					} else {
						uint Count = 2;
						while (Pointer + Count < Length && Source[Pointer + Count] == Source[Pointer] && Count < RleWord::MaxRepeat) Count++;
						RleWord Header;
						ZeroMemory(&Header, sizeof(Header));
						Header.RepeatCount = Count;
						Header.Collapsed = 1;
						Result->Write(&Header, sizeof(Header) * 8);
						Result->Write(Source + Pointer, sizeof(Header) * 8);
						Pointer += Count;
					}
				}
				return Result->GetRetainedStorage();
			}
			template<class RleWord> Array<uint8> * RunLengthDecompress(Array<uint8> * input)
			{
				if (input->Length() % sizeof(RleWord)) throw InvalidFormatException();
				uint Pointer = 0;
				uint Length = input->Length() / sizeof(RleWord);
				RleWord * Source = reinterpret_cast<RleWord *>(input->GetBuffer());
				SafePointer<BitStream> Result = new (std::nothrow) BitStream;
				if (!Result) throw OutOfMemoryException();
				while (Pointer < Length) {
					uint RepeatCount = Source[Pointer].RepeatCount;
					if (RepeatCount == 0) RepeatCount = uint(RleWord::MaxRepeat);
					if (Source[Pointer].Collapsed) {
						if (Pointer < Length - 1) {
							for (uint i = 0; i < RepeatCount; i++) Result->Write(Source + Pointer + 1, sizeof(RleWord) * 8);
							Pointer += 2;
						}
					} else {
						if (Length - Pointer > RepeatCount) {
							Pointer++;
							for (uint i = 0; i < RepeatCount; i++) { Result->Write(Source + Pointer, sizeof(RleWord) * 8); Pointer++; }
						}
					}
				}
				return Result->GetRetainedStorage();
			}
		}
		Array<uint8>* Compress(const void * data, int length, CompressionMethod method)
		{
			Array<uint8> buffer(length);
			buffer.SetLength(length);
			MemoryCopy(buffer.GetBuffer(), data, length);
			return Compress(buffer, method);
		}
		Array<uint8>* Compress(const Array<uint8>& data, CompressionMethod method)
		{
			auto a = const_cast< Array<uint8> *>(&data);
			if (!a->Length()) return new Array<uint8>;
			if (method == CompressionMethod::Huffman) return Compression::HuffmanCompress(a);
			else if (method == CompressionMethod::LempelZivWelch) return Compression::LempelZivWelchCompress(a);
			else if (method == CompressionMethod::RunLengthEncoding8bit) return Compression::RunLengthCompress<Compression::RleWord8>(a);
			else if (method == CompressionMethod::RunLengthEncoding16bit) return Compression::RunLengthCompress<Compression::RleWord16>(a);
			else if (method == CompressionMethod::RunLengthEncoding32bit) return Compression::RunLengthCompress<Compression::RleWord32>(a);
			else if (method == CompressionMethod::RunLengthEncoding64bit) return Compression::RunLengthCompress<Compression::RleWord64>(a);
			else if (method == CompressionMethod::RunLengthEncoding128bit) return Compression::RunLengthCompress<Compression::RleWord128>(a);
			else throw InvalidArgumentException();
		}
		Array<uint8>* Decompress(const void * data, int length, CompressionMethod method)
		{
			Array<uint8> buffer(length);
			buffer.SetLength(length);
			MemoryCopy(buffer.GetBuffer(), data, length);
			return Decompress(buffer, method);
		}
		Array<uint8>* Decompress(const Array<uint8>& data, CompressionMethod method)
		{
			auto a = const_cast< Array<uint8> *>(&data);
			if (!a->Length()) return new Array<uint8>;
			if (method == CompressionMethod::Huffman) return Compression::HuffmanDecompress(a);
			else if (method == CompressionMethod::LempelZivWelch) return Compression::LempelZivWelchDecompress(a);
			else if (method == CompressionMethod::RunLengthEncoding8bit) return Compression::RunLengthDecompress<Compression::RleWord8>(a);
			else if (method == CompressionMethod::RunLengthEncoding16bit) return Compression::RunLengthDecompress<Compression::RleWord16>(a);
			else if (method == CompressionMethod::RunLengthEncoding32bit) return Compression::RunLengthDecompress<Compression::RleWord32>(a);
			else if (method == CompressionMethod::RunLengthEncoding64bit) return Compression::RunLengthDecompress<Compression::RleWord64>(a);
			else if (method == CompressionMethod::RunLengthEncoding128bit) return Compression::RunLengthDecompress<Compression::RleWord128>(a);
			else throw InvalidArgumentException();
		}
	}
}