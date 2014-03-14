#include "ui_label.h"

UI_LABEL LoadDefaultUiLabel(void)
{
	const UI_LABEL default_label =
	{
		{
			"File",
			"Add Model/Accessory",
			"Edit",
			"Undo",
			"Redo"
		},
		{
			"Environment",
			"Bone",
			"Load Models",
			"Enable Physics",
			"Display Grid",
			{
				"Select",
				"Move",
				"Rotate"
			}
		}
	};

	return default_label;
}
