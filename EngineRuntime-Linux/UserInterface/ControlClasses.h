#pragma once

#include "Templates.h"

#define ENGINE_CONTROL_CLASS(class_name) ENGINE_REFLECTED_CLASS(class_name, ::Engine::UI::Template::ControlReflectedBase) \
public: virtual string GetTemplateClass(void) const override { return ENGINE_STRING(class_name); }
#define ENGINE_CONTROL_DERIVATIVE_CLASS(class_name, parent_class) ENGINE_REFLECTED_CLASS(class_name, parent_class) \
public: virtual string GetTemplateClass(void) const override { return ENGINE_STRING(class_name); }

namespace Engine
{
	namespace UI
	{
		namespace Template
		{
			namespace Controls
			{
				ENGINE_CONTROL_CLASS(DialogFrame)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Title)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Captioned)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Sizeble)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, CloseButton)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MaximizeButton)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MinimizeButton)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, HelpButton)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, ToolWindow)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, DefaultBackground)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, BackgroundColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, Background)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, MinimalWidth)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, MinimalHeight)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, MaximalWidth)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, MaximalHeight)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, Transparentcy)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Transparent)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, BlurBehind)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, BlurFactor)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(ControlGroup)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, Background)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(Button)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, ImageNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, ImageGrayed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewPressed)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(Static)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, Image)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, View)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(ColorView)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Color)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, View)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(ProgressBar)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, Progress)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, View)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(ComplexView)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, View)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Integer)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Integer2)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, Float)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, Float2)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Color)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Color2)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text2)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, Image)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, Image2)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font2)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(CheckBox)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Checked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormalChecked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabledChecked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocusedChecked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHotChecked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewPressedChecked)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(RadioButton)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Checked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormalChecked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabledChecked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocusedChecked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHotChecked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewPressedChecked)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(RadioButtonGroup)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, Background)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(HorizontalScrollBar)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, RangeMinimal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, RangeMaximal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Position)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Page)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Line)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewLeftButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewLeftButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewLeftButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewLeftButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewRightButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewRightButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewRightButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewRightButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollerNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollerHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollerPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollerDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewBarNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewBarDisabled)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(VerticalScrollBar)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, RangeMinimal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, RangeMaximal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Position)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Page)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Line)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewUpButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewUpButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewUpButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewUpButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDownButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDownButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDownButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDownButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollerNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollerHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollerPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollerDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewBarNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewBarDisabled)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(PopupMenu)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(MenuItem)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Height)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, SystemAppearance)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Checked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, HasChildren)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, RightText)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, ImageNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, ImageGrayed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, CheckedImageNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, CheckedImageGrayed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewCheckedNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewCheckedDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewCheckedHot)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(MenuSeparator)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Height)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, SystemAppearance)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, View)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(ToolButton)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewPressed)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(ToolButtonPart)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Checked)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DIALOG, DropDownMenu)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, ImageNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, ImageGrayed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFramedNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFramedDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewCheckedNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewCheckedDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewCheckedFramedNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewCheckedFramedDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewCheckedHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewCheckedPressed)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(Edit)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Color)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, ColorDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, SelectionColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Placeholder)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, PlaceholderFont)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, PlaceholderColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, PlaceholderColorDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Password)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, PasswordCharacter)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, CharactersEnabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, ReadOnly)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, UpperCase)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, LowerCase)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DIALOG, ContextMenu)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormalReadOnly)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocusedReadOnly)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Border)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, LeftSpace)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, CaretColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, CaretWidth)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(Scrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Border)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, VerticalScrollSize)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, HorizontalScrollSize)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarLeftButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarLeftButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarLeftButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarLeftButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarRightButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarRightButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarRightButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarRightButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarScrollerNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarScrollerHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarScrollerPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarScrollerDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarBarNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHorizontalScrollBarBarDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarUpButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarUpButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarUpButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarUpButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarDownButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarDownButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarDownButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarDownButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarScrollerNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarScrollerHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarScrollerPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarScrollerDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarBarNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewVerticalScrollBarBarDisabled)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(VerticallyScrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Border)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ScrollSize)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarUpButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarUpButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarUpButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarUpButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarDownButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarDownButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarDownButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarDownButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarScrollerNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarScrollerHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarScrollerPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarScrollerDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarBarNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewScrollBarBarDisabled)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(ScrollBox, Scrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, Background)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, VerticalLine)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, HorizontalLine)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(SplitBox)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, SplitterSize)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewSplitterNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewSplitterHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewSplitterPressed)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(HorizontalSplitBox, SplitBox)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(VerticalSplitBox, SplitBox)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(SplitBoxPart)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Stretch)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, MinimalSize)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(ListBox, VerticallyScrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Tiled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MultiChoose)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ElementHeight)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementSelected)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(TreeView, VerticallyScrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ElementHeight)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ChildOffset)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, BranchColorNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, BranchColorDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNodeCollapsedDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNodeCollapsedNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNodeCollapsedHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNodeOpenedDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNodeOpenedNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNodeOpenedHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementSelected)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(ListView, Scrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MultiChoose)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ElementHeight)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementSelected)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, HeaderHeight)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, HeaderStretchBarWidth)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHeaderNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHeaderDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewColumnHeaderNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewColumnHeaderHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewColumnHeaderPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewColumnHeaderDisabled)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(ListViewColumn)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, MinimalWidth)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementNormal)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(MultiLineEdit, Scrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormalReadOnly)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocusedReadOnly)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Color)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, ColorDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, SelectionColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, CharactersEnabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, ReadOnly)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, UpperCase)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, LowerCase)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DIALOG, ContextMenu)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, CaretColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, CaretWidth)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(ComboBox, VerticallyScrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(RECTANGLE, ItemPosition)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ElementHeight)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewHotFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDropDownList)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementDisabledPlaceholder)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementNormalPlaceholder)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementSelected)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(TextComboBox, VerticallyScrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(RECTANGLE, TextPosition)
					ENGINE_DEFINE_REFLECTED_PROPERTY(RECTANGLE, ButtonPosition)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ElementHeight)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewButtonDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewButtonNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewButtonHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewButtonPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Color)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, AdviceColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, ColorDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, SelectionColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Placeholder)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, PlaceholderFont)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, PlaceholderColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, PlaceholderColorDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, CharactersEnabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, UpperCase)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, LowerCase)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DIALOG, ContextMenu)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewElementSelected)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewDropDownList)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, CaretColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, CaretWidth)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(TrackBar)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, RangeMinimal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, RangeMaximal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Position)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Step)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, TrackerWidth)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewTrackerNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewTrackerFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewTrackerHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewTrackerPressed)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewTrackerDisabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewBarNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewBarDisabled)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(HorizontalTrackBar, TrackBar)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(VerticalTrackBar, TrackBar)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_DERIVATIVE_CLASS(RichEdit, VerticallyScrollable)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, SelectionColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, DefaultFontFace)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, DefaultFontHeight)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, DefaultTextColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, BackgroundColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, HyperlinkColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, HyperlinkHotColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DIALOG, ContextMenu)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, ReadOnly)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, DontCopyAttributes)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, CaretColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, CaretWidth)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(BookmarkView)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(FONT, Font)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewBackground)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewTabNormal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewTabHot)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewTabActive)
					ENGINE_DEFINE_REFLECTED_PROPERTY(APPLICATION, ViewTabActiveFocused)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, TabHeight)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(Bookmark)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(CustomControl)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Invisible)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, Disabled)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(TouchBar)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, MainItemID)
				ENGINE_END_REFLECTED_CLASS
				ENGINE_CONTROL_CLASS(TouchBarButton)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, Image)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, ImageID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Color)
				ENGINE_END_REFLECTED_CLASS
				ENGINE_CONTROL_CLASS(TouchBarGroup)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, MainItemID)
				ENGINE_END_REFLECTED_CLASS
				ENGINE_CONTROL_CLASS(TouchBarPopover)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, Image)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, ImageID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, MainItemID)
				ENGINE_END_REFLECTED_CLASS
				ENGINE_CONTROL_CLASS(TouchBarColorPicker)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, Color)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, AllowTransparent)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, Image)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, ImageID)
				ENGINE_END_REFLECTED_CLASS
				ENGINE_CONTROL_CLASS(TouchBarSlider)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, ID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, RangeMinimal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, RangeMaximal)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DOUBLE, Position)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, Ticks)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, MinImage)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, MinImageID)
					ENGINE_DEFINE_REFLECTED_PROPERTY(TEXTURE, MaxImage)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, MaxImageID)
				ENGINE_END_REFLECTED_CLASS

				ENGINE_CONTROL_CLASS(FrameExtendedData)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, WindowsLeftMargin)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, WindowsTopMargin)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, WindowsRightMargin)
					ENGINE_DEFINE_REFLECTED_PROPERTY(INTEGER, WindowsBottomMargin)
					ENGINE_DEFINE_REFLECTED_PROPERTY(DIALOG, MacTouchBar)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MacTransparentTitle)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MacEffectBackground)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MacCustomBackgroundColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MacUseLightTheme)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MacUseDarkTheme)
					ENGINE_DEFINE_REFLECTED_PROPERTY(BOOLEAN, MacShadowlessWindow)
					ENGINE_DEFINE_REFLECTED_PROPERTY(COLOR, MacBackgroundColor)
					ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, MacEffectBackgroundMaterial)
				ENGINE_END_REFLECTED_CLASS

				ControlReflectedBase * CreateControlByClass(const string & class_name);
			}
		}
	}
}

#undef ENGINE_CONTROL_CLASS
#undef ENGINE_CONTROL_DERIVATIVE_CLASS