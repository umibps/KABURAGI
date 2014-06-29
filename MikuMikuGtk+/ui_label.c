#include "ui_label.h"
#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

UI_LABEL LoadDefaultUiLabel(void)
{
	const UI_LABEL default_label =
	{
		{
			"File",
			"Load Project",
			"Save Project",
			"Save Project As...",
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
			"Direction",
			"Model",
			"Bone",
			"Morph",
			"Load Models",
			"Load Pose",
			"Control Model",
			"No Select",
			"Reset",
			"Apply Center Position",
			"Scale",
			"Color",
			"Opacity",
			"Rate",
			"Edge Size",
			"Distance",
			"Field of View",
			"Connect to",
			"Enable Physics",
			"Display Grid",
			"Render Shadow",
			"Render Edge Only",
			{
				"Select",
				"Move",
				"Rotate"
			},
			{
				"Eye",
				"Lip",
				"Eye Blow",
				"Others"
			}
		}
	};

	return default_label;
}

UI_LABEL* GetUILabel(void* application_context)
{
	APPLICATION *application = (APPLICATION*)application_context;
	return &application->label;
}

#ifdef __cplusplus
}
#endif
