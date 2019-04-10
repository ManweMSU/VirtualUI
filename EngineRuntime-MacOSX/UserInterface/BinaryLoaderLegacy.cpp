#include "BinaryLoaderLegacy.h"

#include "../PlatformDependent/SystemColors.h"

using namespace Engine::Streaming;

namespace Engine
{
	namespace UI
	{
		namespace Loader
		{
			namespace BinaryLoader
			{
				namespace Format {
					ENGINE_PACKED_STRUCTURE(Header)
						uint8 Signature[8];
						uint32 Version;
						uint32 DataOffset;
						uint32 DataSize;
						uint32 TableOffset;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(Table)
						uint32 TextureOffset;
						int32 TextureCount;
						uint32 FontOffset;
						int32 FontCount;
						uint32 ApplicationOffset;
						int32 ApplicationCount;
						uint32 DialogOffset;
						int32 DialogCount;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(Texture)
						uint32 NameOffset;
						uint32 VersionOffset10;
						uint32 VersionSize10;
						uint32 VersionOffset15;
						uint32 VersionSize15;
						uint32 VersionOffset20;
						uint32 VersionSize20;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(Font)
						uint32 NameOffset;
						uint32 FaceOffset;
						uint32 HeightA;
						double HeightZ;
						uint32 Weight;
						uint32 Flags;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(Application)
						uint32 NameOffset;
						uint32 RootOffset;
					ENGINE_END_PACKED_STRUCTURE
					typedef Application Dialog;
					ENGINE_PACKED_STRUCTURE(Template4)
						uint32 Value;
						uint32 Parameter;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(Template8)
						double Value;
						uint32 Parameter;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(Coordinate)
						Template4 A;
						Template8 Z;
						Template8 W;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(Rectangle)
						Coordinate Left;
						Coordinate Top;
						Coordinate Right;
						Coordinate Bottom;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(GradientColor)
						Template4 Color;
						Template8 Position;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(ShapeBase)
						uint32 Type;
						Rectangle Position;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(ShapeFrame)
						ShapeBase Base;
						int32 ChildrenCount;
						uint32 ChildrenOffset[1];
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(ShapeBar)
						ShapeBase Base;
						uint32 Horizontal;
						int32 ColorsCount;
						uint32 ColorsOffset[1];
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(ShapeTexture)
						ShapeBase Base;
						uint32 TextureMode;
						Template4 TextureName;
						Rectangle TextureFrom;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(ShapeText)
						ShapeBase Base;
						Template4 FontName;
						Template4 Color;
						Template4 Text;
						uint32 DrawMode;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(SmallCoordinate)
						double W;
						double Z;
						uint32 A;
						uint32 unused;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(SmallRectangle)
						SmallCoordinate Left, Top, Right, Bottom;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(ControlBase)
						uint32 ClassName;
						SmallRectangle Position;
						int32 SettersCount;
						uint32 SettersOffset[1];
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(ControlTail)
						int32 ChildrenCount;
						uint32 ChildrenOffset[1];
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(Setter)
						uint32 Name;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(SetterBig)
						uint32 Name;
						SmallRectangle Value;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(SetterMedium)
						uint32 Name;
						SmallCoordinate Value;
					ENGINE_END_PACKED_STRUCTURE
					ENGINE_PACKED_STRUCTURE(SetterSmall)
						uint32 Name;
						uint32 Value;
					ENGINE_END_PACKED_STRUCTURE
				}
				using namespace Format;

				Color MapSystemColor(Color input)
				{
					if (input.a == 0) {
						if (!input.Value) return 0;
						int code = input.r;
						Color result = 0;
						if (code == 1) result = GetSystemColor(SystemColor::Theme);
						else if (code == 2) result = GetSystemColor(SystemColor::WindowBackgroup);
						else if (code == 3) result = GetSystemColor(SystemColor::WindowText);
						else if (code == 4) result = GetSystemColor(SystemColor::SelectedBackground);
						else if (code == 5) result = GetSystemColor(SystemColor::SelectedText);
						else if (code == 6) result = GetSystemColor(SystemColor::MenuBackground);
						else if (code == 7) result = GetSystemColor(SystemColor::MenuText);
						else if (code == 8) result = GetSystemColor(SystemColor::MenuHotBackground);
						else if (code == 9) result = GetSystemColor(SystemColor::MenuHotText);
						else if (code == 10) result = GetSystemColor(SystemColor::GrayedText);
						else if (code == 11) result = GetSystemColor(SystemColor::Hyperlink);
						if (input.b != 255) {
							double blend = double(input.b) / 255.0;
							result.a = uint8(double(result.a) * blend);
						}
						return result;
					} else return input;
				}

				void LoadTexture(Texture * Source, uint8 * Data, uint Size, IResourceLoader * ResourceLoader, InterfaceTemplate & Result)
				{
					SafePointer<Stream> TextureSource;
					if (Zoom > 1.75) {
						if (Source->VersionSize20) {
							TextureSource.SetReference(new MemoryStream(Data + Source->VersionOffset20, Source->VersionSize20));
						} else if (Source->VersionSize15) {
							TextureSource.SetReference(new MemoryStream(Data + Source->VersionOffset15, Source->VersionSize15));
						} else if (Source->VersionSize10) {
							TextureSource.SetReference(new MemoryStream(Data + Source->VersionOffset10, Source->VersionSize10));
						}
					} else if (Zoom > 1.25) {
						if (Source->VersionSize15) {
							TextureSource.SetReference(new MemoryStream(Data + Source->VersionOffset15, Source->VersionSize15));
						} else if (Source->VersionSize10) {
							TextureSource.SetReference(new MemoryStream(Data + Source->VersionOffset10, Source->VersionSize10));
						}
					} else {
						if (Source->VersionSize10) {
							TextureSource.SetReference(new MemoryStream(Data + Source->VersionOffset10, Source->VersionSize10));
						}
					}
					if (TextureSource.Inner()) {
						string Name = string(Data + Source->NameOffset, -1, Encoding::UTF16);
						if (Result.Texture.ElementPresent(Name)) {
							ResourceLoader->ReloadTexture(Result.Texture[Name], TextureSource);
						} else {
							SafePointer<ITexture> New = ResourceLoader->LoadTexture(TextureSource);
							if (New.Inner()) {
								Result.Texture.Append(Name, New);
							} else throw InvalidFormatException();
						}
					}
				}
				void LoadFont(Font * Source, uint8 * Data, uint Size, IResourceLoader * ResourceLoader, InterfaceTemplate & Result)
				{
					string Name = string(Data + Source->NameOffset, -1, Encoding::UTF16);
					if (Result.Font.ElementPresent(Name)) {
						ResourceLoader->ReloadFont(Result.Font[Name]);
					} else {
						string FontFace = string(Data + Source->FaceOffset, -1, Encoding::UTF16);
						SafePointer<IFont> New = ResourceLoader->LoadFont(FontFace, Source->HeightA + int(Source->HeightZ * Zoom), Source->Weight, (Source->Flags & 0x01) != 0, (Source->Flags & 0x02) != 0, (Source->Flags & 0x04) != 0);
						if (New.Inner()) {
							Result.Font.Append(Name, New);
						} else throw InvalidFormatException();
					}
				}

				Template::BasicTemplate<int> LoadIntegerTemplate(Template4 * Source, const uint8 * Data, uint Size)
				{
					string Argument = string(Data + Source->Parameter, -1, Encoding::UTF16);
					if (Argument.Length()) return Template::BasicTemplate<int>::Undefined(Argument);
					else return Template::BasicTemplate<int>(Source->Value);
				}
				Template::BasicTemplate<double> LoadFloatTemplate(Template8 * Source, const uint8 * Data, uint Size)
				{
					string Argument = string(Data + Source->Parameter, -1, Encoding::UTF16);
					if (Argument.Length()) return Template::BasicTemplate<double>::Undefined(Argument);
					else return Template::BasicTemplate<double>(Source->Value);
				}
				Template::BasicTemplate<Color> LoadColorTemplate(Template4 * Source, const uint8 * Data, uint Size)
				{
					string Argument = string(Data + Source->Parameter, -1, Encoding::UTF16);
					if (Argument.Length()) return Template::BasicTemplate<Color>::Undefined(Argument);
					else return Template::BasicTemplate<Color>(MapSystemColor(Color(Source->Value)));
				}
				Template::BasicTemplate<string> LoadTextTemplate(Template4 * Source, const uint8 * Data, uint Size)
				{
					string Argument = string(Data + Source->Parameter, -1, Encoding::UTF16);
					if (Argument.Length()) return Template::BasicTemplate<string>::Undefined(Argument);
					else return Template::BasicTemplate<string>(string(Data + Source->Value, -1, Encoding::UTF16));
				}
				Template::ObjectTemplate<ITexture> LoadTextureTemplate(Template4 * Source, const uint8 * Data, uint Size, IResourceResolver * ResourceResolver, InterfaceTemplate & Result)
				{
					string Argument = string(Data + Source->Parameter, -1, Encoding::UTF16);
					if (!Argument.Length()) {
						string Resource = string(Data + Source->Value, -1, Encoding::UTF16);
						if (Resource.Length()) {
							SafePointer<ITexture> Value = Result.Texture[Resource];
							if (!Value.Inner() && ResourceResolver) Value.SetReference(ResourceResolver->GetTexture(Resource));
							else if (Value.Inner()) Value->Retain();
							return Template::ObjectTemplate<ITexture>(Value);
						} else return Template::ObjectTemplate<ITexture>(0);
					} else return Template::ObjectTemplate<ITexture>::Undefined(Argument);
				}
				Template::ObjectTemplate<IFont> LoadFontTemplate(Template4 * Source, const uint8 * Data, uint Size, IResourceResolver * ResourceResolver, InterfaceTemplate & Result)
				{
					string Argument = string(Data + Source->Parameter, -1, Encoding::UTF16);
					if (!Argument.Length()) {
						string Resource = string(Data + Source->Value, -1, Encoding::UTF16);
						if (Resource.Length()) {
							SafePointer<IFont> Value = Result.Font[Resource];
							if (!Value.Inner() && ResourceResolver) Value.SetReference(ResourceResolver->GetFont(Resource));
							else if (Value.Inner()) Value->Retain();
							return Template::ObjectTemplate<IFont>(Value);
						} else return Template::ObjectTemplate<IFont>(0);
					} else return Template::ObjectTemplate<IFont>::Undefined(Argument);
				}
				Template::Coordinate LoadCoordinateTemplate(Coordinate * Source, const uint8 * Data, uint Size)
				{
					Template::Coordinate Result;
					Result.Absolute = LoadIntegerTemplate(&Source->A, Data, Size);
					Result.Zoom = LoadFloatTemplate(&Source->Z, Data, Size);
					Result.Anchor = LoadFloatTemplate(&Source->W, Data, Size);
					return Result;
				}
				Template::Rectangle LoadRectangleTemplate(Rectangle * Source, const uint8 * Data, uint Size)
				{
					Template::Rectangle Result;
					Result.Left = LoadCoordinateTemplate(&Source->Left, Data, Size);
					Result.Top = LoadCoordinateTemplate(&Source->Top, Data, Size);
					Result.Right = LoadCoordinateTemplate(&Source->Right, Data, Size);
					Result.Bottom = LoadCoordinateTemplate(&Source->Bottom, Data, Size);
					return Result;
				}
				Template::GradientPoint LoadGradientTemplate(GradientColor * Source, const uint8 * Data, uint Size)
				{
					Template::GradientPoint Result;
					Result.PointColor = LoadColorTemplate(&Source->Color, Data, Size);
					Result.Position = LoadFloatTemplate(&Source->Position, Data, Size);
					return Result;
				}

				Template::Shape * LoadShape(ShapeBase * Source, uint8 * Data, uint Size, IResourceResolver * ResourceResolver, InterfaceTemplate & Result)
				{
					auto Position = LoadRectangleTemplate(&Source->Position, Data, Size);
					if (Source->Type == 0) {
						auto Shape = reinterpret_cast<ShapeFrame *>(Source);
						SafePointer<Template::FrameShape> Template = new Template::FrameShape;
						Template->Position = Position;
						Template->RenderMode = FrameShape::FrameRenderMode::Normal;
						Template->Opacity = 1.0;
						for (int i = 0; i < Shape->ChildrenCount; i++) {
							SafePointer<Template::Shape> Entity = LoadShape(reinterpret_cast<ShapeBase *>(Data + Shape->ChildrenOffset[i]), Data, Size, ResourceResolver, Result);
							if (Entity.Inner()) Template->Children.Append(Entity);
						}
						Template->Retain();
						return Template;
					} else if (Source->Type == 1) {
						ShapeBar * Shape = reinterpret_cast<ShapeBar *>(Source);
						SafePointer<Template::BarShape> Template = new Template::BarShape;
						Template->Position = Position;
						Template->GradientAngle = Shape->Horizontal ? 0.0 : (-ENGINE_PI / 2.0);
						for (int i = 0; i < Shape->ColorsCount; i++) {
							Template->Gradient << LoadGradientTemplate(reinterpret_cast<GradientColor *>(Data + Shape->ColorsOffset[i]), Data, Size);
						}
						Template->Retain();
						return Template;
					} else if (Source->Type == 2) {
						ShapeTexture * Shape = reinterpret_cast<ShapeTexture *>(Source);
						SafePointer<Template::TextureShape> Template = new Template::TextureShape;
						Template->Position = Position;
						Template->Texture = LoadTextureTemplate(&Shape->TextureName, Data, Size, ResourceResolver, Result);
						Template->From = LoadRectangleTemplate(&Shape->TextureFrom, Data, Size);
						Template->RenderMode = static_cast<TextureShape::TextureRenderMode>(Shape->TextureMode);
						Template->Retain();
						return Template;
					} else if (Source->Type == 3) {
						ShapeText * Shape = reinterpret_cast<ShapeText *>(Source);
						SafePointer<Template::TextShape> Template = new Template::TextShape;
						Template->Position = Position;
						Template->Font = LoadFontTemplate(&Shape->FontName, Data, Size, ResourceResolver, Result);
						Template->Text = LoadTextTemplate(&Shape->Text, Data, Size);
						Template->TextColor = LoadColorTemplate(&Shape->Color, Data, Size);
						if ((Shape->DrawMode & 0x3) == 2) Template->HorizontalAlign = TextShape::TextHorizontalAlign::Right;
						else if ((Shape->DrawMode & 0x3) == 1) Template->HorizontalAlign = TextShape::TextHorizontalAlign::Center;
						else if ((Shape->DrawMode & 0x3) == 0) Template->HorizontalAlign = TextShape::TextHorizontalAlign::Left;
						if ((Shape->DrawMode & 0xC) == 8) Template->VerticalAlign = TextShape::TextVerticalAlign::Bottom;
						else if ((Shape->DrawMode & 0xC) == 4) Template->VerticalAlign = TextShape::TextVerticalAlign::Center;
						else if ((Shape->DrawMode & 0xC) == 0) Template->VerticalAlign = TextShape::TextVerticalAlign::Top;
						Template->Retain();
						return Template;
					} return 0;
				}
				void LoadApplication(Application * Source, uint8 * Data, uint Size, IResourceResolver * ResourceResolver, InterfaceTemplate & Result)
				{
					string Name = string(Data + Source->NameOffset, -1, Encoding::UTF16);
					if (!Result.Application.ElementPresent(Name)) {
						Result.Application.Append(Name, LoadShape(reinterpret_cast<ShapeBase *>(Data + Source->RootOffset), Data, Size, ResourceResolver, Result));
					}
				}

				Template::ControlTemplate * LoadControl(ControlBase * Source, uint8 * Data, uint Size, IResourceResolver * ResourceResolver, InterfaceTemplate & Result)
				{
					string ClassName = string(Data + Source->ClassName, -1, Encoding::UTF16);
					auto ControlPropertyBase = Template::Controls::CreateControlByClass(ClassName);
					Reflection::PropertyZeroInitializer Initializer;
					ControlPropertyBase->EnumerateProperties(Initializer);
					if (ControlPropertyBase) {
						SafePointer<Template::ControlTemplate> Template = new Template::ControlTemplate(ControlPropertyBase);
						Template->Properties->ControlPosition = UI::Rectangle(
							UI::Coordinate(Source->Position.Left.A, Source->Position.Left.Z, Source->Position.Left.W),
							UI::Coordinate(Source->Position.Top.A, Source->Position.Top.Z, Source->Position.Top.W),
							UI::Coordinate(Source->Position.Right.A, Source->Position.Right.Z, Source->Position.Right.W),
							UI::Coordinate(Source->Position.Bottom.A, Source->Position.Bottom.Z, Source->Position.Bottom.W)
						);
						for (int i = 0; i < Source->SettersCount; i++) {
							auto PropertySetter = reinterpret_cast<Setter *>(Data + Source->SettersOffset[i]);
							auto Property = Template->Properties->GetProperty(string(Data + PropertySetter->Name, -1, Encoding::UTF16));
							if (Property.Type != Reflection::PropertyType::Unknown) {
								if (Property.Type == Reflection::PropertyType::Boolean || Property.Type == Reflection::PropertyType::Color || Property.Type == Reflection::PropertyType::String ||
									Property.Type == Reflection::PropertyType::Texture || Property.Type == Reflection::PropertyType::Font || Property.Type == Reflection::PropertyType::Application ||
									Property.Type == Reflection::PropertyType::Dialog) {
									auto Ex = reinterpret_cast<SetterSmall *>(PropertySetter);
									if (Property.Type == Reflection::PropertyType::Boolean) {
										Property.Set<bool>(Ex->Value != 0);
									} else if (Property.Type == Reflection::PropertyType::Color) {
										Property.Set<Color>(MapSystemColor(Ex->Value));
									} else {
										string Resource = string(Data + Ex->Value, -1, Encoding::UTF16);
										if (Property.Type == Reflection::PropertyType::String) {
											Property.Set<string>(Resource);
										} else if (Property.Type == Reflection::PropertyType::Texture) {
											if (Resource.Length()) {
												SafePointer<ITexture> Texture = Result.Texture[Resource];
												if (!Texture.Inner() && ResourceResolver) Texture.SetReference(ResourceResolver->GetTexture(Resource));
												else if (Texture.Inner()) Texture->Retain();
												Property.Get<SafePointer<ITexture>>() = Texture;
											}
										} else if (Property.Type == Reflection::PropertyType::Font) {
											if (Resource.Length()) {
												SafePointer<IFont> Font = Result.Font[Resource];
												if (!Font.Inner() && ResourceResolver) Font.SetReference(ResourceResolver->GetFont(Resource));
												else if (Font.Inner()) Font->Retain();
												Property.Get<SafePointer<IFont>>() = Font;
											}
										} else if (Property.Type == Reflection::PropertyType::Application) {
											if (Resource.Length()) {
												SafePointer<Template::Shape> Shape = Result.Application[Resource];
												if (!Shape.Inner() && ResourceResolver) Shape.SetReference(ResourceResolver->GetApplication(Resource));
												else if (Shape.Inner()) Shape->Retain();
												Property.Get<SafePointer<Template::Shape>>() = Shape;
											}
										} else if (Property.Type == Reflection::PropertyType::Dialog) {
											if (Resource.Length()) {
												SafePointer<Template::ControlTemplate> Control = Result.Dialog[Resource];
												if (!Control.Inner() && ResourceResolver) Control.SetReference(ResourceResolver->GetDialog(Resource));
												else if (Control.Inner()) Control->Retain();
												Property.Get<SafePointer<Template::ControlTemplate>>() = Control;
											}
										}
									}
								} else if (Property.Type == Reflection::PropertyType::Integer || Property.Type == Reflection::PropertyType::Double) {
									auto Ex = reinterpret_cast<SetterMedium *>(PropertySetter);
									if (Property.Type == Reflection::PropertyType::Integer) {
										Property.Set<int>(Ex->Value.A + int(Ex->Value.Z * Zoom));
									} else if (Property.Type == Reflection::PropertyType::Double) {
										Property.Set<double>(Ex->Value.Z);
									}
								} else if (Property.Type == Reflection::PropertyType::Rectangle) {
									auto Ex = reinterpret_cast<SetterBig *>(PropertySetter);
									Property.Set<UI::Rectangle>(UI::Rectangle(
										UI::Coordinate(Ex->Value.Left.A, Ex->Value.Left.Z, Ex->Value.Left.W),
										UI::Coordinate(Ex->Value.Top.A, Ex->Value.Top.Z, Ex->Value.Top.W),
										UI::Coordinate(Ex->Value.Right.A, Ex->Value.Right.Z, Ex->Value.Right.W),
										UI::Coordinate(Ex->Value.Bottom.A, Ex->Value.Bottom.Z, Ex->Value.Bottom.W)
									));
								}
							}
						}
						ControlTail * Tail = reinterpret_cast<ControlTail *>(reinterpret_cast<uint8 *>(Source) + 8 + sizeof(SmallRectangle) + 4 * Source->SettersCount);
						for (int i = 0; i < Tail->ChildrenCount; i++) {
							SafePointer<Template::ControlTemplate> Entity = LoadControl(reinterpret_cast<ControlBase *>(Data + Tail->ChildrenOffset[i]), Data, Size, ResourceResolver, Result);
							if (Entity.Inner()) Template->Children.Append(Entity);
						}
						Template->Retain();
						return Template;
					} else return 0;
				}
				void LoadDialog(Dialog * Source, uint8 * Data, uint Size, IResourceResolver * ResourceResolver, InterfaceTemplate & Result) {
					string Name = string(Data + Source->NameOffset, -1, Encoding::UTF16);
					if (!Result.Dialog.ElementPresent(Name)) {
						Result.Dialog.Append(Name, LoadControl(reinterpret_cast<ControlBase *>(Data + Source->RootOffset), Data, Size, ResourceResolver, Result));
					}
				}

				void BuildInterface(InterfaceTemplate & Template, Table * Source, uint8 * Data, uint Size, IResourceLoader * ResourceLoader, IResourceResolver * ResourceResolver)
				{
					for (int i = 0; i < Source->TextureCount; i++) {
						LoadTexture(reinterpret_cast<Texture *>(Data + Source->TextureOffset + i * sizeof(Texture)), Data, Size, ResourceLoader, Template);
					}
					for (int i = 0; i < Source->FontCount; i++) {
						LoadFont(reinterpret_cast<Font *>(Data + Source->FontOffset + i * sizeof(Font)), Data, Size, ResourceLoader, Template);
					}
					for (int i = 0; i < Source->ApplicationCount; i++) {
						LoadApplication(reinterpret_cast<Application *>(Data + Source->ApplicationOffset + i * sizeof(Application)), Data, Size, ResourceResolver, Template);
					}
					for (int i = 0; i < Source->DialogCount; i++) {
						LoadDialog(reinterpret_cast<Dialog *>(Data + Source->DialogOffset + i * sizeof(Dialog)), Data, Size, ResourceResolver, Template);
					}
				}
			}
			void LoadUserInterfaceFromBinaryLegacy(InterfaceTemplate & Template, Streaming::Stream * Source, IResourceLoader * ResourceLoader, IResourceResolver * ResourceResolver)
			{
				auto base = Source->Seek(0, Current);
				auto length = Source->Length() - base;
				if (length < sizeof(BinaryLoader::Header)) throw InvalidFormatException();
				BinaryLoader::Header hdr;
				Source->Read(&hdr, sizeof(hdr));
				if (MemoryCompare(&hdr.Signature, "eusrint\0", 8) != 0) throw InvalidFormatException();
				if (hdr.Version != 0) throw InvalidFormatException();
				if (hdr.DataOffset > length) throw InvalidFormatException();
				if (hdr.DataOffset + hdr.DataSize > length) throw InvalidFormatException();
				if (hdr.TableOffset + sizeof(BinaryLoader::Table) > hdr.DataSize) throw InvalidFormatException();
				uint8 * data = new (std::nothrow) uint8[hdr.DataSize];
				if (!data) throw OutOfMemoryException();
				try {
					Source->Seek(base + hdr.DataOffset, Begin);
					Source->Read(data, hdr.DataSize);
					BinaryLoader::BuildInterface(Template, reinterpret_cast<BinaryLoader::Table *>(data + hdr.TableOffset), data, hdr.DataSize, ResourceLoader, ResourceResolver);
				}
				catch (...) { delete[] data; throw; }
				delete[] data;
			}
		}
	}
}