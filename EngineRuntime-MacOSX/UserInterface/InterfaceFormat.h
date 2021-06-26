#pragma once

#include "../Streaming.h"
#include "../Miscellaneous/Volumes.h"
#include "../Storage/StringTable.h"
#include "Templates.h"

namespace Engine
{
	namespace UI
	{
		namespace Format
		{
			namespace EncodeFlags {
				enum Flags {
					EncodeStringNames = 0x01,
				};
			}
			class InterfaceTemplateImage;
			// An extensible hierarhy of objects
			ENGINE_REFLECTED_CLASS(InterfaceCoordinate, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Absolute)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, Scalable)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, Anchor)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceAssetEntry, Reflection::Reflected) // A declaration for some asset to be loaded - an identifier of AST file
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, SystemFilter)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, AssetID)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceManifest, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(UINT32, Version)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceAssetEntry, Assets)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceColor, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, IsSystemColor)
				ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Value)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceTexture, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ImageID)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceFont, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, FontFace)
				ENGINE_DEFINE_REFLECTED_GENERIC_PROPERTY(InterfaceCoordinate, Height)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Weight)
				ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, IsItalic)
				ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, IsUnderline)
				ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, IsStrikeout)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceIntegerTemplate, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Argument)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Value)
			public:
				Template::IntegerTemplate ToTemplate(void);
				Template::StringTemplate ToStringTemplate(InterfaceTemplateImage * image);
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceDoubleTemplate, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Argument)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, Value)
			public:
				Template::DoubleTemplate ToTemplate(void);
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceColorTemplate, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Argument)
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Class)
				ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Value)
			public:
				Template::ColorTemplate ToTemplate(InterfaceTemplateImage * image);
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceStringTemplate, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Argument)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Value)
			public:
				Template::StringTemplate ToTemplate(void);
				Template::FontTemplate ToFontTemplate(InterfaceTemplate & interface, IResourceResolver * ResourceResolver);
				Template::TextureTemplate ToTextureTemplate(InterfaceTemplate & interface, IResourceResolver * ResourceResolver);
			ENGINE_END_REFLECTED_CLASS
				ENGINE_REFLECTED_CLASS(InterfaceRectangleTemplate, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_PROPERTY_VOLUME(STRING, Argument, 12)
				ENGINE_DEFINE_REFLECTED_PROPERTY_VOLUME(INTEGER, Absolute, 4);
				ENGINE_DEFINE_REFLECTED_PROPERTY_VOLUME(DOUBLE, Scalable, 8);
			public:
				Template::Rectangle ToTemplate(void);
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceShape, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Class)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceShape, Children)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceIntegerTemplate, IntegerValues)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceDoubleTemplate, DoubleValues)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceColorTemplate, ColorValues)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceStringTemplate, StringValues)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceRectangleTemplate, RectangleValues)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceApplication, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_GENERIC_PROPERTY(InterfaceShape, Root)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfacePropertySetter, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceCoordinatePropertySetter, InterfacePropertySetter)
				// Coordinate form for integer (Absolute + Zoom * Scalable)
				// For double: Scalable is the value
				// For strings: Absolute is string ID in string table
				// For boolean: Absolute is boolean value
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Absolute)
				ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, Scalable)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceStringPropertySetter, InterfacePropertySetter)
				// For strings: the value
				// For object refrences: the object's name
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Value)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceRectanglePropertySetter, InterfacePropertySetter)
				// For rectangles: the value
				ENGINE_DEFINE_REFLECTED_PROPERTY(RECTANGLE, Value)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceColorPropertySetter, InterfacePropertySetter)
				// Based on a Class: 0 - an actual color value, 1 - a system color, 2 - a refrence to a color table
				ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Class)
				ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Value)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceControl, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Class)
				ENGINE_DEFINE_REFLECTED_ARRAY(STRING, Styles)
				ENGINE_DEFINE_REFLECTED_PROPERTY(RECTANGLE, Position)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceControl, Children)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceCoordinatePropertySetter, CoordinateSetters)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceStringPropertySetter, StringsSetters)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceRectanglePropertySetter, RectangleSetters)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceColorPropertySetter, ColorSetters)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceDialog, Reflection::Reflected)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Name)
				ENGINE_DEFINE_REFLECTED_GENERIC_PROPERTY(InterfaceControl, Root)
			ENGINE_END_REFLECTED_CLASS
			ENGINE_REFLECTED_CLASS(InterfaceAsset, Reflection::Reflected) // A set of colors, fonts, textures, applications and dialogs. Locales should be loaded automaticaly by parsing the archive for STT files (locale name in the file name)
				ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, SystemFilter)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceColor, Colors)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceTexture, Textures) // Addressing IMG files
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceFont, Fonts)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceApplication, Applications)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceDialog, Styles)
				ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(InterfaceDialog, Dialogs)
			ENGINE_END_REFLECTED_CLASS

			class TemplateResourceResolver : public IResourceResolver
			{
			public:
				InterfaceTemplate * Source;
				IResourceResolver * DelegateResolver;
				virtual Graphics::IBitmap * GetTexture(const string & Name) override;
				virtual Graphics::IFont * GetFont(const string & Name) override;
				virtual Template::Shape * GetApplication(const string & Name) override;
				virtual Template::ControlTemplate * GetDialog(const string & Name) override;
				virtual Template::ControlReflectedBase * CreateCustomTemplate(const string & Class) override;
			};
			class IMissingStylesReporter : public Object
			{
			public:
				virtual void ReportStyleIsMissing(const string & Name, const string & Class) = 0;
			};
			class InterfaceTemplateImage : public Object
			{
			public:
				Volumes::ObjectDictionary<string, Storage::StringTable> Locales;
				Volumes::ObjectDictionary<int, Codec::Image> Textures;
				Volumes::Dictionary<string, int> StringIDs;
				Volumes::Dictionary<string, int> ColorIDs;
				Array<InterfaceAsset> Assets;

				InterfaceTemplateImage(void);
				InterfaceTemplateImage(Streaming::Stream * Source, const string & Locale, const string & System, double Scale); // Pass zero argument to load all resources (all locales, all system assets and all image sizes)
				virtual ~InterfaceTemplateImage(void) override;

				void Encode(Streaming::Stream * Output, uint32 Flags = 0);
				void Compile(InterfaceTemplate & Template, Graphics::I2DDeviceContextFactory * ResourceLoader = 0, IResourceResolver * ResourceResolver = 0, IMissingStylesReporter * StyleReporter = 0);
				void Compile(InterfaceTemplate & Template, InterfaceTemplate & Style, Graphics::I2DDeviceContextFactory * ResourceLoader = 0, IResourceResolver * ResourceResolver = 0, IMissingStylesReporter * StyleReporter = 0);
				void Specialize(const string & Locale, const string & System, double Scale);
				InterfaceTemplateImage * Clone(void);

				Color GetColorByID(int ID);
				string GetStringByID(int ID);
				Color GetColorByName(const string & name);
				string GetStringByName(const string & name);
			};
		}
	}
}