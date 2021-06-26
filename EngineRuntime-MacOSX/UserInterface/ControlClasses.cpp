#include "ControlClasses.h"

#define TEST_CLASS(class_name_string) if (class_name == ENGINE_STRING(class_name_string)) return new class_name_string;

namespace Engine
{
	namespace UI
	{
		namespace Template
		{
			namespace Controls
			{
				ControlReflectedBase * CreateControlByClass(const string & class_name)
				{
					TEST_CLASS(DialogFrame)
					else TEST_CLASS(ControlGroup)
					else TEST_CLASS(Button)
					else TEST_CLASS(Static)
					else TEST_CLASS(ColorView)
					else TEST_CLASS(ProgressBar)
					else TEST_CLASS(ComplexView)
					else TEST_CLASS(CheckBox)
					else TEST_CLASS(RadioButton)
					else TEST_CLASS(RadioButtonGroup)
					else TEST_CLASS(HorizontalScrollBar)
					else TEST_CLASS(VerticalScrollBar)
					else TEST_CLASS(PopupMenu)
					else TEST_CLASS(MenuItem)
					else TEST_CLASS(MenuSeparator)
					else TEST_CLASS(ToolButton)
					else TEST_CLASS(ToolButtonPart)
					else TEST_CLASS(Edit)
					else TEST_CLASS(ScrollBox)
					else TEST_CLASS(HorizontalSplitBox)
					else TEST_CLASS(VerticalSplitBox)
					else TEST_CLASS(SplitBoxPart)
					else TEST_CLASS(ListBox)
					else TEST_CLASS(TreeView)
					else TEST_CLASS(ListView)
					else TEST_CLASS(ListViewColumn)
					else TEST_CLASS(MultiLineEdit)
					else TEST_CLASS(ComboBox)
					else TEST_CLASS(TextComboBox)
					else TEST_CLASS(HorizontalTrackBar)
					else TEST_CLASS(VerticalTrackBar)
					else TEST_CLASS(BookmarkView)
					else TEST_CLASS(Bookmark)
					else TEST_CLASS(RichEdit)
					else TEST_CLASS(MenuBar)
					else TEST_CLASS(MenuBarElement)
					else TEST_CLASS(CustomControl)

					else TEST_CLASS(TouchBar)
					else TEST_CLASS(TouchBarButton)
					else TEST_CLASS(TouchBarGroup)
					else TEST_CLASS(TouchBarPopover)
					else TEST_CLASS(TouchBarColorPicker)
					else TEST_CLASS(TouchBarSlider)

					else TEST_CLASS(FrameExtendedData)
					else return 0;
				}
			}
		}
	}
}