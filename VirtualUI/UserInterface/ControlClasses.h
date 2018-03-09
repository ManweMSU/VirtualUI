#pragma once

#include "Templates.h"

#define ENGINE_CONTROL_CLASS(class_name) ENGINE_REFLECTED_CLASS(class_name, ::Engine::UI::Template::ControlReflectedBase) \
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
			}
		}
	}
}

/*

TODO: Rewrite all this shit using new reflected structures

ControlType * Static(void)
{
if (!_Static) {
_Static = new ControlType(L"Static");
_Static->AddProperty(new ControlProperty(L"ID", 0));
_Static->AddProperty(new ControlProperty(L"Invisible", 2));
_Static->AddProperty(new ControlProperty(L"Text", 4));
_Static->AddProperty(new ControlProperty(L"Font", 6));
_Static->AddProperty(new ControlProperty(L"Image", 5));
_Static->AddProperty(new ControlProperty(L"View", 7));
}
return _Static;
}
ControlType * CheckBox(void)
{
if (!_CheckBox) {
_CheckBox = new ControlType(L"CheckBox");
_CheckBox->AddProperty(new ControlProperty(L"ID", 0));
_CheckBox->AddProperty(new ControlProperty(L"Invisible", 2));
_CheckBox->AddProperty(new ControlProperty(L"Disabled", 2));
_CheckBox->AddProperty(new ControlProperty(L"Checked", 2));
_CheckBox->AddProperty(new ControlProperty(L"Text", 4));
_CheckBox->AddProperty(new ControlProperty(L"Font", 6));
_CheckBox->AddProperty(new ControlProperty(L"ViewNormal", 7));
_CheckBox->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_CheckBox->AddProperty(new ControlProperty(L"ViewFocused", 7));
_CheckBox->AddProperty(new ControlProperty(L"ViewHot", 7));
_CheckBox->AddProperty(new ControlProperty(L"ViewPressed", 7));
_CheckBox->AddProperty(new ControlProperty(L"ViewNormalChecked", 7));
_CheckBox->AddProperty(new ControlProperty(L"ViewDisabledChecked", 7));
_CheckBox->AddProperty(new ControlProperty(L"ViewFocusedChecked", 7));
_CheckBox->AddProperty(new ControlProperty(L"ViewHotChecked", 7));
_CheckBox->AddProperty(new ControlProperty(L"ViewPressedChecked", 7));
}
return _CheckBox;
}
ControlType * ProgressBar(void)
{
if (!_ProgressBar) {
_ProgressBar = new ControlType(L"ProgressBar");
_ProgressBar->AddProperty(new ControlProperty(L"ID", 0));
_ProgressBar->AddProperty(new ControlProperty(L"Invisible", 2));
_ProgressBar->AddProperty(new ControlProperty(L"Progress", 1));
_ProgressBar->AddProperty(new ControlProperty(L"View", 7));
}
return _ProgressBar;
}
ControlType * RadioButtonGroup(void)
{
if (!_RadioButtonGroup) {
_RadioButtonGroup = new ControlType(L"RadioButtonGroup");
_RadioButtonGroup->AddProperty(new ControlProperty(L"ID", 0));
_RadioButtonGroup->AddProperty(new ControlProperty(L"Invisible", 2));
_RadioButtonGroup->AddProperty(new ControlProperty(L"Disabled", 2));
_RadioButtonGroup->AddProperty(new ControlProperty(L"Background", 7));
}
return _RadioButtonGroup;
}
ControlType * RadioButton(void)
{
if (!_RadioButton) {
_RadioButton = new ControlType(L"RadioButton");
_RadioButton->AddProperty(new ControlProperty(L"ID", 0));
_RadioButton->AddProperty(new ControlProperty(L"Invisible", 2));
_RadioButton->AddProperty(new ControlProperty(L"Disabled", 2));
_RadioButton->AddProperty(new ControlProperty(L"Checked", 2));
_RadioButton->AddProperty(new ControlProperty(L"Text", 4));
_RadioButton->AddProperty(new ControlProperty(L"Font", 6));
_RadioButton->AddProperty(new ControlProperty(L"ViewNormal", 7));
_RadioButton->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_RadioButton->AddProperty(new ControlProperty(L"ViewFocused", 7));
_RadioButton->AddProperty(new ControlProperty(L"ViewHot", 7));
_RadioButton->AddProperty(new ControlProperty(L"ViewPressed", 7));
_RadioButton->AddProperty(new ControlProperty(L"ViewNormalChecked", 7));
_RadioButton->AddProperty(new ControlProperty(L"ViewDisabledChecked", 7));
_RadioButton->AddProperty(new ControlProperty(L"ViewFocusedChecked", 7));
_RadioButton->AddProperty(new ControlProperty(L"ViewHotChecked", 7));
_RadioButton->AddProperty(new ControlProperty(L"ViewPressedChecked", 7));
}
return _RadioButton;
}
ControlType * HorizontalScrollBar(void)
{
if (!_HorizontalScrollBar) {
_HorizontalScrollBar = new ControlType(L"HorizontalScrollBar");
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ID", 0));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"Invisible", 2));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"Disabled", 2));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"RangeMinimal", 0));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"RangeMaximal", 0));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"Position", 0));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"Page", 0));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"Line", 0));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewLeftButtonNormal", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewLeftButtonHot", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewLeftButtonPressed", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewLeftButtonDisabled", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewRightButtonNormal", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewRightButtonHot", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewRightButtonPressed", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewRightButtonDisabled", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewScrollerNormal", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewScrollerHot", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewScrollerPressed", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewScrollerDisabled", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewBarNormal", 7));
_HorizontalScrollBar->AddProperty(new ControlProperty(L"ViewBarDisabled", 7));
}
return _HorizontalScrollBar;
}
ControlType * VerticalScrollBar(void)
{
if (!_VerticalScrollBar) {
_VerticalScrollBar = new ControlType(L"VerticalScrollBar");
_VerticalScrollBar->AddProperty(new ControlProperty(L"ID", 0));
_VerticalScrollBar->AddProperty(new ControlProperty(L"Invisible", 2));
_VerticalScrollBar->AddProperty(new ControlProperty(L"Disabled", 2));
_VerticalScrollBar->AddProperty(new ControlProperty(L"RangeMinimal", 0));
_VerticalScrollBar->AddProperty(new ControlProperty(L"RangeMaximal", 0));
_VerticalScrollBar->AddProperty(new ControlProperty(L"Position", 0));
_VerticalScrollBar->AddProperty(new ControlProperty(L"Page", 0));
_VerticalScrollBar->AddProperty(new ControlProperty(L"Line", 0));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewUpButtonNormal", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewUpButtonHot", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewUpButtonPressed", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewUpButtonDisabled", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewDownButtonNormal", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewDownButtonHot", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewDownButtonPressed", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewDownButtonDisabled", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewScrollerNormal", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewScrollerHot", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewScrollerPressed", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewScrollerDisabled", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewBarNormal", 7));
_VerticalScrollBar->AddProperty(new ControlProperty(L"ViewBarDisabled", 7));
}
return _VerticalScrollBar;
}
ControlType * PopupMenu(void)
{
if (!_PopupMenu) {
_PopupMenu = new ControlType(L"PopupMenu");
}
return _PopupMenu;
}
ControlType * MenuItem(void)
{
if (!_MenuItem) {
_MenuItem = new ControlType(L"MenuItem");
_MenuItem->AddProperty(new ControlProperty(L"Height", 0));
_MenuItem->AddProperty(new ControlProperty(L"ID", 0));
_MenuItem->AddProperty(new ControlProperty(L"Disabled", 2));
_MenuItem->AddProperty(new ControlProperty(L"Checked", 2));
_MenuItem->AddProperty(new ControlProperty(L"HasChildren", 2));
_MenuItem->AddProperty(new ControlProperty(L"Text", 4));
_MenuItem->AddProperty(new ControlProperty(L"RightText", 4));
_MenuItem->AddProperty(new ControlProperty(L"Font", 6));
_MenuItem->AddProperty(new ControlProperty(L"ImageNormal", 5));
_MenuItem->AddProperty(new ControlProperty(L"ImageGrayed", 5));
_MenuItem->AddProperty(new ControlProperty(L"ViewNormal", 7));
_MenuItem->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_MenuItem->AddProperty(new ControlProperty(L"ViewHot", 7));
_MenuItem->AddProperty(new ControlProperty(L"ViewCheckedNormal", 7));
_MenuItem->AddProperty(new ControlProperty(L"ViewCheckedDisabled", 7));
_MenuItem->AddProperty(new ControlProperty(L"ViewCheckedHot", 7));
_MenuItem->AddProperty(new ControlProperty(L"CheckedImageNormal", 5));
_MenuItem->AddProperty(new ControlProperty(L"CheckedImageGrayed", 5));
}
return _MenuItem;
}
ControlType * MenuSeparator(void)
{
if (!_MenuSeparator) {
_MenuSeparator = new ControlType(L"MenuSeparator");
_MenuSeparator->AddProperty(new ControlProperty(L"Height", 0));
_MenuSeparator->AddProperty(new ControlProperty(L"View", 7));
}
return _MenuSeparator;
}
ControlType * ToolButton(void)
{
if (!_ToolButton) {
_ToolButton = new ControlType(L"ToolButton");
_ToolButton->AddProperty(new ControlProperty(L"ID", 0));
_ToolButton->AddProperty(new ControlProperty(L"Invisible", 2));
_ToolButton->AddProperty(new ControlProperty(L"Disabled", 2));
_ToolButton->AddProperty(new ControlProperty(L"ViewNormal", 7));
_ToolButton->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_ToolButton->AddProperty(new ControlProperty(L"ViewHot", 7));
_ToolButton->AddProperty(new ControlProperty(L"ViewPressed", 7));
}
return _ToolButton;
}
ControlType * ToolButtonPart(void)
{
if (!_ToolButtonPart) {
_ToolButtonPart = new ControlType(L"ToolButtonPart");
_ToolButtonPart->AddProperty(new ControlProperty(L"ID", 0));
_ToolButtonPart->AddProperty(new ControlProperty(L"Disabled", 2));
_ToolButtonPart->AddProperty(new ControlProperty(L"Checked", 2));
_ToolButtonPart->AddProperty(new ControlProperty(L"DropDownMenu", 8));
_ToolButtonPart->AddProperty(new ControlProperty(L"Text", 4));
_ToolButtonPart->AddProperty(new ControlProperty(L"Font", 6));
_ToolButtonPart->AddProperty(new ControlProperty(L"ImageNormal", 5));
_ToolButtonPart->AddProperty(new ControlProperty(L"ImageGrayed", 5));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewNormal", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewFramedNormal", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewFramedDisabled", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewHot", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewPressed", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewCheckedNormal", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewCheckedDisabled", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewCheckedFramedNormal", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewCheckedFramedDisabled", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewCheckedHot", 7));
_ToolButtonPart->AddProperty(new ControlProperty(L"ViewCheckedPressed", 7));
}
return _ToolButtonPart;
}
ControlType * Edit(void)
{
if (!_Edit) {
_Edit = new ControlType(L"Edit");
_Edit->AddProperty(new ControlProperty(L"ID", 0));
_Edit->AddProperty(new ControlProperty(L"Invisible", 2));
_Edit->AddProperty(new ControlProperty(L"Disabled", 2));
_Edit->AddProperty(new ControlProperty(L"Text", 4));
_Edit->AddProperty(new ControlProperty(L"Font", 6));
_Edit->AddProperty(new ControlProperty(L"Color", 3));
_Edit->AddProperty(new ControlProperty(L"ColorDisabled", 3));
_Edit->AddProperty(new ControlProperty(L"SelectionColor", 3));
_Edit->AddProperty(new ControlProperty(L"Placeholder", 4));
_Edit->AddProperty(new ControlProperty(L"PlaceholderFont", 6));
_Edit->AddProperty(new ControlProperty(L"PlaceholderColor", 3));
_Edit->AddProperty(new ControlProperty(L"PlaceholderColorDisabled", 3));
_Edit->AddProperty(new ControlProperty(L"Password", 2));
_Edit->AddProperty(new ControlProperty(L"PasswordCharacter", 4));
_Edit->AddProperty(new ControlProperty(L"CharactersEnabled", 4));
_Edit->AddProperty(new ControlProperty(L"ReadOnly", 2));
_Edit->AddProperty(new ControlProperty(L"UpperCase", 2));
_Edit->AddProperty(new ControlProperty(L"LowerCase", 2));
_Edit->AddProperty(new ControlProperty(L"ContextMenu", 8));
_Edit->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_Edit->AddProperty(new ControlProperty(L"ViewNormal", 7));
_Edit->AddProperty(new ControlProperty(L"ViewFocused", 7));
_Edit->AddProperty(new ControlProperty(L"ViewNormalReadOnly", 7));
_Edit->AddProperty(new ControlProperty(L"ViewFocusedReadOnly", 7));
_Edit->AddProperty(new ControlProperty(L"Border", 0));
_Edit->AddProperty(new ControlProperty(L"LeftSpace", 0));
}
return _Edit;
}
ControlType * ScrollBox(void)
{
if (!_ScrollBox) {
_ScrollBox = new ControlType(L"ScrollBox");
_ScrollBox->AddProperty(new ControlProperty(L"ID", 0));
_ScrollBox->AddProperty(new ControlProperty(L"Invisible", 2));
_ScrollBox->AddProperty(new ControlProperty(L"Disabled", 2));
_ScrollBox->AddProperty(new ControlProperty(L"Background", 7));
_ScrollBox->AddProperty(new ControlProperty(L"Border", 0));
_ScrollBox->AddProperty(new ControlProperty(L"VerticalLine", 0));
_ScrollBox->AddProperty(new ControlProperty(L"HorizontalLine", 0));
_ScrollBox->AddProperty(new ControlProperty(L"VerticalScrollSize", 0));
_ScrollBox->AddProperty(new ControlProperty(L"HorizontalScrollSize", 0));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonNormal", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonHot", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonPressed", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonDisabled", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonNormal", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonHot", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonPressed", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonDisabled", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerNormal", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerHot", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerPressed", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerDisabled", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarBarNormal", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarBarDisabled", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonNormal", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonHot", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonPressed", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonDisabled", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonNormal", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonHot", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonPressed", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonDisabled", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerNormal", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerHot", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerPressed", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerDisabled", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarBarNormal", 7));
_ScrollBox->AddProperty(new ControlProperty(L"ViewVerticalScrollBarBarDisabled", 7));
}
return _ScrollBox;
}
ControlType * HorizontalSplitBox(void)
{
if (!_HorizontalSplitBox) {
_HorizontalSplitBox = new ControlType(L"HorizontalSplitBox");
_HorizontalSplitBox->AddProperty(new ControlProperty(L"ID", 0));
_HorizontalSplitBox->AddProperty(new ControlProperty(L"Invisible", 2));
_HorizontalSplitBox->AddProperty(new ControlProperty(L"Disabled", 2));
_HorizontalSplitBox->AddProperty(new ControlProperty(L"SplitterSize", 0));
_HorizontalSplitBox->AddProperty(new ControlProperty(L"ViewSplitterNormal", 7));
_HorizontalSplitBox->AddProperty(new ControlProperty(L"ViewSplitterHot", 7));
_HorizontalSplitBox->AddProperty(new ControlProperty(L"ViewSplitterPressed", 7));
}
return _HorizontalSplitBox;
}
ControlType * VerticalSplitBox(void)
{
if (!_VerticalSplitBox) {
_VerticalSplitBox = new ControlType(L"VerticalSplitBox");
_VerticalSplitBox->AddProperty(new ControlProperty(L"ID", 0));
_VerticalSplitBox->AddProperty(new ControlProperty(L"Invisible", 2));
_VerticalSplitBox->AddProperty(new ControlProperty(L"Disabled", 2));
_VerticalSplitBox->AddProperty(new ControlProperty(L"SplitterSize", 0));
_VerticalSplitBox->AddProperty(new ControlProperty(L"ViewSplitterNormal", 7));
_VerticalSplitBox->AddProperty(new ControlProperty(L"ViewSplitterHot", 7));
_VerticalSplitBox->AddProperty(new ControlProperty(L"ViewSplitterPressed", 7));
}
return _VerticalSplitBox;
}
ControlType * SplitBoxPart(void)
{
if (!_SplitBoxPart) {
_SplitBoxPart = new ControlType(L"SplitBoxPart");
_SplitBoxPart->AddProperty(new ControlProperty(L"ID", 0));
_SplitBoxPart->AddProperty(new ControlProperty(L"Stretch", 2));
_SplitBoxPart->AddProperty(new ControlProperty(L"MinimalSize", 0));
}
return _SplitBoxPart;
}
ControlType * ListBox(void)
{
if (!_ListBox) {
_ListBox = new ControlType(L"ListBox");
_ListBox->AddProperty(new ControlProperty(L"ID", 0));
_ListBox->AddProperty(new ControlProperty(L"Invisible", 2));
_ListBox->AddProperty(new ControlProperty(L"Disabled", 2));
_ListBox->AddProperty(new ControlProperty(L"Tiled", 2));
_ListBox->AddProperty(new ControlProperty(L"Font", 6));
_ListBox->AddProperty(new ControlProperty(L"Border", 0));
_ListBox->AddProperty(new ControlProperty(L"ElementHeight", 0));
_ListBox->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewNormal", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewFocused", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewElementDisabled", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewElementNormal", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewElementHot", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewElementSelected", 7));
_ListBox->AddProperty(new ControlProperty(L"ScrollSize", 0));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonNormal", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonHot", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonPressed", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonDisabled", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonNormal", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonHot", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonPressed", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonDisabled", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerNormal", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerHot", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerPressed", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerDisabled", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarBarNormal", 7));
_ListBox->AddProperty(new ControlProperty(L"ViewScrollBarBarDisabled", 7));
}
return _ListBox;
}
ControlType * TreeView(void)
{
if (!_TreeView) {
_TreeView = new ControlType(L"TreeView");
_TreeView->AddProperty(new ControlProperty(L"ID", 0));
_TreeView->AddProperty(new ControlProperty(L"Invisible", 2));
_TreeView->AddProperty(new ControlProperty(L"Disabled", 2));
_TreeView->AddProperty(new ControlProperty(L"Border", 0));
_TreeView->AddProperty(new ControlProperty(L"ElementHeight", 0));
_TreeView->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewNormal", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewFocused", 7));
_TreeView->AddProperty(new ControlProperty(L"Font", 6));
_TreeView->AddProperty(new ControlProperty(L"ChildOffset", 0));
_TreeView->AddProperty(new ControlProperty(L"BranchColorNormal", 3));
_TreeView->AddProperty(new ControlProperty(L"BranchColorDisabled", 3));
_TreeView->AddProperty(new ControlProperty(L"ViewNodeCollapsedDisabled", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewNodeCollapsedNormal", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewNodeCollapsedHot", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewNodeOpenedDisabled", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewNodeOpenedNormal", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewNodeOpenedHot", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewElementDisabled", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewElementNormal", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewElementHot", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewElementSelected", 7));
_TreeView->AddProperty(new ControlProperty(L"ScrollSize", 0));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonNormal", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonHot", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonPressed", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonDisabled", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonNormal", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonHot", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonPressed", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonDisabled", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarScrollerNormal", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarScrollerHot", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarScrollerPressed", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarScrollerDisabled", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarBarNormal", 7));
_TreeView->AddProperty(new ControlProperty(L"ViewScrollBarBarDisabled", 7));
}
return _TreeView;
}
ControlType * ListView(void)
{
if (!_ListView) {
_ListView = new ControlType(L"ListView");
_ListView->AddProperty(new ControlProperty(L"ID", 0));
_ListView->AddProperty(new ControlProperty(L"Invisible", 2));
_ListView->AddProperty(new ControlProperty(L"Disabled", 2));
_ListView->AddProperty(new ControlProperty(L"Font", 6));
_ListView->AddProperty(new ControlProperty(L"Border", 0));
_ListView->AddProperty(new ControlProperty(L"ElementHeight", 0));
_ListView->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"ViewNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewFocused", 7));
_ListView->AddProperty(new ControlProperty(L"ViewElementHot", 7));
_ListView->AddProperty(new ControlProperty(L"ViewElementSelected", 7));
_ListView->AddProperty(new ControlProperty(L"HeaderHeight", 0));
_ListView->AddProperty(new ControlProperty(L"HeaderStretchBarWidth", 0));
_ListView->AddProperty(new ControlProperty(L"ViewHeaderNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHeaderDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"ViewColumnHeaderNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewColumnHeaderHot", 7));
_ListView->AddProperty(new ControlProperty(L"ViewColumnHeaderPressed", 7));
_ListView->AddProperty(new ControlProperty(L"ViewColumnHeaderDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"VerticalScrollSize", 0));
_ListView->AddProperty(new ControlProperty(L"HorizontalScrollSize", 0));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonHot", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonPressed", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonHot", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonPressed", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerHot", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerPressed", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarBarNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarBarDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonHot", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonPressed", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonHot", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonPressed", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerHot", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerPressed", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerDisabled", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarBarNormal", 7));
_ListView->AddProperty(new ControlProperty(L"ViewVerticalScrollBarBarDisabled", 7));
}
return _ListView;
}
ControlType * ListViewColumn(void)
{
if (!_ListViewColumn) {
_ListViewColumn = new ControlType(L"ListViewColumn");
_ListViewColumn->AddProperty(new ControlProperty(L"ID", 0));
_ListViewColumn->AddProperty(new ControlProperty(L"MinimalWidth", 0));
_ListViewColumn->AddProperty(new ControlProperty(L"Text", 4));
_ListViewColumn->AddProperty(new ControlProperty(L"ViewElementDisabled", 7));
_ListViewColumn->AddProperty(new ControlProperty(L"ViewElementNormal", 7));
}
return _ListViewColumn;
}
ControlType * MultiLineEdit(void)
{
if (!_MultiLineEdit) {
_MultiLineEdit = new ControlType(L"MultiLineEdit");
_MultiLineEdit->AddProperty(new ControlProperty(L"ID", 0));
_MultiLineEdit->AddProperty(new ControlProperty(L"Invisible", 2));
_MultiLineEdit->AddProperty(new ControlProperty(L"Disabled", 2));
_MultiLineEdit->AddProperty(new ControlProperty(L"Text", 4));
_MultiLineEdit->AddProperty(new ControlProperty(L"Font", 6));
_MultiLineEdit->AddProperty(new ControlProperty(L"Border", 0));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewNormal", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewFocused", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewNormalReadOnly", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewFocusedReadOnly", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"Color", 3));
_MultiLineEdit->AddProperty(new ControlProperty(L"ColorDisabled", 3));
_MultiLineEdit->AddProperty(new ControlProperty(L"SelectionColor", 3));
_MultiLineEdit->AddProperty(new ControlProperty(L"CharactersEnabled", 4));
_MultiLineEdit->AddProperty(new ControlProperty(L"ReadOnly", 2));
_MultiLineEdit->AddProperty(new ControlProperty(L"UpperCase", 2));
_MultiLineEdit->AddProperty(new ControlProperty(L"LowerCase", 2));
_MultiLineEdit->AddProperty(new ControlProperty(L"ContextMenu", 8));
_MultiLineEdit->AddProperty(new ControlProperty(L"VerticalScrollSize", 0));
_MultiLineEdit->AddProperty(new ControlProperty(L"HorizontalScrollSize", 0));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonNormal", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonHot", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonPressed", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarLeftButtonDisabled", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonNormal", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonHot", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonPressed", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarRightButtonDisabled", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerNormal", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerHot", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerPressed", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarScrollerDisabled", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarBarNormal", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewHorizontalScrollBarBarDisabled", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonNormal", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonHot", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonPressed", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarUpButtonDisabled", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonNormal", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonHot", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonPressed", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarDownButtonDisabled", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerNormal", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerHot", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerPressed", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarScrollerDisabled", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarBarNormal", 7));
_MultiLineEdit->AddProperty(new ControlProperty(L"ViewVerticalScrollBarBarDisabled", 7));
}
return _MultiLineEdit;
}
ControlType * ColorView(void)
{
if (!_ColorView) {
_ColorView = new ControlType(L"ColorView");
_ColorView->AddProperty(new ControlProperty(L"ID", 0));
_ColorView->AddProperty(new ControlProperty(L"Invisible", 2));
_ColorView->AddProperty(new ControlProperty(L"Color", 3));
_ColorView->AddProperty(new ControlProperty(L"View", 7));
}
return _ColorView;
}
ControlType * ComboBox(void)
{
if (!_ComboBox) {
_ComboBox = new ControlType(L"ComboBox");
_ComboBox->AddProperty(new ControlProperty(L"ID", 0));
_ComboBox->AddProperty(new ControlProperty(L"Invisible", 2));
_ComboBox->AddProperty(new ControlProperty(L"Disabled", 2));
_ComboBox->AddProperty(new ControlProperty(L"Font", 6));
_ComboBox->AddProperty(new ControlProperty(L"ItemPosition", 9));
_ComboBox->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewNormal", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewFocused", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewHot", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewHotFocused", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewPressed", 7));
_ComboBox->AddProperty(new ControlProperty(L"Border", 0));
_ComboBox->AddProperty(new ControlProperty(L"ElementHeight", 0));
_ComboBox->AddProperty(new ControlProperty(L"ViewDropDownList", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewElementDisabled", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewElementNormal", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewElementDisabledPlaceholder", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewElementNormalPlaceholder", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewElementHot", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewElementSelected", 7));
_ComboBox->AddProperty(new ControlProperty(L"ScrollSize", 0));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonNormal", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonHot", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonPressed", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonDisabled", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonNormal", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonHot", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonPressed", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonDisabled", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerNormal", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerHot", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerPressed", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerDisabled", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarBarNormal", 7));
_ComboBox->AddProperty(new ControlProperty(L"ViewScrollBarBarDisabled", 7));
}
return _ComboBox;
}
ControlType * TextComboBox(void)
{
if (!_TextComboBox) {
_TextComboBox = new ControlType(L"TextComboBox");
_TextComboBox->AddProperty(new ControlProperty(L"ID", 0));
_TextComboBox->AddProperty(new ControlProperty(L"Invisible", 2));
_TextComboBox->AddProperty(new ControlProperty(L"Disabled", 2));
_TextComboBox->AddProperty(new ControlProperty(L"Font", 6));
_TextComboBox->AddProperty(new ControlProperty(L"TextPosition", 9));
_TextComboBox->AddProperty(new ControlProperty(L"ButtonPosition", 9));
_TextComboBox->AddProperty(new ControlProperty(L"ViewDisabled", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewNormal", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewFocused", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewButtonDisabled", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewButtonNormal", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewButtonHot", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewButtonPressed", 7));
_TextComboBox->AddProperty(new ControlProperty(L"Text", 4));
_TextComboBox->AddProperty(new ControlProperty(L"Color", 3));
_TextComboBox->AddProperty(new ControlProperty(L"AdviceColor", 3));
_TextComboBox->AddProperty(new ControlProperty(L"ColorDisabled", 3));
_TextComboBox->AddProperty(new ControlProperty(L"SelectionColor", 3));
_TextComboBox->AddProperty(new ControlProperty(L"Placeholder", 4));
_TextComboBox->AddProperty(new ControlProperty(L"PlaceholderFont", 6));
_TextComboBox->AddProperty(new ControlProperty(L"PlaceholderColor", 3));
_TextComboBox->AddProperty(new ControlProperty(L"PlaceholderColorDisabled", 3));
_TextComboBox->AddProperty(new ControlProperty(L"CharactersEnabled", 4));
_TextComboBox->AddProperty(new ControlProperty(L"UpperCase", 2));
_TextComboBox->AddProperty(new ControlProperty(L"LowerCase", 2));
_TextComboBox->AddProperty(new ControlProperty(L"ContextMenu", 8));
_TextComboBox->AddProperty(new ControlProperty(L"ViewElementDisabled", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewElementNormal", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewElementHot", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewElementSelected", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewDropDownList", 7));
_TextComboBox->AddProperty(new ControlProperty(L"Border", 0));
_TextComboBox->AddProperty(new ControlProperty(L"ElementHeight", 0));
_TextComboBox->AddProperty(new ControlProperty(L"ScrollSize", 0));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonNormal", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonHot", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonPressed", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarUpButtonDisabled", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonNormal", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonHot", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonPressed", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarDownButtonDisabled", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerNormal", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerHot", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerPressed", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarScrollerDisabled", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarBarNormal", 7));
_TextComboBox->AddProperty(new ControlProperty(L"ViewScrollBarBarDisabled", 7));
}
return _TextComboBox;
}
ControlType * VerticalTrackBar(void)
{
if (!_VerticalTrackBar) {
_VerticalTrackBar = new ControlType(L"VerticalTrackBar");
_VerticalTrackBar->AddProperty(new ControlProperty(L"ID", 0));
_VerticalTrackBar->AddProperty(new ControlProperty(L"Invisible", 2));
_VerticalTrackBar->AddProperty(new ControlProperty(L"Disabled", 2));
_VerticalTrackBar->AddProperty(new ControlProperty(L"RangeMinimal", 0));
_VerticalTrackBar->AddProperty(new ControlProperty(L"RangeMaximal", 0));
_VerticalTrackBar->AddProperty(new ControlProperty(L"Position", 0));
_VerticalTrackBar->AddProperty(new ControlProperty(L"Step", 0));
_VerticalTrackBar->AddProperty(new ControlProperty(L"TrackerWidth", 0));
_VerticalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerNormal", 7));
_VerticalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerFocused", 7));
_VerticalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerHot", 7));
_VerticalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerPressed", 7));
_VerticalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerDisabled", 7));
_VerticalTrackBar->AddProperty(new ControlProperty(L"ViewBarNormal", 7));
_VerticalTrackBar->AddProperty(new ControlProperty(L"ViewBarDisabled", 7));
}
return _VerticalTrackBar;
}
ControlType * HorizontalTrackBar(void)
{
if (!_HorizontalTrackBar) {
_HorizontalTrackBar = new ControlType(L"HorizontalTrackBar");
_HorizontalTrackBar->AddProperty(new ControlProperty(L"ID", 0));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"Invisible", 2));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"Disabled", 2));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"RangeMinimal", 0));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"RangeMaximal", 0));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"Position", 0));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"Step", 0));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"TrackerWidth", 0));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerNormal", 7));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerFocused", 7));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerHot", 7));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerPressed", 7));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"ViewTrackerDisabled", 7));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"ViewBarNormal", 7));
_HorizontalTrackBar->AddProperty(new ControlProperty(L"ViewBarDisabled", 7));
}
return _HorizontalTrackBar;
}
ControlType * CustomControl(void)
{
if (!_CustomControl) {
_CustomControl = new ControlType(L"CustomControl");
_CustomControl->AddProperty(new ControlProperty(L"ID", 0));
_CustomControl->AddProperty(new ControlProperty(L"Invisible", 2));
_CustomControl->AddProperty(new ControlProperty(L"Disabled", 2));
}
return _CustomControl;
}
}

*/