/*
 * TazWeb is a radicaly simple web browser providing a single window
 * with no toolbar, menu or tabs.
 *
 * 
 * Copyright (C) 2011 SliTaz GNU/Linux <devel@slitaz.org>
 * 
 * 
 */

#include <gtk/gtk.h>
#include <webkit/webkit.h>

static GtkWidget* main_window;
static GtkWidget* uri_entry;
static WebKitWebView* web_view;
static gchar* main_title;
static gdouble load_progress;
static guint status_context_id;

/* Page title to window title */
static void
update_title (GtkWindow* window)
{
    GString* string = g_string_new (main_title);
    g_string_append (string, " - TazWeb");
    if (load_progress < 100)
        g_string_append_printf (string, " (%f%%)", load_progress);
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
destroy_cb (GtkWidget* widget, gpointer data)
{
    gtk_main_quit ();
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

    GtkWidget* vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), create_browser (), TRUE, TRUE, 0);

    main_window = create_window ();
    gtk_container_add (GTK_CONTAINER (main_window), vbox);

	/* Home page url or file */
    gchar* uri = (gchar*) (argc > 1 ? argv[1] : "file:///usr/share/webhome/index.html");
    webkit_web_view_load_uri (web_view, uri);

    gtk_widget_grab_focus (GTK_WIDGET (web_view));
    gtk_widget_show_all (main_window);
    gtk_main ();

    return 0;
}
