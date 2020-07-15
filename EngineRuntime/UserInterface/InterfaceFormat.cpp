#include "InterfaceFormat.h"

#include "../Storage/Archive.h"
#include "../Storage/Object.h"
#include "../Storage/ImageVolume.h"
#include "../PlatformDependent/SystemColors.h"
#include "OverlappedWindows.h"

using namespace Engine::Storage;
using namespace Engine::Streaming;

namespace Engine
{
	namespace UI
	{
		namespace Format
		{
			ENGINE_REFLECTED_CLASS(InterfaceStringMapping, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_ARRAY(STRING, Keys)
				ENGINE_DEFINE_REFLECTED_ARRAY(INTEGER, Values)
			ENGINE_END_REFLECTED_CLASS
			ITexture * TemplateResourceResolver::GetTexture(const string & Name)
			{
				auto obj = Source->Texture[Name];
				if (obj) { obj->Retain(); return obj; }
				else if (DelegateResolver) return DelegateResolver->GetTexture(Name);
				else return 0;
			}
			IFont * TemplateResourceResolver::GetFont(const string & Name)
			{
				auto obj = Source->Font[Name];
				if (obj) { obj->Retain(); return obj; } else if (DelegateResolver) return DelegateResolver->GetFont(Name);
				else return 0;
			}
			Template::Shape * TemplateResourceResolver::GetApplication(const string & Name)
			{
				auto obj = Source->Application[Name];
				if (obj) { obj->Retain(); return obj; } else if (DelegateResolver) return DelegateResolver->GetApplication(Name);
				else return 0;
			}
			Template::ControlTemplate * TemplateResourceResolver::GetDialog(const string & Name)
			{
				auto obj = Source->Dialog[Name];
				if (obj) { obj->Retain(); return obj; } else if (DelegateResolver) return DelegateResolver->GetDialog(Name);
				else return 0;
			}
			typedef TemplateResourceResolver StyleResourceResolver;
			struct DialogReference {
				string ObjectName;
				Reflection::PropertyInfo Property;
			};
			Color GetSystemColorByValue(Color value)
			{
				int code = value.r;
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
				if (value.a != 255) {
					double blend = double(value.a) / 255.0;
					result.a = uint8(double(result.a) * blend);
				}
				return result;
			}
			InterfaceTemplateImage::InterfaceTemplateImage(void) : Locales(0x10), Textures(0x40), StringIDs(0x40), ColorIDs(0x40), Assets(0x10) {}
			InterfaceTemplateImage::InterfaceTemplateImage(Streaming::Stream * Source, const string & Locale, const string & System, double Scale) : InterfaceTemplateImage()
			{
				bool load_best_dpi = Storage::IsVolumeCodecLoadsBestDpiOnly();
				try {
					if (Scale && Scale == UI::Zoom) Storage::SetVolumeCodecLoadBestDpiOnly(true);
					SafePointer<Archive> container = OpenArchive(Source, ArchiveMetadataUsage::IgnoreMetadata);
					if (!container) throw InvalidFormatException();
					InterfaceManifest manifest;
					manifest.Version = 0xFFFFFFFF;
					{
						ArchiveFile manifest_file = container->FindArchiveFile(L"IMF", 1);
						if (!manifest_file) throw InvalidFormatException();
						SafePointer<Stream> manifest_stream = container->QueryFileStream(manifest_file, ArchiveStream::ForceDecompressed);
						Reflection::RestoreFromBinaryObject(manifest, manifest_stream);
						if (manifest.Version != 0) throw InvalidFormatException();
					}
					{
						for (int i = 1; i <= container->GetFileCount(); i++) if (container->GetFileType(i) == L"STT") {
							string lang = container->GetFileName(i);
							if (!lang.Length() || !Locale.Length() || lang == Locale) {
								if (!lang.Length()) lang = L"_";
								SafePointer<Stream> table_stream = container->QueryFileStream(i, ArchiveStream::ForceDecompressed);
								SafePointer<StringTable> table = new StringTable(table_stream);
								Locales.Append(lang, table);
							}
						}
					}
					for (int i = 0; i < manifest.Assets.Length(); i++) {
						if (!manifest.Assets[i].SystemFilter.Length() || !System.Length() || manifest.Assets[i].SystemFilter == System) {
							int asset_id = manifest.Assets[i].AssetID;
							ArchiveFile asset_file = container->FindArchiveFile(L"AST", asset_id);
							if (asset_file) {
								SafePointer<Stream> asset_stream = container->QueryFileStream(asset_file, ArchiveStream::ForceDecompressed);
								Assets.Append(InterfaceAsset());
								Reflection::RestoreFromBinaryObject(Assets.LastElement(), asset_stream);
								for (int j = 0; j < Assets.LastElement().Colors.Length(); j++) {
									auto & clr = Assets.LastElement().Colors[j];
									if (clr.Name.Length()) ColorIDs.Append(clr.Name, clr.ID);
								}
							}
						}
					}
					for (int a = 0; a < Assets.Length(); a++) {
						for (int i = 0; i < Assets[a].Textures.Length(); i++) {
							auto & texture_ref = Assets[a].Textures[i];
							ArchiveFile texture_file = container->FindArchiveFile(L"IMG", texture_ref.ImageID);
							if (!texture_file) throw InvalidFormatException();
							SafePointer<Stream> texture_stream = container->QueryFileStream(texture_file, ArchiveStream::Native);
							SafePointer<Codec::Image> texture = Codec::DecodeImage(texture_stream);
							if (!texture) throw InvalidFormatException();
							if (Scale) {
								SafePointer<Codec::Image> new_texture = new Codec::Image;
								new_texture->Frames.Append(texture->GetFrameBestDpiFit(Scale));
								texture = new_texture;
							}
							Textures.Append(texture_ref.ImageID, texture);
						}
					}
					{
						ArchiveFile str_map_file = container->FindArchiveFile(L"STM", 1);
						if (str_map_file) {
							SafePointer<Stream> str_map_stream = container->QueryFileStream(str_map_file, ArchiveStream::ForceDecompressed);
							InterfaceStringMapping map;
							Reflection::RestoreFromBinaryObject(map, str_map_stream);
							for (int i = 0; i < min(map.Values.Length(), map.Keys.Length()); i++) StringIDs.Append(map.Keys[i], map.Values[i]);
						}
					}
					Storage::SetVolumeCodecLoadBestDpiOnly(load_best_dpi);
				} catch (...) {
					Storage::SetVolumeCodecLoadBestDpiOnly(load_best_dpi);
					throw;
				}
			}
			InterfaceTemplateImage::~InterfaceTemplateImage(void) {}
			void InterfaceTemplateImage::Encode(Streaming::Stream * Output, uint32 Flags)
			{
				int file_count = 1 + Assets.Length() + Locales.Length() + Textures.Length();
				if (Flags & EncodeFlags::EncodeStringNames) file_count++;
				SafePointer<NewArchive> container = CreateArchive(Output, file_count, NewArchiveFlags::NoMetadata | NewArchiveFlags::UseFormat32);
				InterfaceManifest manifest;
				manifest.Version = 0;
				{
					for (int a = 0; a < Assets.Length(); a++) {
						manifest.Assets.AppendNew();
						manifest.Assets.InnerArray.LastElement().SystemFilter = Assets[a].SystemFilter;
						manifest.Assets.InnerArray.LastElement().AssetID = a + 1;
					}
					MemoryStream manifest_stream(0x10000);
					Reflection::SerializeToBinaryObject(manifest, &manifest_stream);
					manifest_stream.Seek(0, Begin);
					container->SetFileType(1, L"IMF");
					container->SetFileID(1, 1);
					container->SetFileData(1, &manifest_stream, MethodChain(CompressionMethod::LempelZivWelch, CompressionMethod::Huffman), CompressionQuality::ExtraVariative, 0, 0x10000);
				}
				ArchiveFile current = 2;
				for (int a = 0; a < Assets.Length(); a++) {
					MemoryStream asset_stream(0x10000);
					Reflection::SerializeToBinaryObject(Assets[a], &asset_stream);
					asset_stream.Seek(0, Begin);
					container->SetFileType(current, L"AST");
					container->SetFileID(current, manifest.Assets[a].AssetID);
					container->SetFileData(current, &asset_stream, MethodChain(CompressionMethod::LempelZivWelch, CompressionMethod::Huffman), CompressionQuality::ExtraVariative, 0, 0x10000);
					current++;
				}
				for (int i = 0; i < Locales.Length(); i++) {
					MemoryStream locale_stream(0x10000);
					Locales.ElementByIndex(i).object->Save(&locale_stream);
					locale_stream.Seek(0, Begin);
					container->SetFileType(current, L"STT");
					container->SetFileID(current, i + 1);
					container->SetFileName(current, (Locales.ElementByIndex(i).key == L"_") ? L"" : Locales.ElementByIndex(i).key);
					container->SetFileData(current, &locale_stream, MethodChain(CompressionMethod::LempelZivWelch, CompressionMethod::Huffman), CompressionQuality::ExtraVariative, 0, 0x10000);
					current++;
				}
				for (int i = 0; i < Textures.Length(); i++) {
					MemoryStream texture_stream(0x10000);
					Codec::EncodeImage(&texture_stream, Textures.ElementByIndex(i).object, L"EIWV");
					texture_stream.Seek(0, Begin);
					container->SetFileType(current, L"IMG");
					container->SetFileID(current, Textures.ElementByIndex(i).key);
					container->SetFileData(current, &texture_stream);
					current++;
				}
				if (Flags & EncodeFlags::EncodeStringNames) {
					InterfaceStringMapping mapping;
					for (int i = 0; i < StringIDs.Length(); i++) {
						mapping.Keys << StringIDs.ElementByIndex(i).key;
						mapping.Values << StringIDs.ElementByIndex(i).value;
					}
					MemoryStream mapping_stream(0x1000);
					Reflection::SerializeToBinaryObject(mapping, &mapping_stream);
					mapping_stream.Seek(0, Begin);
					container->SetFileType(current, L"STM");
					container->SetFileID(current, 1);
					container->SetFileData(current, &mapping_stream, MethodChain(CompressionMethod::LempelZivWelch, CompressionMethod::Huffman), CompressionQuality::ExtraVariative, 0, 0x10000);
					current++;
				}
				container->Finalize();
			}
			Template::Shape * CompileShape(InterfaceTemplateImage * image, InterfaceShape & shape, InterfaceTemplate & Template, IResourceResolver * ResourceResolver)
			{
				if (shape.Class == L"Frame") {
					SafePointer<Template::FrameShape> result = new Template::FrameShape;
					for (int i = 0; i < shape.Children.Length(); i++) {
						SafePointer<Template::Shape> inner = CompileShape(image, shape.Children[i], Template, ResourceResolver);
						result->Children.Append(inner);
					}
					for (int i = 0; i < shape.IntegerValues.Length(); i++) {
						if (shape.IntegerValues[i].Name == L"RenderMode") {
							if (shape.IntegerValues[i].Value == 0) result->RenderMode = FrameShape::FrameRenderMode::Normal;
							else if (shape.IntegerValues[i].Value == 1) result->RenderMode = FrameShape::FrameRenderMode::Clipping;
							else if (shape.IntegerValues[i].Value == 2) result->RenderMode = FrameShape::FrameRenderMode::Layering;
						}
					}
					for (int i = 0; i < shape.DoubleValues.Length(); i++) {
						if (shape.DoubleValues[i].Name == L"Opacity") {
							result->Opacity = shape.DoubleValues[i].ToTemplate();
						}
					}
					for (int i = 0; i < shape.RectangleValues.Length(); i++) {
						if (shape.RectangleValues[i].Name == L"Position") {
							result->Position = shape.RectangleValues[i].ToTemplate();
						}
					}
					result->Retain();
					return result;
				} else if (shape.Class == L"Bar") {
					SafePointer<Template::BarShape> result = new Template::BarShape;
					for (int i = 0; i < shape.ColorValues.Length(); i++) {
						if (shape.ColorValues[i].Name == L"Gradient") {
							result->Gradient << Template::GradientPoint();
							result->Gradient.LastElement().PointColor = shape.ColorValues[i].ToTemplate(image);
							result->Gradient.LastElement().Position = Template::DoubleTemplate(0.0);
						}
					}
					int gp = 0;
					for (int i = 0; i < shape.DoubleValues.Length(); i++) {
						if (shape.DoubleValues[i].Name == L"GradientAngle") {
							result->GradientAngle = shape.DoubleValues[i].ToTemplate();
						} else if (shape.DoubleValues[i].Name == L"Gradient") {
							result->Gradient[gp].Position = shape.DoubleValues[i].ToTemplate();
							gp++;
						}
					}
					for (int i = 0; i < shape.RectangleValues.Length(); i++) {
						if (shape.RectangleValues[i].Name == L"Position") {
							result->Position = shape.RectangleValues[i].ToTemplate();
						}
					}
					result->Retain();
					return result;
				} else if (shape.Class == L"Blur") {
					SafePointer<Template::BlurEffectShape> result = new Template::BlurEffectShape;
					for (int i = 0; i < shape.DoubleValues.Length(); i++) {
						if (shape.DoubleValues[i].Name == L"Power") {
							result->BlurPower = shape.DoubleValues[i].ToTemplate();
						}
					}
					for (int i = 0; i < shape.RectangleValues.Length(); i++) {
						if (shape.RectangleValues[i].Name == L"Position") {
							result->Position = shape.RectangleValues[i].ToTemplate();
						}
					}
					result->Retain();
					return result;
				} else if (shape.Class == L"Inversion") {
					SafePointer<Template::InversionEffectShape> result = new Template::InversionEffectShape;
					for (int i = 0; i < shape.RectangleValues.Length(); i++) {
						if (shape.RectangleValues[i].Name == L"Position") {
							result->Position = shape.RectangleValues[i].ToTemplate();
						}
					}
					result->Retain();
					return result;
				} else if (shape.Class == L"Texture") {
					SafePointer<Template::TextureShape> result = new Template::TextureShape;
					for (int i = 0; i < shape.IntegerValues.Length(); i++) {
						if (shape.IntegerValues[i].Name == L"RenderMode") {
							if (shape.IntegerValues[i].Value == 0) result->RenderMode = TextureShape::TextureRenderMode::Stretch;
							else if (shape.IntegerValues[i].Value == 1) result->RenderMode = TextureShape::TextureRenderMode::Fit;
							else if (shape.IntegerValues[i].Value == 2) result->RenderMode = TextureShape::TextureRenderMode::FillPattern;
							else if (shape.IntegerValues[i].Value == 3) result->RenderMode = TextureShape::TextureRenderMode::AsIs;
						}
					}
					for (int i = 0; i < shape.StringValues.Length(); i++) {
						if (shape.StringValues[i].Name == L"Texture") {
							result->Texture = shape.StringValues[i].ToTextureTemplate(Template, ResourceResolver);
						}
					}
					for (int i = 0; i < shape.RectangleValues.Length(); i++) {
						if (shape.RectangleValues[i].Name == L"Position") {
							result->Position = shape.RectangleValues[i].ToTemplate();
						} else if (shape.RectangleValues[i].Name == L"Source") {
							result->From = shape.RectangleValues[i].ToTemplate();
						}
					}
					result->Retain();
					return result;
				} else if (shape.Class == L"Text") {
					SafePointer<Template::TextShape> result = new Template::TextShape;
					for (int i = 0; i < shape.IntegerValues.Length(); i++) {
						if (shape.IntegerValues[i].Name == L"Text") {
							result->Text = shape.IntegerValues[i].ToStringTemplate(image);
						} else if (shape.IntegerValues[i].Name == L"HorizontalAlign") {
							if (shape.IntegerValues[i].Value == 0) result->HorizontalAlign = TextShape::TextHorizontalAlign::Left;
							else if (shape.IntegerValues[i].Value == 1) result->HorizontalAlign = TextShape::TextHorizontalAlign::Center;
							else if (shape.IntegerValues[i].Value == 2) result->HorizontalAlign = TextShape::TextHorizontalAlign::Right;
						} else if (shape.IntegerValues[i].Name == L"VerticalAlign") {
							if (shape.IntegerValues[i].Value == 0) result->VerticalAlign = TextShape::TextVerticalAlign::Top;
							else if (shape.IntegerValues[i].Value == 1) result->VerticalAlign = TextShape::TextVerticalAlign::Center;
							else if (shape.IntegerValues[i].Value == 2) result->VerticalAlign = TextShape::TextVerticalAlign::Bottom;
						}
					}
					for (int i = 0; i < shape.ColorValues.Length(); i++) {
						if (shape.ColorValues[i].Name == L"Color") {
							result->TextColor = shape.ColorValues[i].ToTemplate(image);
						}
					}
					for (int i = 0; i < shape.StringValues.Length(); i++) {
						if (shape.StringValues[i].Name == L"Text") {
							result->Text = shape.StringValues[i].ToTemplate();
						} else if (shape.StringValues[i].Name == L"Font") {
							result->Font = shape.StringValues[i].ToFontTemplate(Template, ResourceResolver);
						}
					}
					for (int i = 0; i < shape.RectangleValues.Length(); i++) {
						if (shape.RectangleValues[i].Name == L"Position") {
							result->Position = shape.RectangleValues[i].ToTemplate();
						}
					}
					result->Retain();
					return result;
				}
				return 0;
			}
			Template::ControlTemplate * CompileControl(InterfaceTemplateImage * image, InterfaceControl & control, InterfaceTemplate & Template, InterfaceTemplate * Style, IResourceResolver * ResourceResolver, Array<DialogReference> & References)
			{
				auto base = Template::Controls::CreateControlByClass(control.Class);
				if (base) {
					Template::ControlTemplate * base_control = 0;
					if (Style) base_control = Style->Dialog[L"@default:" + control.Class];
					if (base_control) {
						Reflection::PropertyCopyInitializer initializer(*base_control->Properties);
						base->EnumerateProperties(initializer);
					} else {
						Reflection::PropertyZeroInitializer initializer;
						base->EnumerateProperties(initializer);
					}
					base->ControlPosition = Rectangle(0, 0, 0, 0);
					SafePointer<Template::ControlTemplate> result = new Template::ControlTemplate(base);
					base->ControlPosition = control.Position;
					for (int i = 0; i < control.Children.Length(); i++) {
						SafePointer<Template::ControlTemplate> inner = CompileControl(image, control.Children[i], Template, Style, ResourceResolver, References);
						if (inner) result->Children.Append(inner);
					}
					for (int i = 0; i < control.CoordinateSetters.Length(); i++) {
						auto & setter = control.CoordinateSetters[i];
						auto prop = base->GetProperty(setter.Name);
						if (prop.Type == Reflection::PropertyType::Integer) {
							prop.Set<int>(setter.Absolute + int(setter.Scalable * Zoom));
						} else if (prop.Type == Reflection::PropertyType::Double) {
							prop.Set<double>(setter.Scalable);
						} else if (prop.Type == Reflection::PropertyType::String) {
							prop.Set<string>(image->GetStringByID(setter.Absolute));
						} else if (prop.Type == Reflection::PropertyType::Boolean) {
							prop.Set<bool>(setter.Absolute != 0);
						}
					}
					for (int i = 0; i < control.StringsSetters.Length(); i++) {
						auto & setter = control.StringsSetters[i];
						auto prop = base->GetProperty(setter.Name);
						if (prop.Type == Reflection::PropertyType::String) {
							prop.Set<string>(setter.Value);
						} else if (prop.Type == Reflection::PropertyType::Texture) {
							if (setter.Value.Length()) {
								ITexture * texture = Template.Texture[setter.Value];
								if (!texture && ResourceResolver) texture = ResourceResolver->GetTexture(setter.Value);
								prop.Get< SafePointer<ITexture> >().SetRetain(texture);
							} else prop.Get< SafePointer<ITexture> >().SetReference(0);
						} else if (prop.Type == Reflection::PropertyType::Font) {
							if (setter.Value.Length()) {
								IFont * font = Template.Font[setter.Value];
								if (!font && ResourceResolver) font = ResourceResolver->GetFont(setter.Value);
								prop.Get< SafePointer<IFont> >().SetRetain(font);
							} else prop.Get< SafePointer<IFont> >().SetReference(0);
						} else if (prop.Type == Reflection::PropertyType::Application) {
							if (setter.Value.Length()) {
								Template::Shape * application = Template.Application[setter.Value];
								if (!application && ResourceResolver) application = ResourceResolver->GetApplication(setter.Value);
								prop.Get< SafePointer<Template::Shape> >().SetRetain(application);
							} else prop.Get< SafePointer<Template::Shape> >().SetReference(0);
						} else if (prop.Type == Reflection::PropertyType::Dialog) {
							if (setter.Value.Length()) {
								DialogReference ref{ setter.Value, prop };
								References << ref;
							} else prop.Get< SafePointer<Template::ControlTemplate> >().SetReference(0);
						}
					}
					for (int i = 0; i < control.RectangleSetters.Length(); i++) {
						auto & setter = control.RectangleSetters[i];
						auto prop = base->GetProperty(setter.Name);
						if (prop.Type == Reflection::PropertyType::Rectangle) {
							prop.Set<Rectangle>(setter.Value);
						}
					}
					for (int i = 0; i < control.ColorSetters.Length(); i++) {
						auto & setter = control.ColorSetters[i];
						auto prop = base->GetProperty(setter.Name);
						if (prop.Type == Reflection::PropertyType::Color) {
							if (setter.Class == 2) prop.Set<Color>(image->GetColorByID(setter.Value.Value));
							else if (setter.Class == 1) prop.Set<Color>(GetSystemColorByValue(setter.Value));
							else prop.Set<Color>(setter.Value);
						}
					}
					result->Retain();
					return result;
				}
				return 0;
			}
			void ApplyProperties(InterfaceControl & control, const InterfaceControl & source)
			{
				Array<string> props_set(0x40);
				for (int i = 0; i < control.CoordinateSetters.Length(); i++) props_set << control.CoordinateSetters[i].Name;
				for (int i = 0; i < control.StringsSetters.Length(); i++) props_set << control.StringsSetters[i].Name;
				for (int i = 0; i < control.RectangleSetters.Length(); i++) props_set << control.RectangleSetters[i].Name;
				for (int i = 0; i < control.ColorSetters.Length(); i++) props_set << control.ColorSetters[i].Name;
				for (int i = 0; i < source.CoordinateSetters.InnerArray.Length(); i++) {
					bool found = false;
					for (int j = 0; j < props_set.Length(); j++) if (props_set[j] == source.CoordinateSetters.InnerArray[i].Name) { found = true; break; }
					if (!found) control.CoordinateSetters << source.CoordinateSetters.InnerArray[i];
				}
				for (int i = 0; i < source.StringsSetters.InnerArray.Length(); i++) {
					bool found = false;
					for (int j = 0; j < props_set.Length(); j++) if (props_set[j] == source.StringsSetters.InnerArray[i].Name) { found = true; break; }
					if (!found) control.StringsSetters << source.StringsSetters.InnerArray[i];
				}
				for (int i = 0; i < source.RectangleSetters.InnerArray.Length(); i++) {
					bool found = false;
					for (int j = 0; j < props_set.Length(); j++) if (props_set[j] == source.RectangleSetters.InnerArray[i].Name) { found = true; break; }
					if (!found) control.RectangleSetters << source.RectangleSetters.InnerArray[i];
				}
				for (int i = 0; i < source.ColorSetters.InnerArray.Length(); i++) {
					bool found = false;
					for (int j = 0; j < props_set.Length(); j++) if (props_set[j] == source.ColorSetters.InnerArray[i].Name) { found = true; break; }
					if (!found) control.ColorSetters << source.ColorSetters.InnerArray[i];
				}
			}
			void BuildStyle(const Array<InterfaceDialog> & styles, InterfaceControl & control, IMissingStylesReporter * StyleReporter)
			{
				for (int i = control.Styles.Length() - 1; i >= 0; i--) {
					int index = -1;
					for (int j = 0; j < styles.Length(); j++) if (control.Styles[i] == styles[j].Name && control.Class == styles[j].Root.Class) { index = j; break; }
					if (index != -1) ApplyProperties(control, styles[index].Root);
					else if (StyleReporter) StyleReporter->ReportStyleIsMissing(control.Styles[i], control.Class);
				}
				for (int i = 0; i < control.Children.Length(); i++) BuildStyle(styles, control.Children[i], StyleReporter);
			}
			void InterfaceTemplateImage::Compile(InterfaceTemplate & Template, IResourceLoader * ResourceLoader, IResourceResolver * ResourceResolver, IMissingStylesReporter * StyleReporter)
			{
				SafePointer<IResourceLoader> Loader;
				if (ResourceLoader) {
					Loader.SetRetain(ResourceLoader);
				} else {
					Loader = Windows::CreateNativeCompatibleResourceLoader();
				}
				Array<DialogReference> References(0x20);
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Textures.Length(); i++) {
						auto & name = asset.Textures[i].Name;
						auto image = Textures.ElementByKey(asset.Textures[i].ImageID);
						if (image) {
							if (Template.Texture.ElementPresent(name)) {
								Template.Texture[name]->Reload(image->Frames.FirstElement());
							} else {
								SafePointer<ITexture> texture = Loader->LoadTexture(image->Frames.FirstElement());
								Template.Texture.Append(name, texture);
							}
						}
					}
				}
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Fonts.Length(); i++) {
						auto & name = asset.Fonts[i].Name;
						SafePointer<IFont> font = Loader->LoadFont(asset.Fonts[i].FontFace, asset.Fonts[i].Height.Absolute + int(asset.Fonts[i].Height.Scalable * Zoom), asset.Fonts[i].Weight, asset.Fonts[i].IsItalic, asset.Fonts[i].IsUnderline, asset.Fonts[i].IsStrikeout);
						Template.Font.Append(name, font);
					}
				}
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Applications.Length(); i++) {
						auto & name = asset.Applications[i].Name;
						SafePointer<Template::Shape> shape = CompileShape(this, asset.Applications[i].Root, Template, ResourceResolver);
						if (shape) Template.Application.Append(name, shape);
					}
				}
				Array<InterfaceDialog> Styles(0x40);
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Styles.Length(); i++) {
						InterfaceDialog style = asset.Styles[i];
						BuildStyle(Styles, style.Root, StyleReporter);
						Styles << style;
					}
				}
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Dialogs.Length(); i++) {
						auto & name = asset.Dialogs[i].Name;
						InterfaceControl Root = asset.Dialogs[i].Root;
						BuildStyle(Styles, Root, StyleReporter);
						SafePointer<Template::ControlTemplate> control = CompileControl(this, Root, Template, 0, ResourceResolver, References);
						if (control) Template.Dialog.Append(name, control);
					}
				}
				for (int i = 0; i < References.Length(); i++) {
					Template::ControlTemplate * dialog = Template.Dialog[References[i].ObjectName];
					if (!dialog && ResourceResolver) dialog = ResourceResolver->GetDialog(References[i].ObjectName);
					References[i].Property.Get< SafePointer<Template::ControlTemplate> >().SetRetain(dialog);
				}
				for (int i = 0; i < ColorIDs.Length(); i++) {
					Template.Colors.Append(ColorIDs.ElementByIndex(i).key, GetColorByID(ColorIDs.ElementByIndex(i).value));
				}
				for (int i = 0; i < StringIDs.Length(); i++) {
					Template.Strings.Append(StringIDs.ElementByIndex(i).key, GetStringByID(StringIDs.ElementByIndex(i).value));
				}
			}
			void InterfaceTemplateImage::Compile(InterfaceTemplate & Template, InterfaceTemplate & Style, IResourceLoader * ResourceLoader, IResourceResolver * ResourceResolver, IMissingStylesReporter * StyleReporter)
			{
				SafePointer<IResourceLoader> Loader;
				if (ResourceLoader) {
					Loader.SetRetain(ResourceLoader);
				} else {
					Loader = Windows::CreateNativeCompatibleResourceLoader();
				}
				Array<DialogReference> References(0x20);
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Textures.Length(); i++) {
						auto & name = asset.Textures[i].Name;
						auto image = Textures.ElementByKey(asset.Textures[i].ImageID);
						if (image) {
							if (Template.Texture.ElementPresent(name)) {
								Template.Texture[name]->Reload(image->Frames.FirstElement());
							} else {
								SafePointer<ITexture> texture = Loader->LoadTexture(image->Frames.FirstElement());
								Template.Texture.Append(name, texture);
							}
						}
					}
				}
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Fonts.Length(); i++) {
						auto & name = asset.Fonts[i].Name;
						SafePointer<IFont> font = Loader->LoadFont(asset.Fonts[i].FontFace, asset.Fonts[i].Height.Absolute + int(asset.Fonts[i].Height.Scalable * Zoom), asset.Fonts[i].Weight, asset.Fonts[i].IsItalic, asset.Fonts[i].IsUnderline, asset.Fonts[i].IsStrikeout);
						Template.Font.Append(name, font);
					}
				}
				StyleResourceResolver style_resolver;
				style_resolver.Source = &Style;
				style_resolver.DelegateResolver = ResourceResolver;
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Applications.Length(); i++) {
						auto & name = asset.Applications[i].Name;
						SafePointer<Template::Shape> shape = CompileShape(this, asset.Applications[i].Root, Template, &style_resolver);
						if (shape) Template.Application.Append(name, shape);
					}
				}
				Array<InterfaceDialog> Styles(0x40);
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Styles.Length(); i++) {
						InterfaceDialog style = asset.Styles[i];
						BuildStyle(Styles, style.Root, StyleReporter);
						Styles << style;
					}
				}
				for (int a = 0; a < Assets.Length(); a++) {
					auto & asset = Assets[a];
					for (int i = 0; i < asset.Dialogs.Length(); i++) {
						auto & name = asset.Dialogs[i].Name;
						InterfaceControl Root = asset.Dialogs[i].Root;
						BuildStyle(Styles, Root, StyleReporter);
						SafePointer<Template::ControlTemplate> control = CompileControl(this, Root, Template, &Style, &style_resolver, References);
						if (control) Template.Dialog.Append(name, control);
					}
				}
				for (int i = 0; i < References.Length(); i++) {
					Template::ControlTemplate * dialog = Template.Dialog[References[i].ObjectName];
					if (!dialog) dialog = style_resolver.GetDialog(References[i].ObjectName);
					References[i].Property.Get< SafePointer<Template::ControlTemplate> >().SetRetain(dialog);
				}
				for (int i = 0; i < ColorIDs.Length(); i++) {
					Template.Colors.Append(ColorIDs.ElementByIndex(i).key, GetColorByID(ColorIDs.ElementByIndex(i).value));
				}
				for (int i = 0; i < StringIDs.Length(); i++) {
					Template.Strings.Append(StringIDs.ElementByIndex(i).key, GetStringByID(StringIDs.ElementByIndex(i).value));
				}
			}
			void InterfaceTemplateImage::Specialize(const string & Locale, const string & System, double Scale)
			{
				ColorIDs.Clear();
				if (Locale.Length()) for (int i = Locales.Length() - 1; i >= 0; i--) if (Locales.ElementByIndex(i).key != L"_" && Locales.ElementByIndex(i).key != Locale) Locales.RemoveAt(i);
				if (System.Length()) for (int i = Assets.Length() - 1; i >= 0; i--) if (Assets.ElementAt(i).SystemFilter.Length() && Assets.ElementAt(i).SystemFilter != System) Assets.Remove(i);
				if (Scale) for (int i = 0; i < Textures.Length(); i++) {
					auto image = Textures.ElementByIndex(i).object;
					SafePointer<Codec::Frame> frame_ref;
					frame_ref.SetRetain(image->GetFrameBestDpiFit(Scale));
					image->Frames.Clear();
					image->Frames.Append(frame_ref);
				}
				for (int i = 0; i < Assets.Length(); i++) for (int j = 0; j < Assets[i].Colors.Length(); j++) {
					auto & clr = Assets[i].Colors[j];
					if (clr.Name.Length()) ColorIDs.Append(clr.Name, clr.ID);
				}
			}
			InterfaceTemplateImage * InterfaceTemplateImage::Clone(void)
			{
				SafePointer<InterfaceTemplateImage> copy = new InterfaceTemplateImage;
				copy->Assets = Assets;
				for (int i = 0; i < Textures.Length(); i++) copy->Textures.Append(Textures.ElementByIndex(i).key, Textures.ElementByIndex(i).object);
				for (int i = 0; i < Locales.Length(); i++) copy->Locales.Append(Locales.ElementByIndex(i).key, Locales.ElementByIndex(i).object);
				for (int i = 0; i < StringIDs.Length(); i++) copy->StringIDs.Append(StringIDs.ElementByIndex(i).key, StringIDs.ElementByIndex(i).value);
				for (int i = 0; i < ColorIDs.Length(); i++) copy->ColorIDs.Append(ColorIDs.ElementByIndex(i).key, ColorIDs.ElementByIndex(i).value);
				copy->Retain();
				return copy;
			}
			Color InterfaceTemplateImage::GetColorByID(int ID)
			{
				for (int a = 0; a < Assets.Length(); a++) {
					for (int c = 0; c < Assets[a].Colors.Length(); c++) {
						if (Assets[a].Colors[c].ID == ID) {
							auto & clr = Assets[a].Colors[c];
							if (clr.IsSystemColor) return GetSystemColorByValue(clr.Value); else return clr.Value;
						}
					}
				}
				return 0;
			}
			string InterfaceTemplateImage::GetStringByID(int ID)
			{
				try {
					for (int l = 0; l < Locales.Length(); l++) {
						if (Locales.ElementByIndex(l).key == L"_") continue;
						return Locales.ElementByIndex(l).object->GetString(ID);
					}
				} catch (...) {}
				try {
					auto global = Locales[L"_"];
					if (global) return global->GetString(ID);
				} catch (...) {}
				return L"";
			}
			Color InterfaceTemplateImage::GetColorByName(const string & name)
			{
				auto p_id = ColorIDs[name];
				if (p_id) return GetColorByID(*p_id); else return 0;
			}
			string InterfaceTemplateImage::GetStringByName(const string & name)
			{
				auto p_id = StringIDs[name];
				if (p_id) return GetStringByID(*p_id); else return L"";
			}

			Template::IntegerTemplate InterfaceIntegerTemplate::ToTemplate(void)
			{
				return Argument.Length() ? Template::IntegerTemplate::Undefined(Argument) : Template::IntegerTemplate(Value);
			}
			Template::StringTemplate InterfaceIntegerTemplate::ToStringTemplate(InterfaceTemplateImage * image)
			{
				return Argument.Length() ? Template::StringTemplate::Undefined(Argument) : Template::StringTemplate(image->GetStringByID(Value));
			}
			Template::DoubleTemplate InterfaceDoubleTemplate::ToTemplate(void)
			{
				return Argument.Length() ? Template::DoubleTemplate::Undefined(Argument) : Template::DoubleTemplate(Value);
			}
			Template::ColorTemplate InterfaceColorTemplate::ToTemplate(InterfaceTemplateImage * image)
			{
				if (Argument.Length()) return Template::ColorTemplate::Undefined(Argument);
				if (Class == 2) return image->GetColorByID(Value.Value);
				else if (Class == 1) return GetSystemColorByValue(Value);
				else return Value;
			}
			Template::StringTemplate InterfaceStringTemplate::ToTemplate(void)
			{
				return Argument.Length() ? Template::StringTemplate::Undefined(Argument) : Template::StringTemplate(Value);
			}
			Template::FontTemplate InterfaceStringTemplate::ToFontTemplate(InterfaceTemplate & interface, IResourceResolver * ResourceResolver)
			{
				if (Argument.Length()) return Template::FontTemplate::Undefined(Argument);
				IFont * Font = interface.Font[Value];
				if (!Font && ResourceResolver) {
					Font = ResourceResolver->GetFont(Value);
				}
				return Template::FontTemplate(Font);
			}
			Template::TextureTemplate InterfaceStringTemplate::ToTextureTemplate(InterfaceTemplate & interface, IResourceResolver * ResourceResolver)
			{
				if (Argument.Length()) return Template::TextureTemplate::Undefined(Argument);
				ITexture * Texture = interface.Texture[Value];
				if (!Texture && ResourceResolver) {
					Texture = ResourceResolver->GetTexture(Value);
				}
				return Template::TextureTemplate(Texture);
			}
			Template::Rectangle InterfaceRectangleTemplate::ToTemplate(void)
			{
				return Template::Rectangle(
					Template::Coordinate(Argument[0].Length() ? Template::IntegerTemplate::Undefined(Argument[0]) : Template::IntegerTemplate(Absolute[0]),
						Argument[1].Length() ? Template::DoubleTemplate::Undefined(Argument[1]) : Template::DoubleTemplate(Scalable[0]),
						Argument[2].Length() ? Template::DoubleTemplate::Undefined(Argument[2]) : Template::DoubleTemplate(Scalable[1])),
					Template::Coordinate(Argument[3].Length() ? Template::IntegerTemplate::Undefined(Argument[3]) : Template::IntegerTemplate(Absolute[1]),
						Argument[4].Length() ? Template::DoubleTemplate::Undefined(Argument[4]) : Template::DoubleTemplate(Scalable[2]),
						Argument[5].Length() ? Template::DoubleTemplate::Undefined(Argument[5]) : Template::DoubleTemplate(Scalable[3])),
					Template::Coordinate(Argument[6].Length() ? Template::IntegerTemplate::Undefined(Argument[6]) : Template::IntegerTemplate(Absolute[2]),
						Argument[7].Length() ? Template::DoubleTemplate::Undefined(Argument[7]) : Template::DoubleTemplate(Scalable[4]),
						Argument[8].Length() ? Template::DoubleTemplate::Undefined(Argument[8]) : Template::DoubleTemplate(Scalable[5])),
					Template::Coordinate(Argument[9].Length() ? Template::IntegerTemplate::Undefined(Argument[9]) : Template::IntegerTemplate(Absolute[3]),
						Argument[10].Length() ? Template::DoubleTemplate::Undefined(Argument[10]) : Template::DoubleTemplate(Scalable[6]),
						Argument[11].Length() ? Template::DoubleTemplate::Undefined(Argument[11]) : Template::DoubleTemplate(Scalable[7]))
				);
			}
		}
	}
}