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
			"Camera",
			"Light",
			"Position",
			"Rotation",
			"Model",
			"Bone",
			"Load Models",
			"Load Pose",
			"Control Model",
			"No Select",
			"Reset",
			"Apply Center Position",
			"Scale",
			"Opacity",
			"Connect to",
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
