// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <errno.h>
#include "application.h"
#include "menu.h"
#include "utils.h"
#include "memory.h"

// ドラッグ&ドロップの判定用
#define DROP_URI 10

#define INITIALIZE_FILE_PATH "./application.ini"

/*
* OnDeleteMainWindow関数
* メインウィンドウが閉じられる前に呼び出される
* 引数
* window		: メインウィンドウウィジェット
* event_info	: ウィンドウを閉じるイベントの情報
* app			: アプリケーションを管理する構造体
* 返り値
*	そのままウィンドウを閉じる:FALSE	ウィンドウを閉じない:TRUE
*/
static gboolean OnDeleteMainWindow(GtkWidget* window, GdkEvent* event_info, APPLICATION* app)
{
	// 初期化データを書き出す
	WriteInitialize(app, INITIALIZE_FILE_PATH);

	// プログラムを完全終了
	gtk_main_quit();

	return FALSE;
}

/*
* DragDataRecieveCallBack関数
* ドラッグ&ドロップされた時に呼び出される
* 引数
* widget			: ドラッグ&ドロップされたウィジェット
* conext			: ドラッグ&ドロップ処理のコンテキスト
* x					: ドロップされた時のマウスのX座標
* y					: ドロップされた時のマウスのY座標
* selection_data	: ファイルパスが入っている
* target_type		: ドロップされたデータのタイプ
* time_stamp		: ドロップイベントの発生した時間
* app				: アプリケーションを管理する構造体
*/
static void DragDataRecieveCallBack(
	GtkWidget* widget,
	GdkDragContext* context,
	gint x,
	gint y,
	GtkSelectionData* selection_data,
	guint target_type,
	guint time_stamp,
	APPLICATION* app
)
{
	// ドロップされたデータの整合性を確認
	if((selection_data != NULL) && (gtk_selection_data_get_length(selection_data) >= 0))
	{	// ドロップされたデータはファイルパス
		if(target_type == DROP_URI)
		{
			// ディレクトリオープン用
			GDir *dir;
			// メタデータ取得用
			SOUND_PROPERTY *property_data;
			FILE *fp;
			// ドロップされたデータからURIの形式で取り出す
			gchar **uris;
			gchar **uri;
			// UTF-8のパス
			gchar *file_path;
			// システムのパス
			gchar *system_path;
			// '\'を'/'へ変換用
			gchar *str;

			// URI形式を一つ一つのURIに変換
			uris = g_uri_list_extract_uris(selection_data->data);
			// それぞれのURIをUTF-8→OSの文字コードに変換して
				// ファイルをオープン、メタデータを取得
			for(uri = uris; *uri != NULL; uri++)
			{
				file_path = g_filename_from_uri(*uri, NULL, NULL);
				// '\'を'/'へ変換
				str = file_path;
				while(*str != '\0')
				{
					if(*str == '\\')
					{
						*str = '/';
					}

					str = g_utf8_next_char(str);
				}

				// ディレクトリオープン
				dir = g_dir_open(file_path, 0, NULL);
				// オープンに失敗したらファイルオープンを試す
				if(dir == NULL)
				{
					// システムの文字コーへ変換
					system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);

					// ファイルパスを記憶
					MEM_FREE_FUNC(app->play_data.file_path);

					// ファイルをオープンして
					fp = fopen(system_path, "rb");
					if(fp != NULL)
					{	// オープン成功したらメタデータを取得
						property_data = GetSoundMetaData(
								(void*)fp, (size_t (*)(void*, size_t, size_t, void*))fread,
								(int (*)(void*, long, int))fseek, (long (*)(void*))ftell);
						if(property_data != NULL)
						{
							property_data->path = MEM_STRDUP_FUNC(system_path);
							AppendSoundPropertyData(app->widgets.list_view, property_data);
							ListAppendSoundData(app, property_data);
						}
						(void)fclose(fp);
					}

					g_free(system_path);
				}	// // オープンに失敗したらファイルオープンを試す
					// if(dir == NULL)
				else
				{	// 成功したのでディレクトリ内の全てのアイテムをチェック
					g_dir_close(dir);

					GetDirectoryContents(app, file_path);
				}

				g_free(file_path);
			}

			g_strfreev(uris);
		}

		// ドラッグ&ドロップのデータを削除
		gtk_drag_finish(context, TRUE, TRUE, time_stamp);
	}
}

/*
* PlayButtonClicked関数
* 再生ボタンがクリックされた時に呼び出される
* 引数
* button	: ボタンウィジェット
* app		: アプリケーションを管理する構造体
*/
static void PlayButtonClicked(GtkWidget* button, APPLICATION* app)
{
	// 無限ループ回避フラグをチェック
	if((app->flags & APPLICATION_FLAG_IN_PLAY_CONTROL) != 0)
	{
		return;
	}

	// 無限ループ回避フラグを立ててから処理開始
	app->flags |= APPLICATION_FLAG_IN_PLAY_CONTROL;

	// ボタンが押された状態になったら再生開始
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		app->flags &= ~(APPLICATION_FLAG_PAUSE);
	}

	// 処理が終了したので無限ループ回避フラグを降ろす
	app->flags &= ~(APPLICATION_FLAG_IN_PLAY_CONTROL);
}

/*
* PauseButtonClicked関数
* 一時停止ボタンがクリックされた時に呼び出される
* button	: ボタンウィジェット
* app		: アプリケーションを管理する構造体
*/
static void PauseButtonClicked(GtkWidget* button, APPLICATION* app)
{
	// 無限ループ回避フラグをチェック
	if((app->flags & APPLICATION_FLAG_IN_PLAY_CONTROL) != 0)
	{
		return;
	}

	// 無限ループ回避フラグを立ててから処理開始
	app->flags |= APPLICATION_FLAG_IN_PLAY_CONTROL;

	// ボタンが押された
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		// 何も再生していなければボタンの状態を
			// 変更する
		if(app->play_data.audio_player == NULL)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
		}
		else
		{	// 一時停止のフラグを立てる
			app->flags |= APPLICATION_FLAG_PAUSE;
			// 一時停止を実行
			alSourcePause(app->play_data.audio_player->source);
		}
	}
	else
	{	// ボタンの状態が戻った
		app->flags &= ~(APPLICATION_FLAG_PAUSE);
	}

	// 処理が終了したので無限ループ回避フラグを降ろす
	app->flags &= ~(APPLICATION_FLAG_IN_PLAY_CONTROL);
}

/*
* ChangePlayModeButtonClicked関数
* 再生モード設定用ボタンが押された時に呼び出される
* 引数
* button	: ボタンウィジェット
* app		: アプリケーションを管理する構造体
*/
static void ChangePlayModeButtonClicked(GtkWidget* button, APPLICATION* app)
{
	switch(app->play_data.play_mode)
	{
	case APPLICATION_PLAYER_STOP_AT_END:	// 1曲再生で終了
		app->play_data.play_mode = APPLICATION_PLAYER_ONE_LOOP;
		gtk_image_set_from_file(GTK_IMAGE(app->widgets.play_mode_image), "./image/loop_play.png");
		break;
	case APPLICATION_PLAYER_ONE_LOOP:	// 1曲ループ
		app->play_data.play_mode = APPLICATION_PLAYER_STOP_AT_LIST_END;
		gtk_image_set_from_file(GTK_IMAGE(app->widgets.play_mode_image), "./image/list_once.png");
		break;
	case APPLICATION_PLAYER_STOP_AT_LIST_END:	// リストを1周
		app->play_data.play_mode = APPLICATION_PLAYER_LIST_LOOP;
		gtk_image_set_from_file(GTK_IMAGE(app->widgets.play_mode_image), "./image/list_loop.png");
		break;
	case APPLICATION_PLAYER_LIST_LOOP:	// リストをループ
		app->play_data.play_mode = APPLICATION_PLAYER_STOP_AT_END;
		gtk_image_set_from_file(GTK_IMAGE(app->widgets.play_mode_image), "./image/play_once.png");
		break;
	}
}

/*
* VolumeChanged関数
* ボリュームコントロールが操作された時に呼び出される
* 引数
* control	: ボリュームコントロールウィジェット
* volume	: ボリュームの値
* app		: アプリケーションを管理する構造体
*/
static void VolumeControl(GtkWidget* control, gdouble volume, APPLICATION* app)
{
	app->play_data.volume = volume;
	if(app->play_data.audio_player != NULL)
	{
		alSourcef(app->play_data.audio_player->source, AL_GAIN, (ALfloat)volume);
	}
}

/*
* Initialize関数
* アプリケーションの初期化処理
* 引数
* app	: アプリケーションを管理する構造体のデータ
*/
void Initialize(APPLICATION* app)
{
#define BUTTON_IMAGE_SIZE 32
	// 初期化データ
	APPLICATION_INITIALIZE initialize = {0};
	// レイアウト用
	GtkWidget *vbox;
	GtkWidget *hbox;
	// 再生コントロール用のボタン
	GtkWidget *button;
	// ボタンに画像表示用
	GtkWidget *image;
	// 描画実行用
	uint8 *pixels;
	GdkPixbuf *pixbuf;
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	// ファイルドロップ対応用
	GtkTargetEntry target_list[] = {{"text/uri-list", 0, DROP_URI}};

	// 初期化データを読み込む
	ReadInitializeFile(app, &initialize, INITIALIZE_FILE_PATH);

	// ラベルデータを読み込む
	LoadLabel(app->label, "./lang/japanese.lang");

	// メインウィンドウを作成
	app->widgets.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	// ウィンドウの座標を設定
	gtk_window_move(GTK_WINDOW(app->widgets.window), initialize.window_x, initialize.window_y);
	// ウィンドウのサイズを設定
	gtk_widget_set_size_request(app->widgets.window, initialize.window_width, initialize.window_height);

	// メインウィンドウが閉じる時のイベントを記述
	(void)g_signal_connect(G_OBJECT(app->widgets.window), "delete-event",
		G_CALLBACK(OnDeleteMainWindow), app);

	// ファイルのドロップを設定
	gtk_drag_dest_set(
		app->widgets.window,
		GTK_DEST_DEFAULT_ALL,
		target_list,
		1,
		GDK_ACTION_COPY
	);
	gtk_drag_dest_add_uri_targets(app->widgets.window);
	(void)g_signal_connect(G_OBJECT(app->widgets.window), "drag-data-received",
		G_CALLBACK(DragDataRecieveCallBack), app);

	// 時限で呼び出される関数を設定
	app->widgets.time_oud_id = g_timeout_add_full(G_PRIORITY_LOW, 1000/60, (GSourceFunc)TimeOutCallBack,
		app, NULL);

	// コントロールを縦に並べるためのパッキングボックス
	vbox = gtk_vbox_new(FALSE, 0);
	// メインウィンドウ登録
	gtk_container_add(GTK_CONTAINER(app->widgets.window), vbox);

	// メニューバーを登録
	gtk_box_pack_start(GTK_BOX(vbox), MenuBarNew(app->label, app), FALSE, FALSE, 0);

	// ファイルリストビューを登録
	app->widgets.scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(app->widgets.scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	app->widgets.list_view = gtk_tree_view_new();
	// 表示を初期化
	ModelInitialize(app->widgets.list_view, app->label);
	// 表示を縞模様に
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(app->widgets.list_view), TRUE);
	// スクロール内にリストビューを表示
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(app->widgets.scrolled_window), app->widgets.list_view);
	// ダブルクリック時のコールバック関数を設定
	(void)g_signal_connect(G_OBJECT(app->widgets.list_view), "row-activated",
		G_CALLBACK(ListViewDoubleClicked), app);
	gtk_box_pack_start(GTK_BOX(vbox), app->widgets.scrolled_window, TRUE, TRUE, 0);

	// 再生、一時停止ボタンを作成
	hbox = gtk_hbox_new(FALSE, 3);
	// 描画用のピクセルデータを作成
	pixels = (uint8*)MEM_ALLOC_FUNC(BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	(void)memset(pixels, 0, BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	// イメージウィジェットにセット出来る形式に変換
	pixbuf = gdk_pixbuf_new_from_data(pixels, GDK_COLORSPACE_RGB, TRUE, 8, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE,
		BUTTON_IMAGE_SIZE * 4, NULL, NULL);
	// 描画ライブラリにピクセルデータを渡す
		// 描画イメージ作成
	surface_p = cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_ARGB32,
		BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE * 4);
	// 描画コンテキストを作成
	cairo_p = cairo_create(surface_p);
	// 再生の三角を作成
		// 色を指定
	cairo_set_source_rgb(cairo_p, 1, 0, 0);
	cairo_move_to(cairo_p, BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE * 0.5);
	cairo_fill(cairo_p);
	// イメージウィジェットを作成
	image = gtk_image_new_from_pixbuf(pixbuf);
	// ボタン作成
	app->widgets.play_button = gtk_toggle_button_new();
	// ボタンの中にイメージを入れる
	gtk_container_add(GTK_CONTAINER(app->widgets.play_button), image);
	(void)g_signal_connect(G_OBJECT(app->widgets.play_button), "toggled",
		G_CALLBACK(PlayButtonClicked), app);
	gtk_box_pack_start(GTK_BOX(hbox), app->widgets.play_button, FALSE, FALSE, 0);
	// コンテキストを削除
	cairo_surface_destroy(surface_p);
	cairo_destroy(cairo_p);

	// 一時停止のイメージを作成
		// 描画用のピクセルデータを作成
	pixels = (uint8*)MEM_ALLOC_FUNC(BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	(void)memset(pixels, 0, BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	// イメージウィジェットにセット出来る形式に変換
	pixbuf = gdk_pixbuf_new_from_data(pixels, GDK_COLORSPACE_RGB, TRUE, 8, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE,
		BUTTON_IMAGE_SIZE * 4, NULL, NULL);
	// 描画ライブラリにピクセルデータを渡す
		// 描画イメージ作成
	surface_p = cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_ARGB32,
		BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE * 4);
	// 描画コンテキストを作成
	cairo_p = cairo_create(surface_p);
	pixbuf = gdk_pixbuf_new_from_data(pixels, GDK_COLORSPACE_RGB, TRUE, 8, BUTTON_IMAGE_SIZE, BUTTON_IMAGE_SIZE,
		BUTTON_IMAGE_SIZE * 4, NULL, NULL);
	(void)memset(pixels, 0, BUTTON_IMAGE_SIZE*BUTTON_IMAGE_SIZE*4);
	cairo_set_source_rgb(cairo_p, 0, 0, 0);
	cairo_move_to(cairo_p, BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.35, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.35, BUTTON_IMAGE_SIZE * 0.1);
	cairo_fill(cairo_p);
	cairo_set_source_rgb(cairo_p, 0, 0, 0);
	cairo_move_to(cairo_p, BUTTON_IMAGE_SIZE * 0.65, BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE * 0.65, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1);
	cairo_line_to(cairo_p, BUTTON_IMAGE_SIZE - BUTTON_IMAGE_SIZE * 0.1, BUTTON_IMAGE_SIZE * 0.1);
	cairo_fill(cairo_p);
	image = gtk_image_new_from_pixbuf(pixbuf);
	app->widgets.pause_button = gtk_toggle_button_new();
	(void)g_signal_connect(G_OBJECT(app->widgets.pause_button), "toggled",
		G_CALLBACK(PauseButtonClicked), app);
	gtk_container_add(GTK_CONTAINER(app->widgets.pause_button), image);
	gtk_box_pack_start(GTK_BOX(hbox), app->widgets.pause_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	cairo_surface_destroy(surface_p);
	cairo_destroy(cairo_p);
	// 再生モードのボタンを作成
	app->widgets.play_mode_image = gtk_image_new_from_file("./image/play_once.png");
	button = gtk_button_new();
	(void)g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(ChangePlayModeButtonClicked), app);
	gtk_container_add(GTK_CONTAINER(button), app->widgets.play_mode_image);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 10);

	// ボリュームコントロールを作成
	app->widgets.volume_control = gtk_volume_button_new();
	// ウィンドウに追加
	gtk_box_pack_end(GTK_BOX(vbox), app->widgets.volume_control, FALSE, FALSE, 0);
	// ボリュームを設定
	gtk_scale_button_set_value(GTK_SCALE_BUTTON(app->widgets.volume_control), app->play_data.volume);
	// ボリューム操作時のコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(app->widgets.volume_control), "value-changed",
		G_CALLBACK(VolumeControl), app);

	// メインウィンドウを表示
	gtk_widget_show_all(app->widgets.window);
}
