#include <string.h>
#include "labels.h"
#include "ini_file.h"
#include "utils.h"
#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

void LoadLabels(
	APPLICATION_LABELS* labels,
	FRACTAL_LABEL* fractal_labels,
	const char* lang_file_path
)
{
#define MAX_STR_SIZE 512
	// 初期化ファイル解析用
	INI_FILE_PTR file;
	// ファイル読み込みストリーム
	GFile* fp = g_file_new_for_path(lang_file_path);
	GFileInputStream* stream = g_file_read(fp, NULL, NULL);
	// ファイルサイズ取得用
	GFileInfo *file_info;
	// ファイルサイズ
	size_t data_size;
	char temp_str[MAX_STR_SIZE], lang[MAX_STR_SIZE];
	size_t length;
	int i;

	// ファイルオープンに失敗したら終了
	if(stream == NULL)
	{
		g_object_unref(fp);
		return;
	}

	// ファイルサイズを取得
	file_info = g_file_input_stream_query_info(stream,
		G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, NULL);
	data_size = (size_t)g_file_info_get_size(file_info);
	
	file = CreateIniFile(stream,
		(size_t (*)(void*, size_t, size_t, void*))FileRead, data_size, INI_READ);

	// 文字コードを取得
	(void)IniFileGetString(file, "CODE", "CODE_TYPE", lang, MAX_STR_SIZE);

	// 言語名を取得
	length = IniFileGetString(file, "LANGUAGE", "LANGUAGE", temp_str, MAX_STR_SIZE);
	labels->language_name = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// ウィンドウの基本キャプション
	length = IniFileGetString(file, "WINDOW", "OK", temp_str, MAX_STR_SIZE);
	labels->window.ok = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "APPLY", temp_str, MAX_STR_SIZE);
	labels->window.apply = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "CANCEL", temp_str, MAX_STR_SIZE);
	labels->window.cancel = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "NORMAL", temp_str, MAX_STR_SIZE);
	labels->window.normal = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "REVERSE", temp_str, MAX_STR_SIZE);
	labels->window.reverse = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "EDIT_SELECTION", temp_str, MAX_STR_SIZE);
	labels->window.edit_selection = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "WINDOW", temp_str, MAX_STR_SIZE);
	labels->window.window = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "CLOSE_WINDOW", temp_str, MAX_STR_SIZE);
	labels->window.close_window = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "DOCK_LEFT", temp_str, MAX_STR_SIZE);
	labels->window.place_left = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "DOCK_RIGHT", temp_str, MAX_STR_SIZE);
	labels->window.place_right = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "FULLSCREEN", temp_str, MAX_STR_SIZE);
	labels->window.fullscreen = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "REFERENCE", temp_str, MAX_STR_SIZE);
	labels->window.reference = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "MOVE_TOP_LEFT", temp_str, MAX_STR_SIZE);
	labels->window.move_top_left = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "HOT_KEY", temp_str, MAX_STR_SIZE);
	labels->window.hot_key = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "LOADING", temp_str, MAX_STR_SIZE);
	labels->window.loading = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "WINDOW", "SAVING", temp_str, MAX_STR_SIZE);
	labels->window.saving = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// 単位
	length = IniFileGetString(file, "COMMON", "PIXEL", temp_str, MAX_STR_SIZE);
	labels->unit.pixel = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "LENGTH", temp_str, MAX_STR_SIZE);
	labels->unit.length = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "ANGLE", temp_str, MAX_STR_SIZE);
	labels->unit.angle = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "BACK_GROUND", temp_str, MAX_STR_SIZE);
	labels->unit.bg = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "REPEAT", temp_str, MAX_STR_SIZE);
	labels->unit.repeat = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "PREVIEW", temp_str, MAX_STR_SIZE);
	labels->unit.preview = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "INTERVAL", temp_str, MAX_STR_SIZE);
	labels->unit.interval = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "MINUTE", temp_str, MAX_STR_SIZE);
	labels->unit.minute = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "DETAIL", temp_str, MAX_STR_SIZE);
	labels->unit.detail = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "TARGET", temp_str, MAX_STR_SIZE);
	labels->unit.target = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "CLIPBOARD", temp_str, MAX_STR_SIZE);
	labels->unit.clip_board = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "NAME", temp_str, MAX_STR_SIZE);
	labels->unit.name = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "TYPE", temp_str, MAX_STR_SIZE);
	labels->unit.type = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "RESOLUTION", temp_str, MAX_STR_SIZE);
	labels->unit.resolution = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "CENTER", temp_str, MAX_STR_SIZE);
	labels->unit.center = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "STRAIGHT", temp_str, MAX_STR_SIZE);
	labels->unit.straight = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "EXTEND", temp_str, MAX_STR_SIZE);
	labels->unit.extend = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "MODE", temp_str, MAX_STR_SIZE);
	labels->unit.mode = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "RED", temp_str, MAX_STR_SIZE);
	labels->unit.red = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "GREEN", temp_str, MAX_STR_SIZE);
	labels->unit.green = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "BLUE", temp_str, MAX_STR_SIZE);
	labels->unit.blue = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "CYAN", temp_str, MAX_STR_SIZE);
	labels->unit.cyan = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "MAGENTA", temp_str, MAX_STR_SIZE);
	labels->unit.magenta = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "YELLOW", temp_str, MAX_STR_SIZE);
	labels->unit.yellow = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "KEYPLATE", temp_str, MAX_STR_SIZE);
	labels->unit.key_plate = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "ADD", temp_str, MAX_STR_SIZE);
	labels->unit.add = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "COMMON", "DELETE", temp_str, MAX_STR_SIZE);
	labels->unit._delete = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// メニューバー
		// ファイルメニュー
	length = IniFileGetString(file, "FILE", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.file = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILE", "NEW", temp_str, MAX_STR_SIZE);
	labels->menu.make_new = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILE", "OPEN", temp_str, MAX_STR_SIZE);
	labels->menu.open = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILE", "OPEN_AS_LAYER", temp_str, MAX_STR_SIZE);
	labels->menu.open_as_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILE", "SAVE", temp_str, MAX_STR_SIZE);
	labels->menu.save = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILE", "SAVE_AS", temp_str, MAX_STR_SIZE);
	labels->menu.save_as = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILE", "CLOSE", temp_str, MAX_STR_SIZE);
	labels->menu.close = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILE", "QUIT", temp_str, MAX_STR_SIZE);
	labels->menu.quit = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// 編集メニュー
	length = IniFileGetString(file, "EDIT", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.edit = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "EDIT", "UNDO", temp_str, MAX_STR_SIZE);
	labels->menu.undo = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "EDIT", "REDO", temp_str, MAX_STR_SIZE);
	labels->menu.redo = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "EDIT", "COPY", temp_str, MAX_STR_SIZE);
	labels->menu.copy = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "EDIT", "COPY_VISIBLE", temp_str, MAX_STR_SIZE);
	labels->menu.copy_visible = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "EDIT", "CUT", temp_str, MAX_STR_SIZE);
	labels->menu.cut = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "EDIT", "PASTE", temp_str, MAX_STR_SIZE);
	labels->menu.paste = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "EDIT", "CLIPBOARD", temp_str, MAX_STR_SIZE);
	labels->menu.clip_board = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "EDIT", "TRANSFORM", temp_str, MAX_STR_SIZE);
	labels->menu.transform = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "EDIT", "PROJECTION", temp_str, MAX_STR_SIZE);
	labels->menu.projection = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// キャンバスメニュー
	length = IniFileGetString(file, "CANVAS", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.canvas = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "CANVAS", "CHANGE_RESOLUTION", temp_str, MAX_STR_SIZE);
	labels->menu.change_resolution = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "CANVAS", "CHANGE_CANVAS_SIZE", temp_str, MAX_STR_SIZE);
	labels->menu.change_canvas_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "CANVAS", "FLIP_HORIZONTALLY", temp_str, MAX_STR_SIZE);
	labels->menu.flip_canvas_horizontally = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "CANVAS", "FLIP_VERTICALLY", temp_str, MAX_STR_SIZE);
	labels->menu.flip_canvas_vertically = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "CANVAS", "CHANGE_2ND_BG_COLOR", temp_str, MAX_STR_SIZE);
	labels->menu.change_2nd_bg_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "CANVAS", "SWITCH_BG_COLOR", temp_str, MAX_STR_SIZE);
	labels->menu.switch_bg_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "CANVAS", "CHANGE_ICC_PROFILE", temp_str, MAX_STR_SIZE);
	labels->menu.canvas_icc = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// レイヤーメニュー
	length = IniFileGetString(file, "LAYER", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "NEW(COLOR)", temp_str, MAX_STR_SIZE);
	labels->menu.new_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "NEW(VECTOR)", temp_str, MAX_STR_SIZE);
	labels->menu.new_vector = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "NEW_LAYER_SET", temp_str, MAX_STR_SIZE);
	labels->menu.new_layer_set = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "NEW_3D_MODELING", temp_str, MAX_STR_SIZE);
	labels->menu.new_3d_modeling = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "NEW_ADJUSTMENT_LAYER", temp_str, MAX_STR_SIZE);
	labels->menu.new_adjustment_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "COPY", temp_str, MAX_STR_SIZE);
	labels->menu.copy_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "DELETE", temp_str, MAX_STR_SIZE);
	labels->menu.delete_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "MERGE_DOWN", temp_str, MAX_STR_SIZE);
	labels->menu.merge_down_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "FLATTEN_IMAGE", temp_str, MAX_STR_SIZE);
	labels->menu.merge_all_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "FILL_FG", temp_str, MAX_STR_SIZE);
	labels->menu.fill_layer_fg_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "FILL_PATTERN", temp_str, MAX_STR_SIZE);
	labels->menu.fill_layer_pattern = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "RASTERIZE_LAYER", temp_str, MAX_STR_SIZE);
	labels->menu.rasterize_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "ALPHA_TO_SELECT", temp_str, MAX_STR_SIZE);
	labels->layer_window.alpha_to_select = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "ALPHA_ADD_SELECT", temp_str, MAX_STR_SIZE);
	labels->layer_window.alpha_add_select = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "PASTED_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.pasted_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "UNDER_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.under_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "MIXED_UNDER_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.mixed_under_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "VISIBLE_TO_LAYER", temp_str, MAX_STR_SIZE);
	labels->menu.visible2layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER", "VISIBLE_COPY", temp_str, MAX_STR_SIZE);
	labels->menu.visible_copy = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// 選択メニュー
	length = IniFileGetString(file, "SELECT", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.select = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SELECT", "NONE", temp_str, MAX_STR_SIZE);
	labels->menu.select_none = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SELECT", "INVERT", temp_str, MAX_STR_SIZE);
	labels->menu.select_invert = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SELECT", "ALL", temp_str, MAX_STR_SIZE);
	labels->menu.select_all = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SELECT", "GROW", temp_str, MAX_STR_SIZE);
	labels->menu.selection_extend = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SELECT", "SHRINK", temp_str, MAX_STR_SIZE);
	labels->menu.selection_reduct = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// ビューメニュー
	length = IniFileGetString(file, "VIEW", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.view = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "ZOOM", temp_str, MAX_STR_SIZE);
	labels->menu.zoom = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "ZOOM_IN", temp_str, MAX_STR_SIZE);
	labels->menu.zoom_in = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "ZOOM_OUT", temp_str, MAX_STR_SIZE);
	labels->menu.zoom_out = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "ZOOM_RESET", temp_str, MAX_STR_SIZE);
	labels->menu.zoom_reset = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "ROTATE", temp_str, MAX_STR_SIZE);
	labels->menu.rotate = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "ROTATE_RESET", temp_str, MAX_STR_SIZE);
	labels->menu.reset_rotate = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "REVERSE", temp_str, MAX_STR_SIZE);
	labels->menu.reverse_horizontally = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "DISPLAY_FILTER", temp_str, MAX_STR_SIZE);
	labels->menu.display_filter = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "NO_FILTER", temp_str, MAX_STR_SIZE);
	labels->menu.no_filter = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "GRAY_SCALE", temp_str, MAX_STR_SIZE);
	labels->menu.gray_scale = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "VIEW", "GRAY_SCALE_YIQ", temp_str, MAX_STR_SIZE);
	labels->menu.gray_scale_yiq = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// フィルタメニュー
	length = IniFileGetString(file, "FILTERS", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.filters = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "BLUR", temp_str, MAX_STR_SIZE);
	labels->menu.blur = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "MOTION_BLUR", temp_str, MAX_STR_SIZE);
	labels->menu.motion_blur = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "GAUSSIAN_BLUR", temp_str, MAX_STR_SIZE);
	labels->menu.gaussian_blur = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "STRAIGHT_RANDOM", temp_str, MAX_STR_SIZE);
	labels->filter.straight_random = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "BIDIRECTION", temp_str, MAX_STR_SIZE);
	labels->filter.bidirection = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "BRIGHTNESS_CONTRAST", temp_str, MAX_STR_SIZE);
	labels->menu.bright_contrast = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "HUE_SATURATION", temp_str, MAX_STR_SIZE);
	labels->menu.hue_saturtion = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "LEVELS", temp_str, MAX_STR_SIZE);
	labels->menu.color_levels = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "TONE_CURVE", temp_str, MAX_STR_SIZE);
	labels->menu.tone_curve = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "LUMINOSITY_TO_OPACITY", temp_str, MAX_STR_SIZE);
	labels->menu.luminosity2opacity = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "COLOR_TO_ALPHA", temp_str, MAX_STR_SIZE);
	labels->menu.color2alpha = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "COLORIZE_WITH_UNDER", temp_str, MAX_STR_SIZE);
	labels->menu.colorize_with_under = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "GRADATION_MAP", temp_str, MAX_STR_SIZE);
	labels->menu.gradation_map = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "DETECT_MAX", temp_str, MAX_STR_SIZE);
	labels->menu.detect_max = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "TRANSPARENCY_AS_WHITE", temp_str, MAX_STR_SIZE);
	labels->menu.tranparancy_as_white = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "FILL_WITH_VECTOR", temp_str, MAX_STR_SIZE);
	labels->menu.fill_with_vector = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "RENDER", temp_str, MAX_STR_SIZE);
	labels->menu.render = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "CLOUD", temp_str, MAX_STR_SIZE);
	labels->menu.cloud = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "FRACTAL", temp_str, MAX_STR_SIZE);
	labels->menu.fractal = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "TRACE_PIXEL", temp_str, MAX_STR_SIZE);
	labels->menu.trace_pixels = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// 新規作成ウィンドウ
	length = IniFileGetString(file, "NEW", "TITLE", temp_str, MAX_STR_SIZE);
	labels->make_new.title = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "NEW", "NAME", temp_str, MAX_STR_SIZE);
	labels->make_new.name = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "NEW", "WIDTH", temp_str, MAX_STR_SIZE);
	labels->make_new.width = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "NEW", "HEIGHT", temp_str, MAX_STR_SIZE);
	labels->make_new.height = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "NEW", "SECOND_BG_COLOR", temp_str, MAX_STR_SIZE);
	labels->make_new.second_bg_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "NEW", "ADOPT_ICC_PROFILE", temp_str, MAX_STR_SIZE);
	labels->make_new.adopt_icc_profile = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "NEW", "PRESET", temp_str, MAX_STR_SIZE);
	labels->make_new.preset = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "NEW", "ADD_PRESET", temp_str, MAX_STR_SIZE);
	labels->make_new.add_preset = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "NEW", "SWAP_HEIGHT_AND_WIDTH", temp_str, MAX_STR_SIZE);
	labels->make_new.swap_height_and_width = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// ツールボックス
	length = IniFileGetString(file, "TOOL_BOX", "TITLE", temp_str, MAX_STR_SIZE);
	labels->tool_box.title = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "INITITIALIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.initialize = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "NEW_BRUSH", temp_str, MAX_STR_SIZE);
	labels->tool_box.new_brush = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SMOOTH", temp_str, MAX_STR_SIZE);
	labels->tool_box.smooth = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SMOOTH_QUALITY", temp_str, MAX_STR_SIZE);
	labels->tool_box.smooth_quality = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SMOOTH_RATE", temp_str, MAX_STR_SIZE);
	labels->tool_box.smooth_rate = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SMOOTH_GAUSSIAN", temp_str, MAX_STR_SIZE);
	labels->tool_box.smooth_gaussian = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SMOOTH_AVERAGE", temp_str, MAX_STR_SIZE);
	labels->tool_box.smooth_average = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "MAGNIFICATION", temp_str, MAX_STR_SIZE);
	labels->tool_box.base_scale = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "BRUSH_SIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.brush_scale = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SCALE", temp_str, MAX_STR_SIZE);
	labels->tool_box.scale = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "FLOW", temp_str, MAX_STR_SIZE);
	labels->tool_box.flow = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "PRESSURE", temp_str, MAX_STR_SIZE);
	labels->tool_box.pressure = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "RANGE_EXTEND", temp_str, MAX_STR_SIZE);
	labels->tool_box.extend = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "BLUR", temp_str, MAX_STR_SIZE);
	labels->tool_box.blur = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "OUTLINE_HARDNESS", temp_str, MAX_STR_SIZE);
	labels->tool_box.outline_hardness = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "COLOR_EXTEND", temp_str, MAX_STR_SIZE);
	labels->tool_box.color_extend = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_MOVE_START", temp_str, MAX_STR_SIZE);
	labels->tool_box.select_move_start = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ANTI_ALIAS", temp_str, MAX_STR_SIZE);
	labels->tool_box.anti_alias = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_RGB", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.rgb = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_RGBA", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.rgba = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_ALPHA", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.alpha = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_ACTIVE_LAYER", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.active_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_CANVAS", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.canvas = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_UNDER_LAYER", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.under_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_THRESHOLD", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.threshold = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_TARGET", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.target = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_AREA", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.area = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SELECT_AREA_SIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.detect_area = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "AREA_NORMAL", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.area_normal = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "AREA_LARGE", temp_str, MAX_STR_SIZE);
	labels->tool_box.select.area_large = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CHANGE_TEXT_COLOR", temp_str, MAX_STR_SIZE);
	labels->tool_box.change_text_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TEXT_HORIZONAL", temp_str, MAX_STR_SIZE);
	labels->tool_box.text_horizonal = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TEXT_VERTICAL", temp_str, MAX_STR_SIZE);
	labels->tool_box.text_vertical = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TEXT_STYLE", temp_str, MAX_STR_SIZE);
	labels->tool_box.text_style = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TEXT_BOLD", temp_str, MAX_STR_SIZE);
	labels->tool_box.text_bold = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TEXT_NORMAL", temp_str, MAX_STR_SIZE);
	labels->tool_box.text_normal = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TEXT_ITALIC", temp_str, MAX_STR_SIZE);
	labels->tool_box.text_italic = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TEXT_OBLIQUE", temp_str, MAX_STR_SIZE);
	labels->tool_box.text_oblique = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "HAS_BALLOON", temp_str, MAX_STR_SIZE);
	labels->tool_box.has_balloon = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "BALLOON_HAS_EDGE", temp_str, MAX_STR_SIZE);
	labels->tool_box.balloon_has_edge = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "LINE_COLOR", temp_str, MAX_STR_SIZE);
	labels->tool_box.line_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "FILL_COLOR", temp_str, MAX_STR_SIZE);
	labels->tool_box.fill_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "LINE_WIDTH", temp_str, MAX_STR_SIZE);
	labels->tool_box.line_width = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CENTERING_HORIZONTALLY", temp_str, MAX_STR_SIZE);
	labels->tool_box.centering_horizontally = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CENTERING_VERTICALLY", temp_str, MAX_STR_SIZE);
	labels->tool_box.centering_vertically = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "AJDUST_RANGE_TO_TEXT", temp_str, MAX_STR_SIZE);
	labels->tool_box.adjust_range_to_text = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "NUM_EDGE", temp_str, MAX_STR_SIZE);
	labels->tool_box.num_edge = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "EDGE_SIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.edge_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "EDGE_SIZE_RANDOM", temp_str, MAX_STR_SIZE);
	labels->tool_box.random_edge_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "EDGE_DISTANCE_RANDOM", temp_str, MAX_STR_SIZE);
	labels->tool_box.random_edge_distance = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "NUM_CHILDREN", temp_str, MAX_STR_SIZE);
	labels->tool_box.num_children = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "START_CHILD_SIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.start_child_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "END_CHILD_SIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.end_child_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "REVERSE", temp_str, MAX_STR_SIZE);
	labels->tool_box.reverse = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "REVERSE_HORIZONTALLY", temp_str, MAX_STR_SIZE);
	labels->tool_box.reverse_horizontally = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "REVERSE_VERTICALLY", temp_str, MAX_STR_SIZE);
	labels->tool_box.reverse_vertically = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "BLEND_MODE", temp_str, MAX_STR_SIZE);
	labels->tool_box.blend_mode = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "HUE", temp_str, MAX_STR_SIZE);
	labels->tool_box.hue = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SATURATION", temp_str, MAX_STR_SIZE);
	labels->tool_box.saturation = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "BRIGHTNESS", temp_str, MAX_STR_SIZE);
	labels->tool_box.brightness = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CONTRAST", temp_str, MAX_STR_SIZE);
	labels->tool_box.contrast = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "DISTANCE", temp_str, MAX_STR_SIZE);
	labels->tool_box.distance = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ROTATE_START", temp_str, MAX_STR_SIZE);
	labels->tool_box.rotate_start = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ROTATE_SPEED", temp_str, MAX_STR_SIZE);
	labels->tool_box.rotate_speed = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ROTATE_TO_BRUSH_DIRECTION", temp_str, MAX_STR_SIZE);
	labels->tool_box.rotate_to_brush_direction = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "RANDOM_ROTATE", temp_str, MAX_STR_SIZE);
	labels->tool_box.random_rotate = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ROTATE_RANGE", temp_str, MAX_STR_SIZE);
	labels->tool_box.rotate_range = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "RANDOM_SIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.random_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SIZE_RANGE", temp_str, MAX_STR_SIZE);
	labels->tool_box.size_range = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "PICK_MODE", temp_str, MAX_STR_SIZE);
	labels->tool_box.pick_mode = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SINGLE_PIXEL", temp_str, MAX_STR_SIZE);
	labels->tool_box.single_pixels = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "AVERAGE_COLOR", temp_str, MAX_STR_SIZE);
	labels->tool_box.average_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CLOCKWISE", temp_str, MAX_STR_SIZE);
	labels->tool_box.clockwise = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "COUNTER_CLOCKWISE", temp_str, MAX_STR_SIZE);
	labels->tool_box.counter_clockwise = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "MINIMUM_DEGREE", temp_str, MAX_STR_SIZE);
	labels->tool_box.min_degree = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "MINIMUM_DISTANCE", temp_str, MAX_STR_SIZE);
	labels->tool_box.min_distance = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "MINIMUM_PRESSURE", temp_str, MAX_STR_SIZE);
	labels->tool_box.min_pressure = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ENTER", temp_str, MAX_STR_SIZE);
	labels->tool_box.enter = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "OUT", temp_str, MAX_STR_SIZE);
	labels->tool_box.out = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ENTER_AND_OUT", temp_str, MAX_STR_SIZE);
	labels->tool_box.enter_out = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "MIX", temp_str, MAX_STR_SIZE);
	labels->tool_box.mix = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "REVERSE_FG_TO_BG", temp_str, MAX_STR_SIZE);
	labels->tool_box.gradation_reverse = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "DEVIDE_STROKE", temp_str, MAX_STR_SIZE);
	labels->tool_box.devide_stroke = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "DELETE_STROKE", temp_str, MAX_STR_SIZE);
	labels->tool_box.delete_stroke = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "OPEN_PATH", temp_str, MAX_STR_SIZE);
	labels->tool_box.open_path = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CLOSE_PATH", temp_str, MAX_STR_SIZE);
	labels->tool_box.close_path = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TARGET", temp_str, MAX_STR_SIZE);
	labels->tool_box.target = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "STROKE", temp_str, MAX_STR_SIZE);
	labels->tool_box.stroke = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "PRIOR_ANGLE", temp_str, MAX_STR_SIZE);
	labels->tool_box.prior_angle = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TRANSFORM_FREE", temp_str, MAX_STR_SIZE);
	labels->tool_box.transform_free = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TRANSFORM_SCALE", temp_str, MAX_STR_SIZE);
	labels->tool_box.transform_scale = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TRANSFORM_FREE_SHAPE", temp_str, MAX_STR_SIZE);
	labels->tool_box.transform_free_shape = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TRANSFORM_ROTATE", temp_str, MAX_STR_SIZE);
	labels->tool_box.transform_rotate = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "PREFERENCE", temp_str, MAX_STR_SIZE);
	labels->tool_box.preference = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "NAME", temp_str, MAX_STR_SIZE);
	labels->tool_box.name = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "COPY_BRUSH", temp_str, MAX_STR_SIZE);
	labels->tool_box.copy_brush = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CHANGE_BRUSH", temp_str, MAX_STR_SIZE);
	labels->tool_box.change_brush = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "DELETE_BRUSH", temp_str, MAX_STR_SIZE);
	labels->tool_box.delete_brush = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CONTROL_POINT", temp_str, MAX_STR_SIZE);
	labels->tool_box.control_point = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CONTROL_SELECT_POINT", temp_str, MAX_STR_SIZE);
	labels->tool_box.control.select = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CONTROL_MOVE_POINT", temp_str, MAX_STR_SIZE);
	labels->tool_box.control.move = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CONTROL_CHANGE_PRESSURE", temp_str, MAX_STR_SIZE);
	labels->tool_box.control.change_pressure = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CONTROL_DELETE_POINT", temp_str, MAX_STR_SIZE);
	labels->tool_box.control.delete_point = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CONTROL_MOVE_STROKE", temp_str, MAX_STR_SIZE);
	labels->tool_box.control.move_stroke = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CONTROL_COPY_AND_MOVE_STROKE", temp_str, MAX_STR_SIZE);
	labels->tool_box.control.copy_stroke = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CONTROL_JOINT_STROKE", temp_str, MAX_STR_SIZE);
	labels->tool_box.control.joint_stroke = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TEXTURE", temp_str, MAX_STR_SIZE);
	labels->tool_box.texture = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "TEXTURE_STRENGTH", temp_str, MAX_STR_SIZE);
	labels->tool_box.texture_strength = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "NO_TEXTURE", temp_str, MAX_STR_SIZE);
	labels->tool_box.no_texture = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ADD_COLOR", temp_str, MAX_STR_SIZE);
	labels->tool_box.pallete_add = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "DELETE_COLOR", temp_str, MAX_STR_SIZE);
	labels->tool_box.pallete_delete = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "LOAD_PALLETE", temp_str, MAX_STR_SIZE);
	labels->tool_box.load_pallete = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ADD_PALLETE", temp_str, MAX_STR_SIZE);
	labels->tool_box.load_pallete_after = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "WRITE_PALLETE", temp_str, MAX_STR_SIZE);
	labels->tool_box.write_pallete = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CLEAR_PALLETE", temp_str, MAX_STR_SIZE);
	labels->tool_box.clear_pallete = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "UPDATE", temp_str, MAX_STR_SIZE);
	labels->tool_box.update = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "FREQUENCY", temp_str, MAX_STR_SIZE);
	labels->tool_box.frequency = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CLOUD_COLOR", temp_str, MAX_STR_SIZE);
	labels->tool_box.cloud_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "PERSISTENCE", temp_str, MAX_STR_SIZE);
	labels->tool_box.persistence = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "RANDOM_SEED", temp_str, MAX_STR_SIZE);
	labels->tool_box.rand_seed = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "USE_RANDOM", temp_str, MAX_STR_SIZE);
	labels->tool_box.use_random = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "UPDATE_IMMEDIATELY", temp_str, MAX_STR_SIZE);
	labels->tool_box.update_immediately = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "NUMBER_OF_OCTAVES", temp_str, MAX_STR_SIZE);
	labels->tool_box.num_octaves = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "LINEAR", temp_str, MAX_STR_SIZE);
	labels->tool_box.linear = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "COSINE", temp_str, MAX_STR_SIZE);
	labels->tool_box.cosine = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "CUBIC", temp_str, MAX_STR_SIZE);
	labels->tool_box.cubic = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "COLORIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.colorize = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "START_EDIT_3D_MODEL", temp_str, MAX_STR_SIZE);
	labels->tool_box.start_edit_3d = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "END_EDIT_3D_MODEL", temp_str, MAX_STR_SIZE);
	labels->tool_box.end_edit_3d = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SCATTER", temp_str, MAX_STR_SIZE);
	labels->tool_box.scatter = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SCATTER_SIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.scatter_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SCATTER_RANGE", temp_str, MAX_STR_SIZE);
	labels->tool_box.scatter_range = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SCATTER_RANDOM_SIZE", temp_str, MAX_STR_SIZE);
	labels->tool_box.scatter_random_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SCATTER_RANDOM_FLOW", temp_str, MAX_STR_SIZE);
	labels->tool_box.scatter_random_flow = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SHAPE_CIRCLE", temp_str, MAX_STR_SIZE);
	labels->tool_box.shape.circle = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SHAPE_ECLIPSE", temp_str, MAX_STR_SIZE);
	labels->tool_box.shape.eclipse = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SHAPE_TRIANGLE", temp_str, MAX_STR_SIZE);
	labels->tool_box.shape.triangle = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SHAPE_SQUARE", temp_str, MAX_STR_SIZE);
	labels->tool_box.shape.square = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SHAPE_RHOMBUS", temp_str, MAX_STR_SIZE);
	labels->tool_box.shape.rhombus = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SHAPE_HEXAGON", temp_str, MAX_STR_SIZE);
	labels->tool_box.shape.hexagon = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SHAPE_STAR", temp_str, MAX_STR_SIZE);
	labels->tool_box.shape.star = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SHAPE_PATTERN", temp_str, MAX_STR_SIZE);
	labels->tool_box.shape.pattern = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "SHAPE_IMAGE", temp_str, MAX_STR_SIZE);
	labels->tool_box.shape.image = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "BEVEL", temp_str, MAX_STR_SIZE);
	labels->tool_box.bevel = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "ROUND", temp_str, MAX_STR_SIZE);
	labels->tool_box.round = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "MITER", temp_str, MAX_STR_SIZE);
	labels->tool_box.miter = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "MANUALLY_SET", temp_str, MAX_STR_SIZE);
	labels->tool_box.manually_set = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// レイヤーウィンドウ
	length = IniFileGetString(file, "LAYER_WINDOW", "TITLE", temp_str, MAX_STR_SIZE);
	labels->layer_window.title = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "NORMAL_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.new_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "VECTOR_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.new_vector = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "LAYER_SET", temp_str, MAX_STR_SIZE);
	labels->layer_window.new_layer_set = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "3D_MODELING", temp_str, MAX_STR_SIZE);
	labels->layer_window.new_3d_modeling = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "TEXT_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.new_text = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "ADJUSTMENT_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.new_adjsutment_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "ADD_NORMAL_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.add_normal = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "ADD_VECTOR_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.add_vector = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "ADD_LAYER_SET", temp_str, MAX_STR_SIZE);
	labels->layer_window.add_layer_set = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "ADD_3D_MODELING", temp_str, MAX_STR_SIZE);
	labels->layer_window.add_3d_modeling = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "ADD_ADJUSTMENT_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.add_adjustment_layer = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "RENAME", temp_str, MAX_STR_SIZE);
	labels->layer_window.rename = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "REORDER", temp_str, MAX_STR_SIZE);
	labels->layer_window.reorder = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "BLEND_MODE", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_mode = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "OPACITY", temp_str, MAX_STR_SIZE);
	labels->layer_window.opacity = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "MASK_WITH_UNDER", temp_str, MAX_STR_SIZE);
	labels->layer_window.mask_with_under = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "LOCK_OPACITY", temp_str, MAX_STR_SIZE);
	labels->layer_window.lock_opacity = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// レイヤーの合成モード
	length = IniFileGetString(file, "BLEND_MODE", "NORMAL", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_NORMAL] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "ADD", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_ADD] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "MULTIPLY", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_MULTIPLY] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "SCREEN", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_SCREEN] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "OVERLAY", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_OVERLAY] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "LIGHTEN", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_LIGHTEN] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "DARKEN", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_DARKEN] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "DODGE", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_DODGE] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "BURN", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_BURN] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "HARD_LIGHT", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_HARD_LIGHT] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "SOFT_LIGHT", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_SOFT_LIGHT] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "DIFFERENCE", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_DIFFERENCE] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "EXCLUSION", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_EXCLUSION] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "HUE", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_HSL_HUE] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "SATURATION", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_HSL_SATURATION] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "COLOR", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_HSL_COLOR] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "LUMINOSITY", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_HSL_LUMINOSITY] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "BLEND_MODE", "BINALIZE", temp_str, MAX_STR_SIZE);
	labels->layer_window.blend_modes[LAYER_BLEND_BINALIZE] = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// 保存ウィンドウ用
	length = IniFileGetString(file, "SAVE", "COMPRESS", temp_str, MAX_STR_SIZE);
	labels->save.compress = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SAVE", "QUALITY", temp_str, MAX_STR_SIZE);
	labels->save.quality = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SAVE", "WRITE_ALPHA", temp_str, MAX_STR_SIZE);
	labels->save.write_alpha = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SAVE", "WRITE_PROFILE", temp_str, MAX_STR_SIZE);
	labels->save.write_profile = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SAVE", "CLOSE_WITH_UNSAVED_CHANGE", temp_str, MAX_STR_SIZE);
	labels->save.close_with_unsaved_chage = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SAVE", "QUIT_WITH_UNSAVED_CHANGE", temp_str, MAX_STR_SIZE);
	labels->save.quit_with_unsaved_change = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "SAVE", "RECOVER_BACKUP", temp_str, MAX_STR_SIZE);
	labels->save.recover_backup = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// ナビゲーション用
	length = IniFileGetString(file, "NAVIGATION", "TITLE", temp_str, MAX_STR_SIZE);
	labels->navigation.title = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// プラグイン
	length = IniFileGetString(file, "PLUG_IN", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.plug_in = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// スクリプト
	length = IniFileGetString(file, "SCRIPT", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.script = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// ヘルプメニュー
	length = IniFileGetString(file, "HELP", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->menu.help = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "HELP", "VERSION_INFO", temp_str, MAX_STR_SIZE);
	labels->menu.version = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	// 環境設定
	length = IniFileGetString(file, "PREFERENCE", "MENU_NAME", temp_str, MAX_STR_SIZE);
	labels->preference.title = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "BASE_SETTING", temp_str, MAX_STR_SIZE);
	labels->preference.base_setting = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "THEME", temp_str, MAX_STR_SIZE);
	labels->preference.theme = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "DEFAULT_THEME", temp_str, MAX_STR_SIZE);
	labels->preference.default_theme = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "AUTO_SAVE", temp_str, MAX_STR_SIZE);
	labels->preference.auto_save = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "AUTO_SAVE_STATUS", temp_str, MAX_STR_SIZE);
	labels->status_bar.auto_save = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "HOT_KEY_CONFLICT", temp_str, MAX_STR_SIZE);
	labels->preference.conflict_hot_key = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "LANGUAGE", temp_str, MAX_STR_SIZE);
	labels->preference.language = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "CHANGE_BACK_GROUND", temp_str, MAX_STR_SIZE);
	labels->preference.set_back_ground = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "SHOW_PREVIEW_TASKBAR", temp_str, MAX_STR_SIZE);
	labels->preference.show_preview_on_taskbar = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "BACKUP_DIRECTORY", temp_str, MAX_STR_SIZE);
	labels->preference.backup_path = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
#if GTK_MAJOR_VERSION >= 3
	length = IniFileGetString(file, "PREFERENCE", "SCALE_AND_MOVE_WITH_TOUCH", temp_str, MAX_STR_SIZE);
	labels->preference.scale_and_move_with_touch = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "PREFERENCE", "DRAW_WITH_TOUCH", temp_str, MAX_STR_SIZE);
	labels->preference.draw_with_touch = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
#endif

	// ブラシのデフォルト名
	for(i=0; i<NUM_BRUSH_TYPE; i++)
	{
		switch(i)
		{
		case BRUSH_TYPE_PENCIL:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "PENCIL", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_HARD_PEN:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "HARD_PEN", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_AIR_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "AIR_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_BLEND_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "BLEND_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_OLD_AIR_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "OLD_AIR_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_WATER_COLOR_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "WATER_COLOR_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_PICKER_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "PICKER_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_ERASER:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "ERASER", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_BUCKET:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "BUCKET", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_PATTERN_FILL:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "PATTERN_FILL", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_BLUR:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "BLUR", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_SMUDGE:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "SMUDGE", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_MIX_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "MIX_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_GRADATION:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "GRADATION", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_TEXT:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "TEXT", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_STAMP_TOOL:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "STAMP_TOOL", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_IMAGE_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "IMAGE_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_BLEND_IMAGE_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "BLEND_IMAGE_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_PICKER_IMAGE_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "PICKER_IMAGE_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_SCRIPT_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "SCRIPT_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_CUSTOM_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "CUSTOM_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_PLUG_IN:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "PLUG_IN", temp_str, MAX_STR_SIZE);
			break;
		}
		labels->tool_box.brush_default_names[i] =
			g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	}
	length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "NORMAL_BRUSH", temp_str, MAX_STR_SIZE);
	labels->tool_box.normal_brush = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	LoadFractalLabels(fractal_labels, labels, file, lang);

	file->delete_func(file);

	g_object_unref(fp);
	g_object_unref(stream);
	g_object_unref(file_info);
#undef MAX_STR_SIZE
}

void LoadFractalLabels(
	FRACTAL_LABEL* labels,
	APPLICATION_LABELS* app_label,
	INI_FILE* file,
	const char* code
)
{
#define MAX_STR_SIZE 512
	char temp_str[MAX_STR_SIZE];
	size_t length;

	length = IniFileGetString(file, "FRACTAL", "TRIANGLE", temp_str, MAX_STR_SIZE);
	labels->triangle = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "TRANSFORM", temp_str, MAX_STR_SIZE);
	labels->transform = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "VARIATIONS", temp_str, MAX_STR_SIZE);
	labels->variations = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "COLORS", temp_str, MAX_STR_SIZE);
	labels->colors = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "RANDOM", temp_str, MAX_STR_SIZE);
	labels->random = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "WEIGHT", temp_str, MAX_STR_SIZE);
	labels->weight = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "SYMMETRY", temp_str, MAX_STR_SIZE);
	labels->symmetry = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "LINEAR", temp_str, MAX_STR_SIZE);
	labels->linear = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "SINUSOIDAL", temp_str, MAX_STR_SIZE);
	labels->sinusoidal = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "TRIANGLE", temp_str, MAX_STR_SIZE);
	labels->triangle = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "SPHERICAL", temp_str, MAX_STR_SIZE);
	labels->spherical = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "SWIRL", temp_str, MAX_STR_SIZE);
	labels->swirl = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "HORSESHOE", temp_str, MAX_STR_SIZE);
	labels->horseshoe = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "POLAR", temp_str, MAX_STR_SIZE);
	labels->polar = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "HANDKERCHIEF", temp_str, MAX_STR_SIZE);
	labels->handkerchief = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "HEART", temp_str, MAX_STR_SIZE);
	labels->heart = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "DISC", temp_str, MAX_STR_SIZE);
	labels->disc = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "SPIRAL", temp_str, MAX_STR_SIZE);
	labels->spiral = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "HYPERBOLIC", temp_str, MAX_STR_SIZE);
	labels->hyperbolic = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "DIAMOND", temp_str, MAX_STR_SIZE);
	labels->diamond = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "EX", temp_str, MAX_STR_SIZE);
	labels->ex = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "JULIA", temp_str, MAX_STR_SIZE);
	labels->julia = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "BENT", temp_str, MAX_STR_SIZE);
	labels->bent = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "WAVES", temp_str, MAX_STR_SIZE);
	labels->waves = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "FISHEVE", temp_str, MAX_STR_SIZE);
	labels->fisheye = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "POPCORN", temp_str, MAX_STR_SIZE);
	labels->popcorn = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	length = IniFileGetString(file, "FRACTAL", "PRESERVE_WEIGHT", temp_str, MAX_STR_SIZE);
	labels->preserve_weight = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
	labels->update = app_label->tool_box.update;
	labels->update_immediately = app_label->tool_box.update_immediately;
	length = IniFileGetString(file, "FRACTAL", "ADJUST", temp_str, MAX_STR_SIZE);
	labels->adjust = g_convert(temp_str, length, "UTF-8", code, NULL, NULL, NULL);
#undef MAX_STR_SIZE
}

void Load3dModelingLabels(APPLICATION* app, const char* lang_file_path)
{
#define MAX_STR_SIZE 512
	// 初期化ファイル解析用
	INI_FILE_PTR file;
	// ファイル読み込みストリーム
	GFile* fp = g_file_new_for_path(lang_file_path);
	GFileInputStream* stream = g_file_read(fp, NULL, NULL);
	// ファイルサイズ取得用
	GFileInfo *file_info;
	// 設定するラベルデータ
	UI_LABEL *labels;
	// ファイルサイズ
	size_t data_size;
	char temp_str[MAX_STR_SIZE], lang[MAX_STR_SIZE];
	size_t length;

	// ファイルオープンに失敗したら終了
	if(stream == NULL)
	{
		g_object_unref(fp);
		return;
	}

	// ファイルサイズを取得
	file_info = g_file_input_stream_query_info(stream,
		G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, NULL);
	data_size = (size_t)g_file_info_get_size(file_info);
	
	file = CreateIniFile(stream,
		(size_t (*)(void*, size_t, size_t, void*))FileRead, data_size, INI_READ);

	// 文字コードを取得
	(void)IniFileGetString(file, "CODE", "CODE_TYPE", lang, MAX_STR_SIZE);

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	// 3Dモデル用のラベルデータを取得
	labels = GetUILabel(app->modeling);
#endif

	labels->menu.file = app->labels->menu.file;
	length = IniFileGetString(file, "3D_MODELING", "LOAD_PROJECT", temp_str, MAX_STR_SIZE);
	labels->menu.load_project = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "SAVE_PROJECT", temp_str, MAX_STR_SIZE);
	labels->menu.save_project = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "SAVE_PROJECT_AS", temp_str, MAX_STR_SIZE);
	labels->menu.save_project_as = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "ADD_MODEL_ACCESSORY", temp_str, MAX_STR_SIZE);
	labels->menu.add_model_accessory = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	labels->menu.edit = app->labels->menu.edit;
	labels->menu.undo = app->labels->menu.undo;
	labels->menu.redo = app->labels->menu.redo;

	length = IniFileGetString(file, "3D_MODELING", "ENVIRONMENT", temp_str, MAX_STR_SIZE);
	labels->control.environment = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "CAMERA", temp_str, MAX_STR_SIZE);
	labels->control.camera = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "LIGHT", temp_str, MAX_STR_SIZE);
	labels->control.light = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "POSITION", temp_str, MAX_STR_SIZE);
	labels->control.position = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "ROTATION", temp_str, MAX_STR_SIZE);
	labels->control.rotation = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "MODEL", temp_str, MAX_STR_SIZE);
	labels->control.model = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "BONE", temp_str, MAX_STR_SIZE);
	labels->control.bone = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "MORPH", temp_str, MAX_STR_SIZE);
	labels->control.morph = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "LOAD_MODEL", temp_str, MAX_STR_SIZE);
	labels->control.load_model = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "LOAD_POSE", temp_str, MAX_STR_SIZE);
	labels->control.load_pose = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "CONTROL_MODEL", temp_str, MAX_STR_SIZE);
	labels->control.control_model = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "NO_SELECT", temp_str, MAX_STR_SIZE);
	labels->control.no_select = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "RESET", temp_str, MAX_STR_SIZE);
	labels->control.reset = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "APPLY_CENTER_POSITION", temp_str, MAX_STR_SIZE);
	labels->control.apply_center_position = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	labels->control.scale = app->labels->tool_box.scale;
	labels->control.width = app->labels->make_new.width;
	labels->control.height = app->labels->make_new.height;
	length = IniFileGetString(file, "3D_MODELING", "DEPTH", temp_str, MAX_STR_SIZE);
	labels->control.depth = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "COLOR", temp_str, MAX_STR_SIZE);
	labels->control.color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	labels->control.opacity = app->labels->layer_window.opacity;
	length = IniFileGetString(file, "3D_MODELING", "WEIGHT", temp_str, MAX_STR_SIZE);
	labels->control.weight = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "EDGE_SIZE", temp_str, MAX_STR_SIZE);
	labels->control.edge_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "NUM_LINES", temp_str, MAX_STR_SIZE);
	labels->control.num_lines = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	labels->control.line_width = app->labels->tool_box.line_width;
	length = IniFileGetString(file, "3D_MODELING", "DISTANCE", temp_str, MAX_STR_SIZE);
	labels->control.distance = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "FIELD_OF_VIEW", temp_str, MAX_STR_SIZE);
	labels->control.field_of_view = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "MODEL_CONNECT_TO", temp_str, MAX_STR_SIZE);
	labels->control.model_connect_to = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "ENABLE_PHYSICS", temp_str, MAX_STR_SIZE);
	labels->control.enable_physics = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "DISPLAY_GRID", temp_str, MAX_STR_SIZE);
	labels->control.display_grid = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "REMOVE_CURRENT_MODEL", temp_str, MAX_STR_SIZE);
	labels->control.delete_current_model = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "RENDER_SHADOW", temp_str, MAX_STR_SIZE);
	labels->control.render_shadow = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "RENDER_EDGE_ONLY", temp_str, MAX_STR_SIZE);
	labels->control.render_edge_only = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	labels->control.edit_mode.select = app->labels->menu.select;
	length = IniFileGetString(file, "3D_MODELING", "MOVE", temp_str, MAX_STR_SIZE);
	labels->control.edit_mode.move = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	labels->control.edit_mode.rotate = labels->control.rotation;
	length = IniFileGetString(file, "3D_MODELING", "EYE", temp_str, MAX_STR_SIZE);
	labels->control.morph_group.eye = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "LIP", temp_str, MAX_STR_SIZE);
	labels->control.morph_group.lip = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "EYEBLOW", temp_str, MAX_STR_SIZE);
	labels->control.morph_group.eye_blow = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "OTHER", temp_str, MAX_STR_SIZE);
	labels->control.morph_group.other = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "UPPER_SURFACE_SIZE", temp_str, MAX_STR_SIZE);
	labels->control.upper_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "LOWER_SURFACE_SIZE", temp_str, MAX_STR_SIZE);
	labels->control.lower_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "CUBOID", temp_str, MAX_STR_SIZE);
	labels->control.cuboid = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "ADD_CUBOID", temp_str, MAX_STR_SIZE);
	labels->control.add_cuboid = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "CONE", temp_str, MAX_STR_SIZE);
	labels->control.cone = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "ADD_CONE", temp_str, MAX_STR_SIZE);
	labels->control.add_cone = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	labels->control.back_ground_color = app->labels->unit.bg;
	labels->control.line_color = app->labels->tool_box.line_color;
	length = IniFileGetString(file, "3D_MODELING", "SURFACE_COLOR", temp_str, MAX_STR_SIZE);
	labels->control.surface_color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

	file->delete_func(file);
#undef MAX_STR_SIZE
}

ght_hash_table_t* LabelStringTableNew(APPLICATION* app)
{
#define ADD_STR(TABLE, STR, KEY) {const char key[] = (KEY); (void)ght_insert((TABLE), (STR), sizeof(key)-1, key);}
#define TABLE_SIZE 1024
	ght_hash_table_t *table = ght_create(TABLE_SIZE);

	ght_set_hash(table, (ght_fn_hash_t)GetStringHashIgnoreCase);

	ADD_STR(table, app->labels->window.ok, "OK");
	ADD_STR(table, app->labels->window.apply, "Apply");
	ADD_STR(table, app->labels->window.cancel, "Cancel");
	ADD_STR(table, app->labels->window.normal, "Normal");
	ADD_STR(table, app->labels->window.reverse, "Reverse");
	ADD_STR(table, app->labels->window.edit_selection, "Edit Selection");
	ADD_STR(table, app->labels->window.window, "Window");
	ADD_STR(table, app->labels->window.close_window, "Close Window");
	ADD_STR(table, app->labels->window.place_left, "Dock Left");
	ADD_STR(table, app->labels->window.place_right, "Dock Right");
	ADD_STR(table, app->labels->window.fullscreen, "Full Screen");
	ADD_STR(table, app->labels->window.reference, "Reference Image Window");
	ADD_STR(table, app->labels->window.move_top_left, "Move Top Left");
	ADD_STR(table, app->labels->window.hot_key, "Hot Key");
	ADD_STR(table, app->labels->window.loading, "Loading...");
	ADD_STR(table, app->labels->window.saving, "Saving...");

	ADD_STR(table, app->labels->unit.pixel, "pixel");
	ADD_STR(table, app->labels->unit.length, "Length");
	ADD_STR(table, app->labels->unit.angle, "Angle");
	ADD_STR(table, app->labels->unit.pixel, "Pixel");
	ADD_STR(table, app->labels->unit.bg, "BG");
	ADD_STR(table, app->labels->unit.repeat, "Loop");
	ADD_STR(table, app->labels->unit.preview, "Preview");
	ADD_STR(table, app->labels->unit.interval, "Interval");
	ADD_STR(table, app->labels->unit.minute, "minute");
	ADD_STR(table, app->labels->unit.detail, "Detail");
	ADD_STR(table, app->labels->unit.target, "Target");
	ADD_STR(table, app->labels->unit.clip_board, "Clip Board");
	ADD_STR(table, app->labels->unit.name, "Name");
	ADD_STR(table, app->labels->unit.type, "Type");
	ADD_STR(table, app->labels->unit.resolution, "Resolution");
	ADD_STR(table, app->labels->unit.center, "Center");
	ADD_STR(table, app->labels->unit.straight, "Straight");
	ADD_STR(table, app->labels->unit.extend, "Grow");
	ADD_STR(table, app->labels->unit.mode, "Mode");
	ADD_STR(table, app->labels->unit.red, "Red");
	ADD_STR(table, app->labels->unit.green, "Green");
	ADD_STR(table, app->labels->unit.blue, "Blue");
	ADD_STR(table, app->labels->unit.cyan, "Cyan");
	ADD_STR(table, app->labels->unit.magenta, "Magenta");
	ADD_STR(table, app->labels->unit.yellow, "Yellow");
	ADD_STR(table, app->labels->unit.key_plate, "Key Plate");
	ADD_STR(table, app->labels->unit.add, "Add");
	ADD_STR(table, app->labels->unit._delete, "Delete");

	ADD_STR(table, app->labels->menu.file, "File");
	ADD_STR(table, app->labels->menu.make_new, "New");
	ADD_STR(table, app->labels->menu.open, "Open");
	ADD_STR(table, app->labels->menu.open_as_layer, "Open As Layer");
	ADD_STR(table, app->labels->menu.save, "Save");
	ADD_STR(table, app->labels->menu.save_as, "Save as");
	ADD_STR(table, app->labels->menu.close, "Close");
	ADD_STR(table, app->labels->menu.quit, "Quit");
	ADD_STR(table, app->labels->menu.edit, "Edit");
	ADD_STR(table, app->labels->menu.undo, "Undo");
	ADD_STR(table, app->labels->menu.redo, "Redo");
	ADD_STR(table, app->labels->menu.copy, "Copy");
	ADD_STR(table, app->labels->menu.copy_visible, "Copy Visible");
	ADD_STR(table, app->labels->menu.cut, "Cut");
	ADD_STR(table, app->labels->menu.paste, "Paste");
	ADD_STR(table, app->labels->menu.clip_board, "Clipboard");
	ADD_STR(table, app->labels->menu.transform, "Transform");
	ADD_STR(table, app->labels->menu.projection, "Projection");
	ADD_STR(table, app->labels->menu.canvas, "Canvas");
	ADD_STR(table, app->labels->menu.change_resolution, "Change Resolution");
	ADD_STR(table, app->labels->menu.change_canvas_size, "Change Canvas Size");
	ADD_STR(table, app->labels->menu.flip_canvas_horizontally, "Flip Canvas Horizontally");
	ADD_STR(table, app->labels->menu.flip_canvas_vertically, "Flip Canvas Vertically");
	ADD_STR(table, app->labels->menu.switch_bg_color, "Switch BG Color");
	ADD_STR(table, app->labels->menu.change_2nd_bg_color, "Change 2nd BG Color");
	ADD_STR(table, app->labels->menu.canvas_icc, "Change ICC Profile");
	ADD_STR(table, app->labels->menu.layer, "Layer");
	ADD_STR(table, app->labels->menu.new_color, "New Layer(Color)");
	ADD_STR(table, app->labels->menu.new_vector, "New Layer(Vector)");
	ADD_STR(table, app->labels->menu.new_layer_set, "New Layer Set");
	ADD_STR(table, app->labels->menu.new_3d_modeling, "New 3D Modeling Layer");
	ADD_STR(table, app->labels->menu.copy_layer, "Copy Layer");
	ADD_STR(table, app->labels->menu.delete_layer, "Delete Layer");
	ADD_STR(table, app->labels->menu.fill_layer_fg_color, "Fill with FG Color");
	ADD_STR(table, app->labels->menu.fill_layer_pattern , "Fill with Pattern");
	ADD_STR(table, app->labels->menu.rasterize_layer, "Rasterize Layer");
	ADD_STR(table, app->labels->menu.merge_down_layer, "Merge Down");
	ADD_STR(table, app->labels->menu.merge_all_layer, "Flaten Image");
	ADD_STR(table, app->labels->menu.visible2layer, "Visible to Layer");
	ADD_STR(table, app->labels->menu.copy_visible, "Visible Copy");
	ADD_STR(table, app->labels->menu.select, "Select");
	ADD_STR(table, app->labels->menu.select_none, "Select None");
	ADD_STR(table, app->labels->menu.select_invert, "Select Invert");
	ADD_STR(table, app->labels->menu.select_all, "Select All");
	ADD_STR(table, app->labels->menu.selection_extend, "Select Grow");
	ADD_STR(table, app->labels->menu.selection_reduct, "Select Shrink");
	ADD_STR(table, app->labels->menu.view, "View");
	ADD_STR(table, app->labels->menu.zoom, "Zoom");
	ADD_STR(table, app->labels->menu.zoom_in, "Zoom In");
	ADD_STR(table, app->labels->menu.zoom_out, "Zoom Out");
	ADD_STR(table, app->labels->menu.zoom_reset, "Actual Size");
	ADD_STR(table, app->labels->menu.reverse_horizontally, "Reverse Horizontally");
	ADD_STR(table, app->labels->menu.rotate, "Rotate");
	ADD_STR(table, app->labels->menu.reset_rotate, "Reset Rotate");
	ADD_STR(table, app->labels->menu.display_filter, "Display Filters");
	ADD_STR(table, app->labels->menu.no_filter, "Nothing");
	ADD_STR(table, app->labels->menu.gray_scale, "Gray Scale");
	ADD_STR(table, app->labels->menu.gray_scale_yiq, "Gray Scale(YIQ)");
	ADD_STR(table, app->labels->menu.filters, "Filters");
	ADD_STR(table, app->labels->menu.blur, "Blur Filter");
	ADD_STR(table, app->labels->menu.motion_blur, "Motion Blur");
	ADD_STR(table, app->labels->menu.gaussian_blur, "Gaussian Blur");
	ADD_STR(table, app->labels->menu.bright_contrast, "Brightness & Contrast");
	ADD_STR(table, app->labels->menu.hue_saturtion, "Hue & Saturation");
	ADD_STR(table, app->labels->menu.color_levels, "Color Levels");
	ADD_STR(table, app->labels->menu.tone_curve, "Tone Curve");
	ADD_STR(table, app->labels->menu.luminosity2opacity, "Luminosity to Opacity");
	ADD_STR(table, app->labels->menu.color2alpha, "Color to Alpha");
	ADD_STR(table, app->labels->menu.colorize_with_under, "Colorize with Under Layer");
	ADD_STR(table, app->labels->menu.gradation_map, "Gradation Map");
	ADD_STR(table, app->labels->menu.detect_max, "Map with Detect Max Black Value");
	ADD_STR(table, app->labels->menu.tranparancy_as_white, "Transparency as White");
	ADD_STR(table, app->labels->menu.fill_with_vector, "Fill with Vector");
	ADD_STR(table, app->labels->menu.render, "Render");
	ADD_STR(table, app->labels->menu.cloud, "Cloud");
	ADD_STR(table, app->labels->menu.fractal, "Fractal");
	ADD_STR(table, app->labels->menu.plug_in, "Plug-in");
	ADD_STR(table, app->labels->menu.script, "Script");
	ADD_STR(table, app->labels->menu.help, "Help");
	ADD_STR(table, app->labels->menu.version, "Version");

	ADD_STR(table, app->labels->make_new.title, "Make New Title");
	ADD_STR(table, app->labels->make_new.name, "New Canvas");
	ADD_STR(table, app->labels->make_new.width, "Width");
	ADD_STR(table, app->labels->make_new.height, "Height");
	ADD_STR(table, app->labels->make_new.second_bg_color, "2nd BG Color");
	ADD_STR(table, app->labels->make_new.adopt_icc_profile, "Adopt ICC Profile?");

	ADD_STR(table, app->labels->tool_box.title, "Tool Box");
	ADD_STR(table, app->labels->tool_box.initialize, "Initialize");
	ADD_STR(table, app->labels->tool_box.new_brush, "New Brush");
	ADD_STR(table, app->labels->tool_box.smooth, "Smooth");
	ADD_STR(table, app->labels->tool_box.smooth_quality, "Quality");
	ADD_STR(table, app->labels->tool_box.smooth_rate, "Smooth Rate");
	ADD_STR(table, app->labels->tool_box.smooth_gaussian, "Gaussian");
	ADD_STR(table, app->labels->tool_box.smooth_average, "Average");
	ADD_STR(table, app->labels->tool_box.base_scale, "Magnification");
	ADD_STR(table, app->labels->tool_box.brush_scale, "Brush Size");
	ADD_STR(table, app->labels->tool_box.scale, "Scale");
	ADD_STR(table, app->labels->tool_box.flow, "Flow");
	ADD_STR(table, app->labels->tool_box.pressure, "Pressure");
	ADD_STR(table, app->labels->tool_box.extend, "Extend Range");
	ADD_STR(table, app->labels->tool_box.blur, "Blur");
	ADD_STR(table, app->labels->tool_box.outline_hardness, "Outline Hardness");
	ADD_STR(table, app->labels->tool_box.color_extend, "Color Extends");
	ADD_STR(table, app->labels->tool_box.select_move_start, "Start Distance");
	ADD_STR(table, app->labels->tool_box.anti_alias, "Anti Alias");
	ADD_STR(table, app->labels->tool_box.change_text_color, "Change Text Color");
	ADD_STR(table, app->labels->tool_box.text_horizonal, "Horzonal");
	ADD_STR(table, app->labels->tool_box.text_vertical, "Vertical");
	ADD_STR(table, app->labels->tool_box.text_style, "Style");
	ADD_STR(table, app->labels->tool_box.text_normal, "Text Normal");
	ADD_STR(table, app->labels->tool_box.text_bold, "Bold");
	ADD_STR(table, app->labels->tool_box.text_italic, "Italic");
	ADD_STR(table, app->labels->tool_box.text_oblique, "Oblique");
	ADD_STR(table, app->labels->tool_box.has_balloon, "Balloon");
	ADD_STR(table, app->labels->tool_box.balloon_has_edge, "Balloon Has Edge");
	ADD_STR(table, app->labels->tool_box.line_color, "Line Color");
	ADD_STR(table, app->labels->tool_box.fill_color, "Fill Color");
	ADD_STR(table, app->labels->tool_box.line_width, "Line Width");
	ADD_STR(table, app->labels->tool_box.change_line_width, "Change Line Width");
	ADD_STR(table, app->labels->tool_box.manually_set, "Manually Set");
	ADD_STR(table, app->labels->tool_box.aspect_ratio, "Aspect Ratio");
	ADD_STR(table, app->labels->tool_box.centering_horizontally, "Centering Horizontally");
	ADD_STR(table, app->labels->tool_box.centering_vertically, "Centering Vertically");
	ADD_STR(table, app->labels->tool_box.adjust_range_to_text, "Adjust Range to Text");
	ADD_STR(table, app->labels->tool_box.num_edge, "Number of Edge");
	ADD_STR(table, app->labels->tool_box.edge_size, "Edge Size");
	ADD_STR(table, app->labels->tool_box.random_edge_size, "Edge Size Random");
	ADD_STR(table, app->labels->tool_box.random_edge_distance, "Edge Distance Random");
	ADD_STR(table, app->labels->tool_box.num_children, "Number of Children");
	ADD_STR(table, app->labels->tool_box.start_child_size, "Start Child Size");
	ADD_STR(table, app->labels->tool_box.end_child_size, "End Child Size");
	ADD_STR(table, app->labels->tool_box.reverse, "Reverse");
	ADD_STR(table, app->labels->tool_box.reverse_horizontally, "Reverse Horizontally");
	ADD_STR(table, app->labels->tool_box.reverse_vertically, "Reverse Vertically");
	ADD_STR(table, app->labels->tool_box.blend_mode, "Blend Mode");
	ADD_STR(table, app->labels->tool_box.hue, "Hue");
	ADD_STR(table, app->labels->tool_box.saturation, "Saturation");
	ADD_STR(table, app->labels->tool_box.brightness, "Brightness");
	ADD_STR(table, app->labels->tool_box.contrast, "Contrast");
	ADD_STR(table, app->labels->tool_box.distance, "Distance");
	ADD_STR(table, app->labels->tool_box.rotate_start, "Rotate Start");
	ADD_STR(table, app->labels->tool_box.rotate_speed, "Rotate Speed");
	ADD_STR(table, app->labels->tool_box.random_rotate, "Random Rotate");
	ADD_STR(table, app->labels->tool_box.rotate_to_brush_direction, "Rotate to Brush Direction");
	ADD_STR(table, app->labels->tool_box.size_range, "Size Range");
	ADD_STR(table, app->labels->tool_box.rotate_range, "Rotate Range");
	ADD_STR(table, app->labels->tool_box.random_size, "Random Size");
	ADD_STR(table, app->labels->tool_box.clockwise, "Clockwise");
	ADD_STR(table, app->labels->tool_box.counter_clockwise, "Counter Clockwise");
	ADD_STR(table, app->labels->tool_box.both_direction, "Both Direction");
	ADD_STR(table, app->labels->tool_box.min_degree, "Minimum Degree");
	ADD_STR(table, app->labels->tool_box.min_distance, "Minimum Distance");
	ADD_STR(table, app->labels->tool_box.min_pressure, "Minimum Pressure");
	ADD_STR(table, app->labels->tool_box.enter, "Enter");
	ADD_STR(table, app->labels->tool_box.out, "Out");
	ADD_STR(table, app->labels->tool_box.enter_out, "Enter & Out");
	ADD_STR(table, app->labels->tool_box.mix, "Mix");
	ADD_STR(table, app->labels->tool_box.gradation_reverse, "Reverse FG BG");
	ADD_STR(table, app->labels->tool_box.devide_stroke, "Devide Stroke");
	ADD_STR(table, app->labels->tool_box.delete_stroke, "Delete Stroke");
	ADD_STR(table, app->labels->tool_box.target, "Vector Target");
	ADD_STR(table, app->labels->tool_box.stroke, "Stroke");
	ADD_STR(table, app->labels->tool_box.prior_angle, "Prior Angle");
	ADD_STR(table, app->labels->tool_box.control_point, "Control Point");
	ADD_STR(table, app->labels->tool_box.transform_free, "Transform Free");
	ADD_STR(table, app->labels->tool_box.transform_scale, "Transform Scale");
	ADD_STR(table, app->labels->tool_box.transform_free_shape, "Transform Free Shape");
	ADD_STR(table, app->labels->tool_box.transform_rotate, "Transform Rotate");
	ADD_STR(table, app->labels->tool_box.preference, "Preference");
	ADD_STR(table, app->labels->tool_box.name, "Brush Name");
	ADD_STR(table, app->labels->tool_box.copy_brush, "Copy Brush");
	ADD_STR(table, app->labels->tool_box.change_brush, "Change Brush");
	ADD_STR(table, app->labels->tool_box.delete_brush, "Delete Brush");
	ADD_STR(table, app->labels->tool_box.texture, "Texture");
	ADD_STR(table, app->labels->tool_box.texture_strength, "Strength");
	ADD_STR(table, app->labels->tool_box.no_texture, "No Texture");
	ADD_STR(table, app->labels->tool_box.pallete_add, "Add Color");
	ADD_STR(table, app->labels->tool_box.pallete_delete, "Delete Color");
	ADD_STR(table, app->labels->tool_box.load_pallete, "Load Pallete");
	ADD_STR(table, app->labels->tool_box.load_pallete_after, "Add Pallete");
	ADD_STR(table, app->labels->tool_box.write_pallete, "Write Pallete");
	ADD_STR(table, app->labels->tool_box.clear_pallete, "Clear Pallete");
	ADD_STR(table, app->labels->tool_box.pick_mode, "Pick Mode");
	ADD_STR(table, app->labels->tool_box.single_pixels, "Single Pixel");
	ADD_STR(table, app->labels->tool_box.average_color, "Average Color");
	ADD_STR(table, app->labels->tool_box.open_path, "Open Path");
	ADD_STR(table, app->labels->tool_box.close_path, "Close Path");
	ADD_STR(table, app->labels->tool_box.update, "Update");
	ADD_STR(table, app->labels->tool_box.frequency, "Frequency");
	ADD_STR(table, app->labels->tool_box.cloud_color, "Cloud Color");
	ADD_STR(table, app->labels->tool_box.persistence, "Persistence");
	ADD_STR(table, app->labels->tool_box.rand_seed, "Random Seed");
	ADD_STR(table, app->labels->tool_box.use_random, "Use Random");
	ADD_STR(table, app->labels->tool_box.update_immediately, "Update Immediately");
	ADD_STR(table, app->labels->tool_box.num_octaves, "Number of Octaves");
	ADD_STR(table, app->labels->tool_box.linear, "Linear");
	ADD_STR(table, app->labels->tool_box.cosine, "Cosine");
	ADD_STR(table, app->labels->tool_box.cubic, "Cubic");
	ADD_STR(table, app->labels->tool_box.colorize, "Colorize");
	ADD_STR(table, app->labels->tool_box.start_edit_3d, "Start Editting 3D Model");
	ADD_STR(table, app->labels->tool_box.end_edit_3d, "Finish Editting 3D Model");
	ADD_STR(table, app->labels->tool_box.scatter, "Scatter");
	ADD_STR(table, app->labels->tool_box.scatter_size, "Scatter Size");
	ADD_STR(table, app->labels->tool_box.scatter_range, "Scatter Range");
	ADD_STR(table, app->labels->tool_box.scatter_random_size, "Random Size Scatter");
	ADD_STR(table, app->labels->tool_box.scatter_random_flow, "Random Flow Scatter");
	ADD_STR(table, app->labels->tool_box.normal_brush, "Normal Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_PENCIL], "Pencil");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_HARD_PEN], "Hard Pen");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_AIR_BRUSH], "Air Brush");
	//ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_OLD_AIR_BRUSH], "Old Air Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_WATER_COLOR_BRUSH], "Water Color Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_PICKER_BRUSH], "Picker Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_ERASER], "Eraser");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_BUCKET], "Bucket");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_PATTERN_FILL], "Pattern Fill");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_BLUR], "Blur Tool");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_SMUDGE], "Smudge");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_MIX_BRUSH], "Mix Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_GRADATION], "Gradation");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_TEXT], "Text Tool");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_STAMP_TOOL], "Stamp Tool");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_IMAGE_BRUSH], "Image Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_BLEND_IMAGE_BRUSH], "Image Blend Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_PICKER_IMAGE_BRUSH], "Picker Image Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_SCRIPT_BRUSH], "Script Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_CUSTOM_BRUSH], "Custom Brush");
	ADD_STR(table, app->labels->tool_box.brush_default_names[BRUSH_TYPE_PLUG_IN], "PLUG_IN");
	ADD_STR(table, app->labels->tool_box.select.target, "Detection Target");
	ADD_STR(table, app->labels->tool_box.select.area, "Detect from ...");
	ADD_STR(table, app->labels->tool_box.select.rgb, "Pixels Color");
	ADD_STR(table, app->labels->tool_box.select.rgba, "Pixel Color + Alpha");
	ADD_STR(table, app->labels->tool_box.select.alpha, "Alpha");
	ADD_STR(table, app->labels->tool_box.select.active_layer, "Active Layer");
	ADD_STR(table, app->labels->tool_box.select.under_layer, "Under Layer");
	ADD_STR(table, app->labels->tool_box.select.canvas, "Canvas");
	ADD_STR(table, app->labels->tool_box.select.threshold, "Threshold");
	ADD_STR(table, app->labels->tool_box.select.detect_area, "Detection Area");
	ADD_STR(table, app->labels->tool_box.select.area_large, "Large");
	ADD_STR(table, app->labels->tool_box.control.select, "Select/Release");
	ADD_STR(table, app->labels->tool_box.control.move, "Move Control Point");
	ADD_STR(table, app->labels->tool_box.control.change_pressure, "Change Pressure");
	ADD_STR(table, app->labels->tool_box.control.delete_point, "Delete Control Point");
	ADD_STR(table, app->labels->tool_box.control.move_stroke, "Move Stroke");
	ADD_STR(table, app->labels->tool_box.control.copy_stroke, "Copy & Move Stroke");
	ADD_STR(table, app->labels->tool_box.control.joint_stroke, "Joint Storke");
	ADD_STR(table, app->labels->tool_box.shape.circle, "Circle");
	ADD_STR(table, app->labels->tool_box.shape.eclipse, "Eclipse");
	ADD_STR(table, app->labels->tool_box.shape.triangle, "Triangle");
	ADD_STR(table, app->labels->tool_box.shape.square, "Square");
	ADD_STR(table, app->labels->tool_box.shape.rhombus, "Rhombus");
	ADD_STR(table, app->labels->tool_box.shape.hexagon, "Hexagon");
	ADD_STR(table, app->labels->tool_box.shape.star, "Star");
	ADD_STR(table, app->labels->tool_box.shape.pattern, "Pattern");
	ADD_STR(table, app->labels->tool_box.shape.image, "Image");

	//ADD_STR(table, app->labels->layer_window.new_layer, "Layer");
	ADD_STR(table, app->labels->layer_window.new_vector, "Vector");
	ADD_STR(table, app->labels->layer_window.new_layer_set, "Layer Set");
	ADD_STR(table, app->labels->layer_window.new_text, "Text");
	ADD_STR(table, app->labels->layer_window.new_3d_modeling, "3D Modeling");
	ADD_STR(table, app->labels->layer_window.new_adjsutment_layer, "Adjustment Layer");
	ADD_STR(table, app->labels->layer_window.rename, "Rename Layer");
	ADD_STR(table, app->labels->layer_window.reorder, "Reorder Layer");
	ADD_STR(table, app->labels->layer_window.alpha_to_select, "Opaciy to Selection Area");
	ADD_STR(table, app->labels->layer_window.alpha_add_select, "Opacity Add Selection Area");
	ADD_STR(table, app->labels->layer_window.pasted_layer, "Pasted Layer");
	ADD_STR(table, app->labels->layer_window.under_layer, "Under Layer");
	ADD_STR(table, app->labels->layer_window.mixed_under_layer, "Mixed Under Layer");
	ADD_STR(table, app->labels->layer_window.blend_mode, "Blend Mode");
	ADD_STR(table, app->labels->layer_window.opacity, "Opacity");
	ADD_STR(table, app->labels->layer_window.mask_with_under, "Making with Under Layer");
	ADD_STR(table, app->labels->layer_window.lock_opacity, "Lock Opacity");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_ADD], "Blend Add");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_MULTIPLY], "Mlutiply");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_SCREEN], "Screen");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_OVERLAY], "Overlay");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_LIGHTEN], "Lighten");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_DARKEN], "Darken");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_DODGE], "Dodge");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_BURN], "Burn");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_HARD_LIGHT], "Hard Light");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_SOFT_LIGHT], "Soft Light");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_DIFFERENCE], "Difference");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_EXCLUSION], "Exclusion");
	ADD_STR(table, app->labels->layer_window.blend_modes[LAYER_BLEND_BINALIZE], "Binalize");

	return table;
#undef TABLE_SIZE
}

/*******************************************************************
* GetLabelString関数                                               *
* 日本語環境なら日本語の英語環境なら英語のラベル用文字列を取得する *
* 引数                                                             *
* app	: アプリケーションを管理するデータ                         *
* key	: 基本的に英語環境時のラベル文字列                         *
* 返り値                                                           *
*	ラベル用文字列(free厳禁)                                       *
*******************************************************************/
char* GetLabelString(APPLICATION* app, const char* key)
{
	if(app->label_table == NULL)
	{
		if((app->label_table = LabelStringTableNew(app)) == NULL)
		{
			return NULL;
		}
	}

	return (char*)ght_get(app->label_table, (unsigned int)strlen(key), key);
}

#ifdef __cplusplus
}
#endif
