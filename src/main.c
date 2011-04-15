/*
 * TazWeb is a radicaly simple web browser providing a single window
 * with a single toolbar with buttons and URL entry but no menu or tabs.
 *
 * 
 * Copyright (C) 2011 SliTaz GNU/Linux <devel@slitaz.org>
 * 
 * 
 */

#include <gtk/gtk.h>
#include <webkit/webkit.h>

static GtkWidget* main_window;
static WebKitWebView* web_view;
static GtkWidget* uri_entry;
static gchar* main_title;
static gdouble load_progress;
static guint status_context_id;

/* Page title to window title */
static void
update_title (GtkWindow* window)
{
    GString* string = g_string_new (main_title);
    /* g_string_append (string, " - TazWeb"); */
    if (load_progress < 100)
        g_string_append_printf (string, " (%f%%)", load_progress);
    gchar* title = g_string_free (string, FALSE);
    gtk_window_set_title (window, title);
    g_free (title);
}

static void
activate_uri_entry_cb (GtkWidget* entry, gpointer data)
{
	const gchar* uri = gtk_entry_get_text (GTK_ENTRY (entry));
	g_assert (uri);
	webkit_web_view_load_uri (web_view, uri);
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
        WebKitWebFrame* frame = webkit_web_view_get_main_frame (web_view);
        const gchar* uri = webkit_web_frame_get_uri (frame);
        if (uri)
            gtk_entry_set_text (GTK_ENTRY (uri_entry), uri);
    }
}

static void
destroy_cb (GtkWidget* widget, gpointer data)
{
    gtk_main_quit ();
}

/* Home button function */
static void
go_home_cb (GtkWidget* widget, gpointer data)
{
	const gchar* uri = ("file:///usr/share/webhome/index.html");	
    g_assert (uri);
    webkit_web_view_load_uri (web_view, uri);
}

/* Bookmarks button function */
static void
bookmarks_cb (GtkWidget* widget, gpointer data)
{
    const gchar* uri = g_strdup_printf ("file://%s/.config/tazweb/page.html",
                                         g_get_home_dir ());
    g_assert (uri);
    webkit_web_view_load_uri (web_view, uri);
}

/* Navigation button function */
static void
go_back_cb (GtkWidget* widget, gpointer data)
{
    webkit_web_view_go_back (web_view);
}

static void
go_forward_cb (GtkWidget* widget, gpointer data)
{
    webkit_web_view_go_forward (web_view);
}

static void
refresh_cb (GtkWidget* widget, gpointer data)
{
    webkit_web_view_reload (web_view);
}

/* Fullscreen and unfullscreen action */
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

/* TazWeb doc function */
static void
tazweb_doc_cb (GtkWidget* widget, gpointer data)
{
    const gchar* uri = ("file:///usr/share/doc/tazweb/tazweb.html");
    g_assert (uri);
    webkit_web_view_load_uri (web_view, uri);
}

static GtkWidget*
create_browser ()
{
    GtkWidget* scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    web_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
    gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (web_view));

    g_signal_connect (web_view, "notify::title", G_CALLBACK (notify_title_cb), web_view);
    g_signal_connect (web_view, "notify::progress", G_CALLBACK (notify_progress_cb), web_view);
    g_signal_connect (web_view, "notify::load-status", G_CALLBACK (notify_load_status_cb), web_view);

    return scrolled_window;
}

/* Create an icon */
static GdkPixbuf*
create_pixbuf (const gchar * image)
{
   GdkPixbuf *pixbuf;
   pixbuf = gdk_pixbuf_new_from_file (image, NULL);

   return pixbuf;
}

static GtkWidget*
create_toolbar ()
{
    GtkWidget* toolbar = gtk_toolbar_new ();
    GtkToolItem* item;

    gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH_HORIZ);

    /* The back button */
    item = gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK);
    g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (go_back_cb), NULL);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

    /* The forward button */
    item = gtk_tool_button_new_from_stock (GTK_STOCK_GO_FORWARD);
    g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (go_forward_cb), NULL);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

    /* The Reload button */
    item = gtk_tool_button_new_from_stock (GTK_STOCK_REFRESH);
    g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (refresh_cb), NULL);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

	/* The URL entry */
	item = gtk_tool_item_new ();
	gtk_tool_item_set_expand (item, TRUE);
	uri_entry = gtk_entry_new ();
	gtk_container_add (GTK_CONTAINER (item), uri_entry);
	g_signal_connect (G_OBJECT (uri_entry), "activate",
		              G_CALLBACK (activate_uri_entry_cb), NULL);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

	/* The Home button */
    item = gtk_tool_button_new_from_stock (GTK_STOCK_HOME);
    g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (go_home_cb), NULL);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

    /* The Bookmarks button */
    item = gtk_tool_button_new_from_stock (GTK_STOCK_PREFERENCES);
    g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (bookmarks_cb), NULL);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

    /* The TazWeb doc button */
    item = gtk_tool_button_new_from_stock (GTK_STOCK_INFO);
    g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (tazweb_doc_cb), NULL);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

    /* The Fullscreen button */
    item = gtk_tool_button_new_from_stock (GTK_STOCK_FULLSCREEN);
    g_signal_connect (G_OBJECT (item), "clicked", G_CALLBACK (fullscreen_cb), NULL);
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);

    return toolbar;
}

static GtkWidget*
create_window ()
{
    GtkWidget* window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    /* Default tazweb window size ratio to 3/4 ?? --> 720, 540*/
    gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
    gtk_window_set_icon (GTK_WINDOW (window),
		                 create_pixbuf ("/usr/share/pixmaps/tazweb.png"));
    gtk_widget_set_name (window, "TazWeb");
    g_signal_connect (window, "destroy", G_CALLBACK (destroy_cb), NULL);

    return window;
}

int
main (int argc, char* argv[])
{
    gtk_init (&argc, &argv);
    if (!g_thread_supported ())
        g_thread_init (NULL);

    /* Get a default bookmark.html if missing */
	const gchar* config = g_strdup_printf ("%s/.config/tazweb", g_get_home_dir ());
    if (!g_file_test(config, G_FILE_TEST_EXISTS)) {
	    g_mkdir(config, 0700);
        system("cp /usr/share/tazweb/*.html $HOME/.config/tazweb");
        system("cp /usr/share/tazweb/*.css $HOME/.config/tazweb");
    }

    GtkWidget* vbox = gtk_vbox_new (FALSE, 2);
    gtk_box_pack_start (GTK_BOX (vbox), create_browser (), TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), create_toolbar (), FALSE, FALSE, 0);

    main_window = create_window ();
    gtk_container_add (GTK_CONTAINER (main_window), vbox);

	/* Home page url or file */
    gchar* uri = (gchar*) (argc > 1 ? argv[1] :
		                   "file:///usr/share/webhome/index.html");
    webkit_web_view_load_uri (web_view, uri);

    gtk_widget_grab_focus (GTK_WIDGET (web_view));
    gtk_widget_show_all (main_window);
    gtk_main ();

    return 0;
}
