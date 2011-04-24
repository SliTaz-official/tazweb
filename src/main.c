/*
 * TazWeb is a radically simple web browser providing a single window
 * with a single toolbar with buttons, an URL entry and search as well
 * as a contextual menu, but no menu bar or tabs. Commented line code
 * starts with // and comments are between * *
 *
 * Copyright(C) 2011 SliTaz GNU/Linux - BSD License
 * See AUTHORS and LICENSE for detailed information
 * 
 */

#include <gtk/gtk.h>
#include <webkit/webkit.h>

#define CONFIG   g_strdup_printf("%s/.config/tazweb", g_get_home_dir())
#define START    "file:///usr/share/webhome/index.html"

/* Colors(Loader: #d66018 #7b705c) */
static gchar *loadfg    = "#351a0a";
static gchar *toolbarbg = "#f1efeb";

/* Needs AppleWebKit/531.2+ to handle all sites ? */
static gchar *useragent = "TazWeb (X11; SliTaz GNU/Linux; U; en_US)";

static gchar* pagetitle;
static GtkWidget *mainwindow, *browser, *loader, *toolbar;
static GtkWidget *urientry, *search;
static WebKitWebView* webview;
static WebKitWebFrame* frame;
static gint progress;
static gint count = 0;
const gchar* uri;
static GtkWidget* create_window(WebKitWebView** newwebview);

/* Create an icon */
static GdkPixbuf*
create_pixbuf(const gchar* image)
{
	GdkPixbuf *pixbuf;
	pixbuf = gdk_pixbuf_new_from_file(image, NULL);
	return pixbuf;
}

/* Can be: http://hg.slitaz.org or hg.slitaz.org */
static void
check_requested_uri()
{
	uri = g_strrstr(uri, "://") ? g_strdup(uri)
		: g_strdup_printf("http://%s", uri);
}

/* Loader area */
static void
update_loader()
{
	GdkGC* gc;
	GdkColor fg;
	gint width;

	gc = gdk_gc_new(loader->window);
	width = progress * loader->allocation.width / 100;
	
	gdk_color_parse(loadfg, &fg);
	gdk_gc_set_rgb_fg_color(gc, &fg);
	gdk_draw_rectangle(loader->window,
			loader->style->bg_gc [GTK_WIDGET_STATE(loader)],
			TRUE, 0, 0, loader->allocation.width, loader->allocation.height);
	gdk_draw_rectangle(loader->window, gc, TRUE, 0, 0, width,
			loader->allocation.height);
	g_object_unref(gc);
}

/* Loader progress */
static gboolean
expose_loader_cb(GtkWidget *loader, GdkEventExpose *event, gpointer data)
{
	update_loader();
	return TRUE;
}

/* Update title */
static void
update_title(GtkWindow* window, WebKitWebView* webview)
{
	GString *string = g_string_new(webkit_web_view_get_title(webview));
	gdouble loadprogress = webkit_web_view_get_progress(webview) * 100;
	//g_string_append(string, " - TazWeb");
	gchar *title = g_string_free(string, FALSE);
	gtk_window_set_title(window, title);
	g_free(title);
}

/* Get the page title */
static void
notify_title_cb(WebKitWebView* webview, GParamSpec* pspec, GtkWidget* window)
{
	pagetitle = g_strdup(webkit_web_view_get_title(webview));
	update_title(GTK_WINDOW(window), webview);
	update_loader();
}

/* Request progress in window title */
static void
notify_progress_cb(WebKitWebView* webview, GParamSpec* pspec, GtkWidget* window)
{
	progress = webkit_web_view_get_progress(webview) * 100;
	update_title(GTK_WINDOW(window), webview);
	update_loader();
}

/* Notify loader and url entry */
static void
notify_load_status_cb(WebKitWebView* webview, GParamSpec* pspec, gpointer data)
{
	switch(webkit_web_view_get_load_status(webview))
	{
	case WEBKIT_LOAD_COMMITTED:
		break;
	case WEBKIT_LOAD_FINISHED:
		progress = 0;
		update_loader();
		break;
	}
	frame = webkit_web_view_get_main_frame(webview);
	uri = webkit_web_frame_get_uri(frame);
		if (uri)
			gtk_entry_set_text(GTK_ENTRY(urientry), uri);
}

/* Destroy the window */
static void
destroy_cb(GtkWidget* widget, GtkWindow* window)
{
	if (g_atomic_int_dec_and_test(&count))
		gtk_main_quit();
}

/* Show page source */
static void
view_source_cb()
{
	gboolean source;
	
	frame = webkit_web_view_get_main_frame(webview);
	uri = webkit_web_frame_get_uri(frame);
	source = webkit_web_view_get_view_source_mode(webview);
	
	webkit_web_view_set_view_source_mode(webview, !source);
	webkit_web_view_load_uri(webview, uri);
}

/* URL entry callback function */
static void
uri_entry_cb(GtkWidget* entry, gpointer data)
{
	uri = gtk_entry_get_text(GTK_ENTRY(entry));
	g_assert(uri);
	check_requested_uri();
	webkit_web_view_load_uri(webview, uri);
}

/* Search entry callback function */
static void
search_entry_cb(GtkWidget* entry, gpointer data)
{
	uri = g_strdup_printf("http://www.google.com/search?q=%s",
			gtk_entry_get_text(GTK_ENTRY(entry)));
	g_assert(uri);
	webkit_web_view_load_uri(webview, uri);
}

/* Home button callback function */
static void
go_home_cb(GtkWidget* widget, gpointer data)
{
	uri = g_strdup_printf("file://%s/home.html", CONFIG);
	g_assert(uri);
	webkit_web_view_load_uri(webview, uri);
}

/* Navigation button function */
static void
go_back_cb(GtkWidget* widget, gpointer data)
{
    webkit_web_view_go_back(webview);
}

static void
go_forward_cb(GtkWidget* widget, gpointer data)
{
    webkit_web_view_go_forward(webview);
}

/* Fullscreen and unfullscreen callback function */
static void
fullscreen_cb(GtkWindow* window, gpointer data)
{
	GdkWindowState state;
	state = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(mainwindow)));

	if(state & GDK_WINDOW_STATE_FULLSCREEN)
		gtk_window_unfullscreen(GTK_WINDOW(mainwindow));
	else
		gtk_window_fullscreen(GTK_WINDOW(mainwindow));
}

/* TazWeb doc callback function */
static void
tazweb_doc_cb(GtkWidget* widget, gpointer data)
{
	uri = ("file:///usr/share/doc/tazweb/tazweb.html");
	g_assert(uri);
	webkit_web_view_load_uri(webview, uri);
}

/* Download function */
static gboolean
download_requested_cb(WebKitWebView *webview, WebKitDownload *download,
		gpointer user_data)
{
	uri = webkit_download_get_uri(download);
	const gchar* buffer;
	asprintf(&buffer,
			"xterm -T 'Download' -geom 72x10+0-24 -e 'cd $HOME/Downloads && \
				wget -c %s; sleep 2' &", uri);
	system(buffer);
}

/* Zoom out and in callback function */
static void
zoom_out_cb(GtkWidget *mainwindow)
{
	webkit_web_view_zoom_out(webview);
}

static void
zoom_in_cb(GtkWidget *mainwindow)
{
	webkit_web_view_zoom_in(webview);
}

/* New webview clallbacks */
static WebKitWebView*
create_web_view_cb(WebKitWebView* webview, WebKitWebFrame* web_frame,
		GtkWidget* window)
{
	WebKitWebView *newview;
	create_window(&newview);
	return newview;
}

static gboolean
webview_ready_cb(WebKitWebView* webview, GtkWidget* window)
{
	gtk_widget_grab_focus(GTK_WIDGET(webview));
	gtk_widget_show_all(window);
	return FALSE;
}

static gboolean
close_webview_cb(WebKitWebView* webview, GtkWidget* window)
{
	gtk_widget_destroy(window);
	return TRUE;
}

/* Add items to WebKit contextual menu */
static void
populate_menu_cb(WebKitWebView *webview, GtkMenu *menu, gpointer data)
{
	GtkWidget* item;
	
	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Zoom in */
	item = gtk_image_menu_item_new_with_label("Zoom in");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_ZOOM_IN, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(zoom_in_cb), NULL);
	
	/* Zoom out */
	item = gtk_image_menu_item_new_with_label("Zoom out");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_ZOOM_OUT, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(zoom_out_cb), NULL);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* TazWeb documentation */
	item = gtk_image_menu_item_new_with_label("TazWeb manual");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_INFO, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(tazweb_doc_cb), NULL);

	/* View source mode */
	item = gtk_image_menu_item_new_with_label("View source mode");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(view_source_cb), NULL);
	
	gtk_widget_show_all(GTK_WIDGET(menu));
}

/* Scrolled window for the webview */
static GtkWidget*
create_browser(GtkWidget* window)
{
	WebKitWebSettings *settings;
	
	browser = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(browser),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
	gtk_container_add(GTK_CONTAINER(browser), GTK_WIDGET(webview));

	/* User agent */
	settings = webkit_web_view_get_settings (webview);
	g_object_set (G_OBJECT (settings), "user-agent", useragent, NULL);

	/* Connect WebKit events */
	g_signal_connect(webview, "notify::title",
			G_CALLBACK(notify_title_cb), window);
	g_signal_connect(webview, "notify::progress",
			G_CALLBACK(notify_progress_cb), window);
	g_signal_connect(webview, "notify::load-status",
			G_CALLBACK(notify_load_status_cb), window);
	g_signal_connect(webview, "download-requested",
			G_CALLBACK(download_requested_cb), NULL);
	g_signal_connect(webview, "create-web-view",
			G_CALLBACK(create_web_view_cb), window);
	g_signal_connect(webview, "web-view-ready",
			G_CALLBACK(webview_ready_cb), window);
	g_signal_connect(webview, "close-web-view",
			G_CALLBACK(close_webview_cb), window);

	/* Connect WebKit contextual menu items */
	g_object_connect(G_OBJECT(webview), "signal::populate-popup",
		G_CALLBACK(populate_menu_cb), webview, NULL);

	return browser;
}

/* Loader area */
static GtkWidget*
create_loader()
{
	loader = gtk_drawing_area_new();
	gtk_widget_set_size_request(loader, 0, 2);
	g_signal_connect(G_OBJECT(loader), "expose_event",
		G_CALLBACK(expose_loader_cb), NULL);
	
	return loader;
}

static GtkWidget*
create_toolbar()
{
	GtkToolItem* item;
	GdkColor bg;
	
	toolbar = gtk_toolbar_new();
	//gtk_widget_set_size_request(toolbar, 0, 24);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar),
			GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),
			GTK_TOOLBAR_BOTH_HORIZ);
	gdk_color_parse(toolbarbg, &bg);
	gtk_widget_modify_bg(toolbar, GTK_STATE_NORMAL, &bg);

	/* The back button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(go_back_cb), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

    /* The forward button */
    item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
    g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(go_forward_cb), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

    /* Home button */
	item = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
	g_signal_connect(G_OBJECT(item), "clicked",
			G_CALLBACK(go_home_cb), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	/* URL entry */
	item = gtk_tool_item_new();
	gtk_tool_item_set_expand(item, TRUE);
	
	urientry = gtk_entry_new();
	gtk_widget_modify_base( GTK_WIDGET(urientry),
			GTK_STATE_NORMAL, &bg);
	gtk_entry_set_inner_border(GTK_ENTRY(urientry), NULL);
	gtk_entry_set_has_frame(GTK_ENTRY(urientry), FALSE);
	
	gtk_container_add(GTK_CONTAINER(item), urientry);
	g_signal_connect(G_OBJECT(urientry), "activate",
			G_CALLBACK(uri_entry_cb), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	/* Separator */
	//item = gtk_separator_tool_item_new();
	//gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1); 
	
	/* Search entry */
	item = gtk_tool_item_new();
	search = gtk_entry_new();
	gtk_widget_set_size_request(search, 150, 20);
	gtk_container_add(GTK_CONTAINER(item), search);
	g_signal_connect(G_OBJECT(search), "activate",
			G_CALLBACK(search_entry_cb), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	/* The Fullscreen button */
	item = gtk_tool_button_new_from_stock(GTK_STOCK_FULLSCREEN);
	g_signal_connect(G_OBJECT(item), "clicked",
			G_CALLBACK(fullscreen_cb), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	return toolbar;
}

/* Main window */
static GtkWidget*
create_window(WebKitWebView** newwebview)
{
	g_atomic_int_inc(&count);
	
	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0);

	/* Default TazWeb window size ratio to 3/4 --> 720, 540 */
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_window_set_icon(GTK_WINDOW(window),
			create_pixbuf("/usr/share/pixmaps/tazweb.png"));
	gtk_widget_set_name(window, "TazWeb");
	g_signal_connect(window, "destroy", G_CALLBACK(destroy_cb), NULL);

	/* Pack box and container */
	gtk_box_pack_start(GTK_BOX(vbox), create_browser(window), TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(vbox), create_loader());
	gtk_box_set_child_packing(GTK_BOX(vbox), loader,
		FALSE, FALSE, 0, GTK_PACK_START);
	gtk_box_pack_start(GTK_BOX(vbox), create_toolbar(), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	if (newwebview)
		*newwebview = webview;
	
	return window;	
}

int
main(int argc, char* argv[])
{
	gtk_init(&argc, &argv);
	if (!g_thread_supported())
		g_thread_init(NULL);

	/* Get a default home.html if missing */
	if (! g_file_test(CONFIG, G_FILE_TEST_EXISTS))
		system("cp -r /usr/share/tazweb $HOME/.config/tazweb");

	/* Load the start page file or the url in argument */
	uri =(char*)(argc > 1 ? argv[1] : START);
	if (argv[1])
		check_requested_uri();
		
	mainwindow = create_window(&webview);
	gtk_widget_show_all(mainwindow);
	
	webkit_web_view_load_uri(webview, uri);
	gtk_widget_grab_focus(GTK_WIDGET(webview));
	
	gtk_main();
	
	return 0;
}
