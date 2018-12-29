#include <gtk/gtk.h>
#include "../../layer.h"
#include "../../layer_window.h"

#ifdef __cplusplus
extern "C" {
#endif

void UpdateLayerThumbnailWidget(LAYER* layer)
{
	gtk_widget_queue_draw(layer->widget->thumbnail);
}

#ifdef __cplusplus
}
#endif
