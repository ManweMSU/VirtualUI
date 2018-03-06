#pragma once

#include "ShapeBase.h"

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
			virtual void GetArgument(const string & name, ITexture ** value) = 0;
			virtual void GetArgument(const string & name, IFont ** value) = 0;
		};

		namespace Template
		{
			template <class T> class BasicTemplate
			{
				T value;
				string argument;
			public:
				BasicTemplate(void) {}
				BasicTemplate(const T & src) : value(src) {}

				static BasicTemplate Undefined(const string & argument_from) { BasicTemplate result; result.argument = argument_from; return result; }
				
				bool IsDefined(void) const { return argument.Length() == 0; }
				T Initialize(IArgumentProvider * provider) const
				{
					if (argument.Length()) {
						T result;
						provider->GetArgument(argument, &result);
						return result;
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
				ObjectTemplate(O * object) : value(object) {}
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
				GradientPoint(const Engine::UI::GradientPoint & gradient);
				GradientPoint(const BasicTemplate<Color> & color, const BasicTemplate<double> & position);

				bool IsDefined(void) const;
				Engine::UI::GradientPoint Initialize(IArgumentProvider * provider) const;
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
				BasicTemplate<double> GradientAngle;

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
				ObjectTemplate<ITexture> Texture;
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
				ObjectTemplate<IFont> Font;
				BasicTemplate<string> Text;
				Engine::UI::TextShape::TextHorizontalAlign HorizontalAlign;
				Engine::UI::TextShape::TextVerticalAlign VerticalAlign;
				BasicTemplate<Color> TextColor;

				TextShape(void);
				virtual ~TextShape(void) override;
				virtual string ToString(void) const override;
				virtual bool IsDefined(void) const override;
				virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const override;
			};
			class LineShape : public Shape
			{
			public:
				BasicTemplate<Color> LineColor;
				bool Dotted;

				LineShape(void);
				virtual ~LineShape(void) override;
				virtual string ToString(void) const override;
				virtual bool IsDefined(void) const override;
				virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const override;
			};
		}
	}
}