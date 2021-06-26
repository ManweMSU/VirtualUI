#pragma once

#include "ShapeBase.h"
#include "../Miscellaneous/Volumes.h"
#include "../Miscellaneous/Reflection.h"
#include "../Syntax/MathExpression.h"

namespace Engine
{
	namespace UI
	{
		class IArgumentProvider
		{
		public:
			virtual void GetArgument(const string & name, int * value) = 0;
			virtual void GetArgument(const string & name, double * value) = 0;
			virtual void GetArgument(const string & name, Color * value) = 0;
			virtual void GetArgument(const string & name, string * value) = 0;
			virtual void GetArgument(const string & name, Graphics::IBitmap ** value) = 0;
			virtual void GetArgument(const string & name, Graphics::IFont ** value) = 0;
		};
		class ZeroArgumentProvider : public IArgumentProvider
		{
		public:
			ZeroArgumentProvider(void);
			virtual void GetArgument(const string & name, int * value) override;
			virtual void GetArgument(const string & name, double * value) override;
			virtual void GetArgument(const string & name, Color * value) override;
			virtual void GetArgument(const string & name, string * value) override;
			virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override;
			virtual void GetArgument(const string & name, Graphics::IFont ** value) override;
		};
		class ReflectorArgumentProvider : public IArgumentProvider
		{
			Reflection::Reflected * Source;
		public:
			ReflectorArgumentProvider(Reflection::Reflected * source);
			virtual void GetArgument(const string & name, int * value) override;
			virtual void GetArgument(const string & name, double * value) override;
			virtual void GetArgument(const string & name, Color * value) override;
			virtual void GetArgument(const string & name, string * value) override;
			virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override;
			virtual void GetArgument(const string & name, Graphics::IFont ** value) override;
		};

		namespace Template
		{
			class ExpressionArgumentProvider : public Syntax::Math::IVariableProvider
			{
			public:
				IArgumentProvider * Source;
				ExpressionArgumentProvider(IArgumentProvider * provider);
				virtual int64 GetInteger(const string & name) override;
				virtual double GetDouble(const string & name) override;
			};
			template <class T> T ResolveExpression(const string & expression, IArgumentProvider * provider) { return T(); }
			template <> int ResolveExpression<int>(const string & expression, IArgumentProvider * provider);
			template <> double ResolveExpression<double>(const string & expression, IArgumentProvider * provider);
			template <class T> class BasicTemplate
			{
				T value;
				string argument;
			public:
				BasicTemplate(void) {}
				BasicTemplate(const T & src) : value(src) {}

				static BasicTemplate Undefined(const string & argument_from) { BasicTemplate result; result.argument = argument_from; return result; }

				T & GetValue(void) { return value; }
				const T & GetValue(void) const { return value; }
				string & GetArgument(void) { return argument; }
				const string & GetArgument(void) const { return argument; }
				
				bool IsDefined(void) const { return argument.Length() == 0; }
				T Initialize(IArgumentProvider * provider) const
				{
					if (argument.Length()) {
						if (argument[0] == L'=') {
							return ResolveExpression<T>(argument.Fragment(1, -1), provider);
						} else {
							T result;
							provider->GetArgument(argument, &result);
							return result;
						}
					} else return value;
				}
			};
			template <class O> class ObjectTemplate
			{
				SafePointer<O> value;
				string argument;
			public:
				ObjectTemplate(void) {}
				ObjectTemplate(const ObjectTemplate & src) : argument(src.argument) { value.SetRetain(src.value); }
				ObjectTemplate(ObjectTemplate && src) : argument(src.argument) { value.SetRetain(src.value); src.value.SetReference(0); }
				ObjectTemplate(O * object) { value.SetRetain(object); }
				ObjectTemplate & operator = (const ObjectTemplate & src)
				{
					if (this == &src) return *this;
					value.SetRetain(src.value);
					argument = src.argument;
					return *this;
				}

				static ObjectTemplate Undefined(const string & argument_from) { ObjectTemplate result; result.argument = argument_from; return result; }

				bool IsDefined(void) const { return argument.Length() == 0; }
				O * Initialize(IArgumentProvider * provider) const
				{
					if (argument.Length()) {
						O * result;
						provider->GetArgument(argument, &result);
						return result;
					} else {
						if (value.Inner()) value->Retain();
						return value;
					}
				}
			};

			typedef BasicTemplate<int> IntegerTemplate;
			typedef BasicTemplate<double> DoubleTemplate;
			typedef BasicTemplate<Color> ColorTemplate;
			typedef BasicTemplate<string> StringTemplate;
			typedef ObjectTemplate<Graphics::IBitmap> TextureTemplate;
			typedef ObjectTemplate<Graphics::IFont> FontTemplate;

			class Coordinate
			{
			public:
				BasicTemplate<double> Anchor, Zoom;
				BasicTemplate<int> Absolute;

				Coordinate(void);
				Coordinate(const Engine::UI::Coordinate & src);
				Coordinate(const BasicTemplate<int> & shift, const BasicTemplate<double> & zoom, const BasicTemplate<double> & anchor);

				bool IsDefined(void) const;
				Engine::UI::Coordinate Initialize(IArgumentProvider * provider) const;
			};
			class Rectangle
			{
			public:
				Coordinate Left, Top, Right, Bottom;

				Rectangle(void);
				Rectangle(const Engine::UI::Rectangle & src);
				Rectangle(const Coordinate & left, const Coordinate & top, const Coordinate & right, const Coordinate & bottom);

				bool IsDefined(void) const;
				Engine::UI::Rectangle Initialize(IArgumentProvider * provider) const;
			};
			class GradientPoint
			{
			public:
				BasicTemplate<Color> PointColor;
				BasicTemplate<double> Position;

				GradientPoint(void);
				GradientPoint(const Engine::GradientPoint & gradient);
				GradientPoint(const BasicTemplate<Color> & color, const BasicTemplate<double> & position);

				bool IsDefined(void) const;
				Engine::GradientPoint Initialize(IArgumentProvider * provider) const;
			};

			class Shape : public Object
			{
			public:
				Rectangle Position;

				virtual bool IsDefined(void) const = 0;
				virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const = 0;
			};
			class FrameShape : public Shape
			{
			public:
				ObjectArray<Shape> Children;
				Engine::UI::FrameShape::FrameRenderMode RenderMode;
				BasicTemplate<double> Opacity;

				FrameShape(void);
				virtual ~FrameShape(void) override;
				virtual string ToString(void) const override;
				virtual bool IsDefined(void) const override;
				virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const override;
			};
			class BarShape : public Shape
			{
			public:
				Array<GradientPoint> Gradient;
				Coordinate X1, Y1, X2, Y2;

				BarShape(void);
				virtual ~BarShape(void) override;
				virtual string ToString(void) const override;
				virtual bool IsDefined(void) const override;
				virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const override;
			};
			class BlurEffectShape : public Shape
			{
			public:
				BasicTemplate<double> BlurPower;

				BlurEffectShape(void);
				virtual ~BlurEffectShape(void) override;
				virtual string ToString(void) const override;
				virtual bool IsDefined(void) const override;
				virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const override;
			};
			class InversionEffectShape : public Shape
			{
			public:
				InversionEffectShape(void);
				virtual ~InversionEffectShape(void) override;
				virtual string ToString(void) const override;
				virtual bool IsDefined(void) const override;
				virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const override;
			};
			class TextureShape : public Shape
			{
			public:
				ObjectTemplate<Graphics::IBitmap> Texture;
				Engine::UI::TextureShape::TextureRenderMode RenderMode;
				Rectangle From;

				TextureShape(void);
				virtual ~TextureShape(void) override;
				virtual string ToString(void) const override;
				virtual bool IsDefined(void) const override;
				virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const override;
			};
			class TextShape : public Shape
			{
			public:
				ObjectTemplate<Graphics::IFont> Font;
				BasicTemplate<string> Text;
				BasicTemplate<Color> TextColor;
				uint32 Flags;

				TextShape(void);
				virtual ~TextShape(void) override;
				virtual string ToString(void) const override;
				virtual bool IsDefined(void) const override;
				virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const override;
			};

			class ControlReflectedBase : public Reflection::Reflected
			{
			public:
				UI::Rectangle ControlPosition;
				virtual string GetTemplateClass(void) const = 0;
				virtual ~ControlReflectedBase(void);
			};
			class ControlTemplate : public Object
			{
			public:
				ObjectArray<ControlTemplate> Children;
				ControlReflectedBase * Properties;

				ControlTemplate(ControlReflectedBase * properties);
				virtual ~ControlTemplate(void) override;
				virtual string ToString(void) const override;
			};
		}

		class IResourceResolver : public Object
		{
		public:
			virtual Graphics::IBitmap * GetTexture(const string & Name) = 0;
			virtual Graphics::IFont * GetFont(const string & Name) = 0;
			virtual Template::Shape * GetApplication(const string & Name) = 0;
			virtual Template::ControlTemplate * GetDialog(const string & Name) = 0;
			virtual Template::ControlReflectedBase * CreateCustomTemplate(const string & Class) = 0;
		};

		class InterfaceTemplate : public Object
		{
		public:
			Volumes::ObjectDictionary<string, Graphics::IBitmap> Texture;
			Volumes::ObjectDictionary<string, Graphics::IFont> Font;
			Volumes::ObjectDictionary<string, Template::Shape> Application;
			Volumes::ObjectDictionary<string, Template::ControlTemplate> Dialog;
			Volumes::Dictionary<string, Color> Colors;
			Volumes::Dictionary<string, string> Strings;

			InterfaceTemplate(void);
			virtual ~InterfaceTemplate(void) override;
			virtual string ToString(void) const override;
		};
	}
}