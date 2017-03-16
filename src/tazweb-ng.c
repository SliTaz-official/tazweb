/*
 * TazWeb is a radically simple web browser providing a single window
 * with a single toolbar with buttons, a URL entry and search as well
 * as a contextual menu, but no menu bar or tabs. Commented line code
 * starts with // and comments are between * *
 *
 * NEXT GENERATION
 *  This is TazWeb NG (2.0) with tabs and cookies support :-)
 *
 * Copyright (C) 2011-2017 SliTaz GNU/Linux - BSD License
 * See AUTHORS and LICENSE for detailed information
 *
 */

#include <glib.h>
#include <glib/gi18n.h>
#define GETTEXT_PACKAGE "tazweb"

#include <stdlib.h>
#include <sys/queue.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h> // for cookies

#define HOME		g_get_home_dir()
#define CONFIG		g_strdup_printf("%s/.config/tazweb", HOME)
#define BMTXT		g_strdup_printf("%s/bookmarks.txt", CONFIG)
#define BMURL		g_strdup_printf("%s/bookmarks.html", CONFIG)
#define COOKIES		g_strdup_printf("%s/cookies.txt", CONFIG)
#define WEBHOME		"file:///usr/share/webhome/index.html"
#define SEARCH		"http://duckduckgo.com/?q=%s&t=slitaz"

static gchar *useragent = "TazWeb (X11; SliTaz GNU/Linux; U; en_US) AppleWebKit/535.22+";
static GtkWidget *tazweb_window;
static GtkNotebook *notebook;
static gboolean notoolbar;
static gboolean kiosk;

const gchar* uri;

static SoupSession		*session;
static SoupCookieJar	*cookiejar;

/* Tab structure */
struct tab {
	TAILQ_ENTRY(tab) entry;
	GtkWidget		*vbox;
	GtkWidget		*label;
	GtkWidget		*urientry;
	GtkWidget		*search_entry;
	GtkWidget		*toolbar;
	GtkWidget		*browser;
	GtkToolItem		*backward;
	GtkToolItem		*forward;
	GtkToolItem		*home;
	GtkToolItem		*bookmarks;
	GtkToolItem		*ttb;
	GtkWidget		*spinner;
	guint			tab_id;
	int				focus_wv;
	WebKitWebView	*webview;
};
TAILQ_HEAD(tab_list, tab);
struct tab_list tabs;

/* Protocols */
void create_new_tab(char *, int);
void close_tab(struct tab *);

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

int destroy_cb()
{
	gtk_main_quit();
	return (1);
}

int focus()
{
	return (0);
}

static void
uri_entry_cb(GtkWidget* entry, struct tab *ttb)
{
	uri = gtk_entry_get_text(GTK_ENTRY(entry));
	g_assert(uri);
	check_requested_uri();
	webkit_web_view_load_uri(ttb->webview, uri);
	gtk_widget_grab_focus(GTK_WIDGET(ttb->webview));
}

static void
notify_load_status_cb(WebKitWebView* webview, GParamSpec* pspec,
	struct tab *ttb)
{
	GString 		*string;
	WebKitWebFrame 	*frame;
	const gchar		*uri, *title;
	gdouble 		progress;

	switch (webkit_web_view_get_load_status(webview)) {

	/* Webkit is loading */
	case WEBKIT_LOAD_COMMITTED:
		frame = webkit_web_view_get_main_frame(webview);
		uri = webkit_web_frame_get_uri(frame);

		string = g_string_new("Loading");
		progress = webkit_web_view_get_progress(webview) * 100;
		if (progress < 100)
			g_string_append_printf(string, " (%f%%)", progress);

		title = g_string_free(string, FALSE);
		gtk_label_set_text(GTK_LABEL(ttb->label), title);

		/* Start spinner */
		gtk_widget_show(ttb->spinner);
		gtk_spinner_start(GTK_SPINNER(ttb->spinner));

		/* Update URL entry */
		if (uri)
			gtk_entry_set_text(GTK_ENTRY(ttb->urientry), uri);

		/* Focus */
		ttb->focus_wv = 1;
		if (gtk_notebook_get_current_page(notebook) == ttb->tab_id)
			gtk_widget_grab_focus(GTK_WIDGET(ttb->webview));
	break;

	/* First layout with actual visible content */
	case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
		title = webkit_web_view_get_title(webview);
		gtk_label_set_text(GTK_LABEL(ttb->label), title);
	break;

	/* URL was loaded */
	case WEBKIT_LOAD_FINISHED:
		title = webkit_web_view_get_title(webview);
		gtk_label_set_text(GTK_LABEL(ttb->label), title);
		gtk_spinner_stop(GTK_SPINNER(ttb->spinner));
		gtk_widget_hide(ttb->spinner);
	break;

		/* URL fail to load */
	case WEBKIT_LOAD_FAILED:
		gtk_label_set_text(GTK_LABEL(ttb->label), "Failed");
		gtk_spinner_stop(GTK_SPINNER(ttb->spinner));
		gtk_widget_hide(ttb->spinner);
	break;

	}

	gtk_widget_set_sensitive(GTK_WIDGET(ttb->backward),
		webkit_web_view_can_go_back(webview));

	gtk_widget_set_sensitive(GTK_WIDGET(ttb->forward),
		webkit_web_view_can_go_forward(webview));
}

/* Search entry and icon callback function */
static void
search_entry_cb(GtkWidget* search_entry, struct tab *ttb)
{
	uri = g_strdup_printf(SEARCH, gtk_entry_get_text(GTK_ENTRY(search_entry)));
	g_assert(uri);
	webkit_web_view_load_uri(ttb->webview, uri);
}

static void
search_icon_cb(GtkWidget *search_entry, GtkEntryIconPosition pos,
	GdkEvent *event, struct tab *ttb)
{
	search_entry_cb(search_entry, ttb);
}

/* Navigation button functions */
static void
go_back_cb(GtkWidget *widget, struct tab *ttb)
{
	webkit_web_view_go_back(ttb->webview);
}

static void
go_forward_cb(GtkWidget *widget, struct tab *ttb)
{
	webkit_web_view_go_forward(ttb->webview);
}

static void
go_home_cb(GtkWidget* w, struct tab *ttb)
{
	uri = WEBHOME;
	g_assert(uri);
	webkit_web_view_load_uri(ttb->webview, uri);
}

static void
go_bookmarks_cb(GtkWidget* w, struct tab *ttb)
{
	system("/usr/lib/tazweb/helper.sh html_bookmarks");
	uri = g_strdup_printf("file://%s", BMURL);
	g_assert(uri);
	webkit_web_view_load_uri(ttb->webview, uri);
}

/* Setup session cookies */
void
cookies_setup(void)
{
	if (cookiejar) {
		soup_session_remove_feature(session,
			(SoupSessionFeature*)cookiejar);
		g_object_unref(cookiejar);
		cookiejar = NULL;
	}

	cookiejar = soup_cookie_jar_text_new(COOKIES, 0);
	soup_session_add_feature(session, (SoupSessionFeature*)cookiejar);
}

static void
cookies_view_cb(GtkWidget* widget, WebKitWebView* webview)
{
	system("/usr/lib/tazweb/helper.sh html_cookies");
	uri = g_strdup_printf("file://%s/cookies.html", CONFIG);
	g_assert(uri);
	webkit_web_view_load_uri(webview, uri);
}

static void
cookies_clean_cb(GtkWidget* widget, WebKitWebView* webview)
{
	system("/usr/lib/tazweb/helper.sh clean_cookies");
}

/* Add items to WebKit contextual menu */
static void
populate_menu_cb(WebKitWebView *ttb, GtkMenu *menu, gpointer data)
{
	GtkWidget* item;

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	//if (! kiosk) {
		///* Add a bookmark */
		//item = gtk_image_menu_item_new_with_label(_("Add a bookmark"));
		//gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		//gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
		//gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		//g_signal_connect(item, "activate", G_CALLBACK(add_bookmark_cb), webview);
	//}

	///* Printing */
	//item = gtk_image_menu_item_new_with_label(_("Print this page"));
	//gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	//gtk_image_new_from_stock(GTK_STOCK_PRINT, GTK_ICON_SIZE_MENU));
	//gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	//g_signal_connect(item, "activate", G_CALLBACK(print_page_cb), webview);

	///* View source mode */
	//item = gtk_image_menu_item_new_with_label(_("View source mode"));
	//gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	//gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
	//gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	//g_signal_connect(item, "activate", G_CALLBACK(view_source_cb), webview);

	///* Separator */
	//item = gtk_separator_menu_item_new();
	//gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	///* TazWeb documentation */
	//item = gtk_image_menu_item_new_with_label(_("TazWeb manual"));
	//gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	//gtk_image_new_from_stock(GTK_STOCK_HELP, GTK_ICON_SIZE_MENU));
	//gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	//g_signal_connect(item, "activate", G_CALLBACK(tazweb_doc_cb), webview);

	/* Quit TazWeb */
	item = gtk_image_menu_item_new_with_label(_("Quit TazWeb"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(destroy_cb), NULL);

	gtk_widget_show_all(GTK_WIDGET(menu));
}

/* The browser */
GtkWidget *
create_browser(struct tab *ttb)
{
	GtkWidget *window;
	WebKitWebSettings *settings;
	
	window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	ttb->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(ttb->webview));
	
	/* User agent */
	settings = webkit_web_view_get_settings (ttb->webview);
	g_object_set(G_OBJECT(settings), "user-agent", useragent, NULL);

	g_signal_connect(ttb->webview, "notify::load-status",
		G_CALLBACK(notify_load_status_cb), ttb);

	/* Connect WebKit contextual menu items */
	g_object_connect(G_OBJECT(ttb->webview), "signal::populate-popup",
		G_CALLBACK(populate_menu_cb), ttb, NULL);

	return (window);
}

/* Create a new tab callback */
static void
create_new_tab_cb()
{
	create_new_tab(WEBHOME, +1);
}

/* Toolbar with URL and search entry */
GtkWidget *
create_toolbar(struct tab *ttb)
{
	GtkWidget		*toolbar = gtk_toolbar_new();
	GtkToolItem		*item;

	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar),
		GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),
		GTK_TOOLBAR_BOTH_HORIZ);
	
	/* New tab button */
	item = gtk_tool_button_new_from_stock(GTK_STOCK_ADD);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
	g_signal_connect(G_OBJECT(item), "clicked",
		G_CALLBACK(create_new_tab_cb), FALSE);

	/* The backward button */
	ttb->backward = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	gtk_widget_set_sensitive(GTK_WIDGET(ttb->backward), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ttb->backward, -1);
	g_signal_connect(G_OBJECT(ttb->backward), "clicked",
		G_CALLBACK(go_back_cb), ttb);

	/* The forward button */
	ttb->forward = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	gtk_widget_set_sensitive(GTK_WIDGET(ttb->forward), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ttb->forward, -1);
	g_signal_connect(G_OBJECT(ttb->forward), "clicked",
		G_CALLBACK(go_forward_cb), ttb);

	/* URL entry */
	item = gtk_tool_item_new();
	gtk_tool_item_set_expand(item, TRUE);
	//gtk_widget_set_size_request(urientry, 0, 20);
	ttb->urientry = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(item), ttb->urientry);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
	g_signal_connect(G_OBJECT(ttb->urientry), "activate",
		G_CALLBACK(uri_entry_cb), ttb);

	/* Separator --> 4-6px */
	item = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	/* Search entry */
	item = gtk_tool_item_new();
	gtk_tool_item_set_expand(item, FALSE);
	gtk_widget_set_size_request(GTK_WIDGET(item), 200, 0);
	ttb->search_entry = gtk_entry_new();

	gtk_entry_set_icon_from_stock(GTK_ENTRY(ttb->search_entry),
		GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_FIND);
	gtk_container_add(GTK_CONTAINER(item), ttb->search_entry);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
	g_signal_connect(GTK_ENTRY(ttb->search_entry), "icon-press",
		G_CALLBACK(search_icon_cb), ttb);
	g_signal_connect(G_OBJECT(ttb->search_entry), "activate",
		G_CALLBACK(search_entry_cb), ttb);

	/* Home button */
	ttb->home = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ttb->home, -1);
	g_signal_connect(G_OBJECT(ttb->home), "clicked",
		G_CALLBACK(go_home_cb), ttb);

	/* Bookmarks button */
	ttb->bookmarks = gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES);
	//gtk_widget_set_tooltip_text(GTK_WIDGET(item), "Bookmarks");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), ttb->bookmarks, -1);
	g_signal_connect(G_OBJECT(ttb->bookmarks), "clicked",
		G_CALLBACK(go_bookmarks_cb), ttb);

	return (toolbar);
}

void
close_tab(struct tab *ttb)
{
	if (ttb == NULL)
		return;

	TAILQ_REMOVE(&tabs, ttb, entry);
	if (TAILQ_EMPTY(&tabs))
		create_new_tab(NULL, 1);

	webkit_web_view_stop_loading(ttb->webview);
	gtk_widget_destroy(ttb->vbox);
	g_free(ttb);
}

gboolean
close_tab_cb(GtkWidget *event_box, GdkEventButton *event, struct tab *ttb)
{
	close_tab(ttb);
	return (FALSE);
}

void
create_new_tab(char *title, int focus)
{
	struct tab	*ttb;
	int	load = 1;
	char *newuri = NULL;
	GtkWidget *image, *hbox, *event_box;

	ttb = g_malloc0(sizeof *ttb);
	TAILQ_INSERT_TAIL(&tabs, ttb, entry);

	if (title == NULL) {
		title = "Empty";
		load = 0;
	}

	ttb->vbox = gtk_vbox_new(FALSE, 0);

	/* Tab title, spinner & close button */
	hbox = gtk_hbox_new(FALSE, 0);
	ttb->spinner = gtk_spinner_new();
	ttb->label = gtk_label_new(title);
	image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);

	event_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(event_box), image);

	gtk_widget_set_size_request(ttb->label, 160, -1);
	gtk_box_pack_start(GTK_BOX(hbox), ttb->spinner, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), ttb->label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), event_box, FALSE, FALSE, 0);

	/* Toolbar */
	ttb->toolbar = create_toolbar(ttb);
	gtk_box_pack_start(GTK_BOX(ttb->vbox), ttb->toolbar,
		FALSE, FALSE, 0);

	/* Browser */
	ttb->browser = create_browser(ttb);
	gtk_box_pack_start(GTK_BOX(ttb->vbox), ttb->browser, TRUE, TRUE, 0);

	/* Show all widgets */
	gtk_widget_show_all(hbox);
	gtk_widget_show_all(ttb->vbox);
	ttb->tab_id = gtk_notebook_append_page(notebook, ttb->vbox, hbox);

	/* Reorderable notebook tabs */
	gtk_notebook_set_tab_reorderable(notebook, ttb->vbox, TRUE);

	/* Close tab event */
	g_signal_connect(G_OBJECT(event_box), "button_press_event",
		G_CALLBACK(close_tab_cb), ttb);

	if (notoolbar)
		gtk_widget_hide(ttb->toolbar);

	if (focus) {
		gtk_notebook_set_current_page(notebook, ttb->tab_id);
	}

	if (load)
		webkit_web_view_load_uri(ttb->webview, title);
	else
		gtk_widget_grab_focus(GTK_WIDGET(ttb->urientry));

	if (newuri)
		free(newuri);
}

/* Main window */
GtkWidget *
create_window(void)
{
	GtkWidget *window;

	/* Default TazWeb window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	gtk_window_set_icon_name(GTK_WINDOW(window), "tazweb");
	gtk_widget_set_name(window, "TazWeb");
	gtk_window_set_wmclass(GTK_WINDOW(window), "tazweb", "TazWeb");
	g_signal_connect(window, "destroy", G_CALLBACK(destroy_cb), NULL);

	return (window);
}

void
create_canvas(void)
{
	GtkWidget *vbox;

	vbox = gtk_vbox_new(FALSE, 0);
	notebook = GTK_NOTEBOOK(gtk_notebook_new());

	/* Hide tabs */
	if (notoolbar)
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);

	gtk_notebook_set_scrollable(notebook, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(notebook), TRUE, TRUE, 0);

	tazweb_window = create_window();
	gtk_container_add(GTK_CONTAINER(tazweb_window), vbox);
	gtk_widget_show_all(tazweb_window);
}

int
main(int argc, char *argv[])
{
	int	focus = 1;

	while (argc > 1) {
		if (!strcmp(argv[1],"--notoolbar")) {
			notoolbar++;
		}
		else if (!strcmp(argv[1],"--kiosk")) {
			kiosk++;
		}
		else if (!strcmp(argv[1],"--useragent") && argc > 2) {
			argc--;
			argv++;
			useragent = argv[1];
		}
		else if (!strcmp(argv[1],"--help")) {
			printf ("Usage: tazweb [--notoolbar|--kiosk|--useragent] [ua]\n");
			printf ("Bookmarks: %s\n", BMTXT);
			return 0;
		}
		else break;
		argc--;
		argv++;
	}

	argc -= optind;
	argv += optind;

	/* FreeBSD style! */
	TAILQ_INIT(&tabs);

	/* Initialize GTK */
	gtk_init(&argc, &argv);
	create_canvas();

	/* Open all urls in a new tab */
	while (argc) {
		create_new_tab(argv[0], FALSE);
		focus = 0;
		argc--;
		argv++;
	}
	if (focus == 1)
		create_new_tab(WEBHOME, 1);

	gtk_main();

	return (0);
}
