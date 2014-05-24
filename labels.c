#include "labels.h"
#include "ini_file.h"
#include "utils.h"
#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

void LoadLabels(APPLICATION_LABELS* labels, const char* lang_file_path)
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
	length = IniFileGetString(file, "WINDOW", "CLOSE", temp_str, MAX_STR_SIZE);
	labels->window.close = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
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
	labels->window.hot_ley = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
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
	length = IniFileGetString(file, "FILTERS", "STRAIGHT_RANDOM", temp_str, MAX_STR_SIZE);
	labels->filter.straight_random = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "BIDIRECTION", temp_str, MAX_STR_SIZE);
	labels->filter.bidirection = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "BRIGHTNESS_CONTRAST", temp_str, MAX_STR_SIZE);
	labels->menu.bright_contrast = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "FILTERS", "HUE_SATURATION", temp_str, MAX_STR_SIZE);
	labels->menu.hue_saturtion = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
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
	labels->tool_box.randoam_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
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
	length = IniFileGetString(file, "TOOL_BOX", "ENTER", temp_str, MAX_STR_SIZE);
	labels->tool_box.enter = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "OUT", temp_str, MAX_STR_SIZE);
	labels->tool_box.out = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
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
	length = IniFileGetString(file, "TOOL_BOX", "START_EDIT_3D_MODEL", temp_str, MAX_STR_SIZE);
	labels->tool_box.start_edit_3d = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "TOOL_BOX", "END_EDIT_3D_MODEL", temp_str, MAX_STR_SIZE);
	labels->tool_box.end_edit_3d = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);

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
	length = IniFileGetString(file, "LAYER_WINDOW", "ADD_NORMAL_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.add_normal = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "ADD_VECTOR_LAYER", temp_str, MAX_STR_SIZE);
	labels->layer_window.add_vector = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "ADD_LAYER_SET", temp_str, MAX_STR_SIZE);
	labels->layer_window.add_layer_set = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "LAYER_WINDOW", "ADD_3D_MODELING", temp_str, MAX_STR_SIZE);
	labels->layer_window.add_3d_modeling = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
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
#if MAJOR_VERSION > 1
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
		case BRUSH_TYPE_PICKER_IMAGE_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "PICKER_IMAGE_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		case BRUSH_TYPE_SCRIPT_BRUSH:
			length = IniFileGetString(file, "BRUSH_DEFAULT_NAME", "SCRIPT_BRUSH", temp_str, MAX_STR_SIZE);
			break;
		}
		labels->tool_box.brush_default_names[i] =
			g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	}

	file->delete_func(file);

	g_object_unref(fp);
	g_object_unref(stream);
	g_object_unref(file_info);
#undef MAX_STR_SIZE
}

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
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

	// 3Dモデル用のラベルデータを取得
	labels = GetUILabel(app->modeling);

	labels->menu.file = app->labels->menu.file;
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
	length = IniFileGetString(file, "3D_MODELING", "COLOR", temp_str, MAX_STR_SIZE);
	labels->control.color = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	labels->control.opacity = app->labels->layer_window.opacity;
	length = IniFileGetString(file, "3D_MODELING", "WEIGHT", temp_str, MAX_STR_SIZE);
	labels->control.weight = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
	length = IniFileGetString(file, "3D_MODELING", "EDGE_SIZE", temp_str, MAX_STR_SIZE);
	labels->control.edge_size = g_convert(temp_str, length, "UTF-8", lang, NULL, NULL, NULL);
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

	file->delete_func(file);
#undef MAX_STR_SIZE
}
#endif

#ifdef __cplusplus
}
#endif
