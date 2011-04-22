/*
 * TazWeb is a radically simple web browser providing a single window
 * with a single toolbar with buttons and an URL entry, but no menu or
 * tabs.
 *
 * Copyright (C) 2011 SliTaz GNU/Linux <devel@slitaz.org>
 *
 */

#include <gtk/gtk.h>
#include <webkit/webkit.h>

static GtkWidget *main_window, *scrolled, *toolbar, *uri_entry, *search_entry;
static WebKitWebView* web_view;
static WebKitWebFrame* frame;
static gdouble load_progress;
static guint status_context_id;
static gchar* main_title;

const gchar* config;
const gchar* uri;

/* Create an icon */
static GdkPixbuf*
create_pixbuf (const gchar * image)
{
	GdkPixbuf *pixbuf;
	pixbuf = gdk_pixbuf_new_from_file (image, NULL);
	return pixbuf;
}

/* Get a default page.html if missing */
static void
get_config ()
{
	config = g_strdup_printf ("%s/.config/tazweb", g_get_home_dir ());
	if (! g_file_test(config, G_FILE_TEST_EXISTS)) {
		g_mkdir (config, 0700);
		system ("cp /usr/share/tazweb/* $HOME/.config/tazweb");
	}
}

/* Page title to window title */
static void
update_title (GtkWindow* window)
{
	GString* string = g_string_new (main_title);
	/* g_string_append (string, " - TazWeb"); */
	if (load_progress < 100)
		g_string_append_printf (string, " [ %f%% ] ", load_progress);
	gchar* title = g_string_free (string, FALSE);
	gtk_window_set_title (window, title);
	g_free (title);
}

static void
notify_title_cb (WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
	if (main_title)
		g_free (main_title);
	main_title = g_strdup (webkit_web_view_get_title(web_view));
	update_title (GTK_WINDOW (main_window));
}

/* Request progress in window title */
static void
notify_progress_cb (WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
	load_progress = webkit_web_view_get_progress (web_view) * 100;
	update_title (GTK_WINDOW (main_window));
}

static void
notify_load_status_cb (WebKitWebView* web_view, GParamSpec* pspec, gpointer data)
{
	if (webkit_web_view_get_load_status (web_view) == WEBKIT_LOAD_COMMITTED) {
		frame = webkit_web_view_get_main_frame (web_view);
		uri = webkit_web_frame_get_uri (frame);
		if (uri)
			gtk_entry_set_text (GTK_ENTRY (uri_entry), uri);
	}
}

static void
destroy_cb (GtkWidget* widget, gpointer data)
{
	gtk_main_quit ();
}

/* Show page source */
static void
view_source_cb ()
{
	gboolean source;
	
	frame = webkit_web_view_get_main_frame (web_view);
	uri = webkit_web_frame_get_uri (frame);
	source = webkit_web_view_get_view_source_mode (web_view);
	
	webkit_web_view_set_view_source_mode(web_view, !source);
	webkit_web_view_load_uri (web_view, uri);
}

/* URL entry callback function */
static void
uri_entry_cb (GtkWidget* entry, gpointer data)
{
	uri = gtk_entry_get_text (GTK_ENTRY (entry));
	g_assert (uri);
	webkit_web_view_load_uri (web_view, uri);
}

/* Search entry callback function */
static void
search_entry_cb (GtkWidget* entry, gpointer data)
{
	uri = g_strdup_printf ("http://www.google.com/search?q=%s",
			gtk_entry_get_text (GTK_ENTRY (entry)));
	g_assert (uri);
	webkit_web_view_load_uri (web_view, uri);
}

/* Home button callback function */
static void
go_home_cb (GtkWidget* widget, gpointer data)
{
	uri = g_strdup_printf ("file://%s/.config/tazweb/home.html",
			g_get_home_dir ());
	g_assert (uri);
	webkit_web_view_load_uri (web_view, uri);
}

/* Fullscreen and unfullscreen callback function */
static void
fullscreen_cb (GtkWindow* window, gpointer data)
{
	GdkWindowState state;
	state = gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (main_window)));

	if (state & GDK_WINDOW_STATE_FULLSCREEN)
		gtk_window_unfullscreen (GTK_WINDOW (main_window));
	else
		gtk_window_fullscreen (GTK_WINDOW (main_window));
}

/* TazWeb doc callback function */
static void
tazweb_doc_cb (GtkWidget* widget, gpointer data)
{
	uri = ("file:///usr/share/doc/tazweb/tazweb.html");
	g_assert (uri);
	webkit_web_view_load_uri (web_view, uri);
}

/* Download function */
static gboolean
download_requested_cb (WebKitWebView *web_view, WebKitDownload *download,
		gpointer user_data)
{
	uri = webkit_download_get_uri (download);
	gchar *buffer;
	asprintf (&buffer, "tazbox dl-out %s", uri);
	system (buffer);
}

/* Add items to WebKit contextual menu */
static void
populate_menu_cb (WebKitWebView *web_view, GtkMenu *menu, gpointer data)
{
	GtkWidget *item;
	
	/* separator */
	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	/* TazWeb documentation */
	item = gtk_image_menu_item_new_with_label ("TazWeb manual");
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
	gtk_image_new_from_stock (GTK_STOCK_INFO, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK (tazweb_doc_cb), NULL);

	/* View source mode */
	item = gtk_image_menu_item_new_with_label ("View source mode");
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
	gtk_image_new_from_stock (GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (item, "activate", G_CALLBACK (view_source_cb), NULL);
	
	gtk_widget_show_all (GTK_WIDGET (menu));
}

/* Scrolled window for the web_view */
static GtkWidget*
create_browser ()
{
	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	web_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
	gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (web_view));

	/* Connect events */
	g_signal_connect (web_view, "notify::title",
			G_CALLBACK (notify_title_cb), web_view);
	g_signal_connect (web_view, "notify::progress",
			G_CALLBACK (notify_progress_cb), web_view);
	g_signal_connect (web_view, "notify::load-status",
			G_CALLBACK (notify_load_status_cb), web_view);
	g_signal_connect (web_view, "download-requested",
			G_CALLBACK (download_requested_cb), NULL);

	/* Connect WebKit contextual menu items */
	g_object_connect (G_OBJECT (web_view), "signal::populate-popup",
		G_CALLBACK (populate_menu_cb), web_view, NULL);

	return scrolled;
}

static GtkWidget*
create_toolbar ()
{
	GtkToolItem* item;

	toolbar = gtk_toolbar_new ();
	gtk_widget_set_size_request (toolbar, 0, 31);
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar),
			GTK_TOOLBAR_BOTH_HORIZ);

	/* Home button */
	item = gtk_tool_button_new_from_stock (GTK_STOCK_HOME);
	g_signal_connect (G_OBJECT (item), "clicked",
			G_CALLBACK (go_home_cb), NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

	/* URL entry */
	item = gtk_tool_item_new ();
	gtk_tool_item_set_expand (item, TRUE);
	uri_entry = gtk_entry_new ();
	gtk_container_add (GTK_CONTAINER (item), uri_entry);
	g_signal_connect (G_OBJECT (uri_entry), "activate",
			G_CALLBACK (uri_entry_cb), NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

	/* Separator */
	item = gtk_separator_tool_item_new ();
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1); 
	
	/* Search entry */
	item = gtk_tool_item_new ();
	search_entry = gtk_entry_new ();
	gtk_container_add (GTK_CONTAINER (item), search_entry);
	g_signal_connect (G_OBJECT (search_entry), "activate",
			G_CALLBACK (search_entry_cb), NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

	/* The Fullscreen button */
	item = gtk_tool_button_new_from_stock (GTK_STOCK_FULLSCREEN);
	g_signal_connect (G_OBJECT (item), "clicked",
			G_CALLBACK (fullscreen_cb), NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

	return toolbar;
}

/* Main window */
static GtkWidget*
create_window ()
{
	GtkWidget* window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	GtkWidget* vbox = gtk_vbox_new (FALSE, 0);
	
	gtk_box_pack_start (GTK_BOX (vbox), create_browser (), TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), create_toolbar (), FALSE, FALSE, 0);
	
	/* Default TazWeb window size ratio to 3/4 --> 720, 540*/
	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
	gtk_window_set_icon (GTK_WINDOW (window),
			create_pixbuf ("/usr/share/pixmaps/tazweb.png"));
	gtk_widget_set_name (window, "TazWeb");
	g_signal_connect (window, "destroy", G_CALLBACK (destroy_cb), NULL);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	main_window = window;
	gtk_widget_show_all (main_window);
}

int
main (int argc, char* argv[])
{
	gtk_init (&argc, &argv);
	if (!g_thread_supported ())
		g_thread_init (NULL);

	get_config ();
	create_window ();
	
	/* Start page url or file */
	uri = (gchar*) (argc > 1 ? argv[1] :
			"file:///usr/share/webhome/index.html");
	
	webkit_web_view_load_uri (web_view, uri);
	gtk_widget_grab_focus (GTK_WIDGET (web_view));
	
	gtk_main ();
	return 0;
}
