/*
 * TazWeb is a radically simple web browser providing a single window
 * with a single toolbar with buttons, a URL entry and search as well
 * as a contextual menu, but no menu bar or tabs. Commented line code
 * starts with // and comments are between * *. IDE: Geany
 *
 * Copyright (C) 2011-2017 SliTaz GNU/Linux - BSD License
 * See AUTHORS and LICENSE for detailed information
 *
 */

#include <getopt.h>
#include <glib.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <webkit/webkit.h>
#include <libsoup/soup.h>

#define VERSION			"1.12"
#define GETTEXT_PACKAGE	"tazweb"
#define WEBHOME			"file:///usr/share/webhome/index.html"
#define SEARCH			"http://duckduckgo.com/?q=%s&t=slitaz"
#define HOME			g_get_home_dir()
#define CONFIG			g_strdup_printf("%s/.config/tazweb", HOME)
#define BOOKMARKS		g_strdup_printf("%s/bookmarks.txt", CONFIG)
#define COOKIES			g_strdup_printf("%s/cookies.txt", CONFIG)
#define DOWNLOADS		g_strdup_printf("%s/Downloads", HOME)

/* User agent string */
#define UA_TAZWEB		g_strdup_printf("TazWeb/%s (X11; SliTaz GNU/Linux)", VERSION)
#define UA_COMPAT		"Mozilla/5.0 AppleWebKit/535.22+"
#define UA				g_strdup_printf("%s %s", UA_TAZWEB, UA_COMPAT)

int		width			= 800;
int		height			= 600;
int		private			= 0;

static gchar			*useragent;
static gboolean		notoolbar;
static gboolean		nomenu;
static gboolean		kiosk;

static GtkWidget*		create_window(WebKitWebView** newwebview);
static GtkWidget		*tazweb_window, *vbox, *browser, *toolbar;
static WebKitWebView	*webview;
static WebKitWebFrame	*frame;
static SoupSession		*session;
static SoupCookieJar	*cookiejar;
static gint count		= 0;
const gchar*			uri;

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

/* Update title */
static void
update_title(GtkWindow* window, WebKitWebView* webview)
{
	GString *string;
	gdouble progress;
	gchar *title;

	string = g_string_new(webkit_web_view_get_title(webview));
	progress = webkit_web_view_get_progress(webview) * 100;
	if (progress < 100)
		g_string_append_printf(string, " [ %f%% ] ", progress);

	title = g_string_free(string, FALSE);
	gtk_window_set_title(window, title);
	g_free(title);
}

/* Get the page title */
static void
notify_title_cb(WebKitWebView* webview, GParamSpec* pspec, GtkWidget* window)
{
	update_title(GTK_WINDOW(window), webview);
}

/* Request progress in window title */
static void
notify_progress_cb(WebKitWebView* webview, GParamSpec* pspec, GtkWidget* window)
{
	update_title(GTK_WINDOW(window), webview);
}

/* Notify url entry */
static void
notify_load_status_cb(WebKitWebView* webview, GParamSpec* pspec, GtkWidget* urientry)
{
	if (webkit_web_view_get_load_status(webview) == WEBKIT_LOAD_COMMITTED) {
		frame = webkit_web_view_get_main_frame(webview);
		uri = webkit_web_frame_get_uri(frame);
		if (uri)
			gtk_entry_set_text(GTK_ENTRY(urientry), uri);
	}
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
view_source_cb(GtkWidget* widget, WebKitWebView* webview)
{
	gboolean source;

	frame = webkit_web_view_get_main_frame(webview);
	uri = webkit_web_frame_get_uri(frame);
	source = webkit_web_view_get_view_source_mode(webview);

	webkit_web_view_set_view_source_mode(webview, !source);
	webkit_web_view_reload(webview);
}

/* URL entry callback function */
static void
uri_entry_cb(GtkWidget* urientry, WebKitWebView* webview)
{
	uri = gtk_entry_get_text(GTK_ENTRY(urientry));
	g_assert(uri);
	check_requested_uri();
	webkit_web_view_load_uri(webview, uri);
}

/* Search entry and icon callback function */
static void
search_web(GtkWidget* search, WebKitWebView* webview)
{
	uri = g_strdup_printf(SEARCH, gtk_entry_get_text(GTK_ENTRY(search)));
	g_assert(uri);
	webkit_web_view_load_uri(webview, uri);
}

static void
search_entry_cb(GtkWidget* search, WebKitWebView* webview)
{
	search_web(search, webview);
}

static void
search_icon_press_cb(GtkWidget *search, GtkEntryIconPosition pos,
		GdkEvent *event, WebKitWebView* webview)
{
	search_web(search, webview);
}

/*
 *
 * Navigation functions
 *
 */

static void
bookmarks_edit_cb()
{
	system("/usr/lib/tazweb/helper.sh bookmarks_handler &");
}

static void
go_bookmarks_cb(GtkWidget* widget, WebKitWebView* webview)
{
	system("/usr/lib/tazweb/helper.sh html_bookmarks");
	uri = g_strdup_printf("file://%s/bookmarks.html", CONFIG);
	g_assert(uri);
	webkit_web_view_load_uri(webview, uri);
}

static void
go_home_cb(GtkWidget* widget, WebKitWebView* webview)
{
	uri = WEBHOME;
	g_assert(uri);
	webkit_web_view_load_uri(webview, uri);
}

static void
go_back_cb(GtkWidget* widget, WebKitWebView* webview)
{
	webkit_web_view_go_back(webview);
}

static void
go_forward_cb(GtkWidget* widget, WebKitWebView* webview)
{
	webkit_web_view_go_forward(webview);
}

/* Documentation callback */
static void
tazweb_doc_cb(GtkWidget* widget, WebKitWebView *webview)
{
	uri = ("file:///usr/share/doc/tazweb/tazweb.html");
	g_assert(uri);
	webkit_web_view_load_uri(webview, uri);
}

/* Download callback */
static gboolean
download_requested_cb(WebKitWebView *webview, WebKitDownload *download,
		gpointer user_data)
{
	const gchar* buffer;
	uri = webkit_download_get_uri(download);
	asprintf(&buffer, "xterm -T 'TazWeb Download' -geom 72x12+0-24 -e \
				'mkdir -p %s && wget -P %s -c %s; sleep 2' &",
				DOWNLOADS, DOWNLOADS, uri);
	system(buffer);
}

/* Printing callback function */
static void
print_page_cb (GtkWidget *widget, gpointer data)
{
	webkit_web_frame_print (webkit_web_view_get_main_frame (
		WEBKIT_WEB_VIEW (webview)));
}

/* New webview callbacks */
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

/* Add a bookmark */
void
add_bookmark_cb(GtkWidget *widget, gpointer data)
{
	const gchar* title;
	const gchar* buffer;

	title = webkit_web_view_get_title(WEBKIT_WEB_VIEW (webview));
	uri = webkit_web_view_get_uri(WEBKIT_WEB_VIEW (webview));

	asprintf(&buffer, "echo '%s|%s' >> %s", title, uri, BOOKMARKS);
	system(buffer);
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
cookies_clean_cb()
{
	system("/usr/lib/tazweb/helper.sh clean_cookies");
}

/* Add items to WebKit contextual menu */
static void
populate_menu_cb(WebKitWebView *webview, GtkMenu *menu, gpointer data)
{
	GtkWidget* item;

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Bookmark */
	item = gtk_image_menu_item_new_with_label(_("Bookmarks"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(go_bookmarks_cb), webview);

	if (! kiosk) {
		/* Add a bookmark */
		item = gtk_image_menu_item_new_with_label(_("Add a bookmark"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(item, "activate", G_CALLBACK(add_bookmark_cb), webview);

		/* Edit bookmarks */
		item = gtk_image_menu_item_new_with_label(_("Edit bookmarks"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(item, "activate", G_CALLBACK(bookmarks_edit_cb), NULL);

		/* Separator */
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}

	/* Printing */
	item = gtk_image_menu_item_new_with_label(_("Print this page"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_PRINT, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(print_page_cb), webview);

	/* View source mode */
	item = gtk_image_menu_item_new_with_label(_("View source mode"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(view_source_cb), webview);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	/* Cookies */
	if (! private) {
		item = gtk_image_menu_item_new_with_label(_("View cookies"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_stock(GTK_STOCK_HELP, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(item, "activate", G_CALLBACK(cookies_view_cb), webview);

		item = gtk_image_menu_item_new_with_label(_("Clean all cookies"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(item, "activate", G_CALLBACK(cookies_clean_cb), NULL);

		/* Separator */
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	}

	/* TazWeb documentation */
	item = gtk_image_menu_item_new_with_label(_("TazWeb manual"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_HELP, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(tazweb_doc_cb), webview);

	/* Quit TazWeb */
	item = gtk_image_menu_item_new_with_label(_("Quit TazWeb"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(destroy_cb), webview);

	gtk_widget_show_all(GTK_WIDGET(menu));
}

/* Scrolled window for the webview */
static GtkWidget*
create_browser(GtkWidget* window, GtkWidget* urientry, GtkWidget* search,
		WebKitWebView* webview)
{
	WebKitWebSettings *settings;

	browser = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(browser),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(browser), GTK_WIDGET(webview));

	/* Webkit settings */
	settings = webkit_web_view_get_settings (webview);
	if (! useragent)
		useragent = g_strdup_printf("%s", UA);
	g_object_set(G_OBJECT(settings), "user-agent", useragent, NULL);
	
	if (private)
		g_object_set(G_OBJECT(settings), "enable-private-browsing", TRUE);
	
	/* Connect Webkit events */
	g_signal_connect(webview, "notify::title",
			G_CALLBACK(notify_title_cb), window);
	g_signal_connect(webview, "notify::progress",
			G_CALLBACK(notify_progress_cb), window);
	g_signal_connect(webview, "notify::load-status",
			G_CALLBACK(notify_load_status_cb), urientry);
	g_signal_connect(webview, "web-view-ready",
			G_CALLBACK(webview_ready_cb), window);
	g_signal_connect(webview, "close-web-view",
			G_CALLBACK(close_webview_cb), window);

	/* Impossible to open in new window or download in kiosk mode */
	if (! kiosk) {
		g_signal_connect(webview, "download-requested",
			G_CALLBACK(download_requested_cb), NULL);
		g_signal_connect(webview, "create-web-view",
			G_CALLBACK(create_web_view_cb), window);
	}

	if (! nomenu) {
		/* Connect WebKit contextual menu items */
		g_object_connect(G_OBJECT(webview), "signal::populate-popup",
			G_CALLBACK(populate_menu_cb), webview, NULL);
	}
	return browser;
}

static GtkWidget*
create_toolbar(GtkWidget* urientry, GtkWidget* search, WebKitWebView* webview)
{
	GtkToolItem* item;

	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar),
			GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),
			GTK_TOOLBAR_BOTH_HORIZ);

	/* The back button */
	item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_BACK);
	g_signal_connect(G_OBJECT(item), "clicked",
			G_CALLBACK(go_back_cb), webview);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	/* The forward button */
	item = gtk_tool_button_new_from_stock(GTK_STOCK_GO_FORWARD);
	g_signal_connect(G_OBJECT(item), "clicked",
			G_CALLBACK(go_forward_cb), webview);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	/* URL entry */
	item = gtk_tool_item_new();
	gtk_tool_item_set_expand(item, TRUE);
	gtk_container_add(GTK_CONTAINER(item), urientry);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
	g_signal_connect(G_OBJECT(urientry), "activate",
			G_CALLBACK(uri_entry_cb), webview);

	/* Separator */
	item = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	/* Search entry */
	item = gtk_tool_item_new();
	gtk_tool_item_set_expand(item, FALSE);
	gtk_widget_set_size_request(GTK_WIDGET(item), 200, 0);
	gtk_container_add(GTK_CONTAINER(item), search);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(search),
			GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_FIND);
	g_signal_connect(GTK_ENTRY(search), "icon-press",
			G_CALLBACK(search_icon_press_cb), webview);
	g_signal_connect(G_OBJECT(search), "activate",
			G_CALLBACK(search_entry_cb), webview);

	/* Home button */
	item = gtk_tool_button_new_from_stock(GTK_STOCK_HOME);
	gtk_widget_set_tooltip_text(GTK_WIDGET(item), "Home page");
	g_signal_connect(G_OBJECT(item), "clicked",
			G_CALLBACK(go_home_cb), webview);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	/* Bookmark button */
	item = gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES);
	gtk_widget_set_tooltip_text(GTK_WIDGET(item), "Bookmarks");
	g_signal_connect(G_OBJECT(item), "clicked",
			G_CALLBACK(go_bookmarks_cb), webview);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, -1);

	return toolbar;
}

/* Main window */
static GtkWidget*
create_window(WebKitWebView** newwebview)
{
	GtkWidget *window;
	GtkWidget *urientry;
	GtkWidget *search;

	g_atomic_int_inc(&count);

	/* Default TazWeb window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), width, height);
	gtk_window_set_icon_name(GTK_WINDOW(window), "tazweb");
	gtk_widget_set_name(window, "TazWeb");
	gtk_window_set_wmclass(GTK_WINDOW(window), "tazweb", "TazWeb");
	g_signal_connect(window, "destroy", G_CALLBACK(destroy_cb), NULL);

	/* Webview and widgets */
	webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
	urientry = gtk_entry_new();
	search = gtk_entry_new();
	vbox = gtk_vbox_new(FALSE, 0);

	/* Pack box and container */
	if (! notoolbar)
		gtk_box_pack_start(GTK_BOX(vbox),
			create_toolbar(urientry, search, webview), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox),
			create_browser(window, urientry, search, webview), TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);

	if (newwebview)
		*newwebview = webview;

	return window;
}

/* Cmdline Help & usage */
void
help(void)
{
	printf("\nTazWeb - Light and fast web browser using Webkit engine\n\n\
Usage: tazweb [--options] [value] url\n\
\n\
Options:\n\
  -h  --help            Print TazWeb command line help\n\
  -p  --private         Disable on-disk cache, cookies, history\n\
  -u  --useragent [ua]  Configure the user agent string\n\
  -k  --kiosk           Fullscreen, no bookmarks and download support\n\
  -r  --raw             Raw webkit window without toolbar and menu\n\
  -s  --small           Small Tazweb window for tiny web applications\n\
      --notoolbar       Disable the top toolbar\n\
      --nomenu          Disable TazWeb contextual menu\n\n");
    
	return;
}

int
main(int argc, char *argv[])
{
	textdomain (GETTEXT_PACKAGE);
	int c;

	/* Cmdline parsing with getopt_long to handle --option or -o */
	while (1) {
		static struct option long_options[] =
		{
			/* Set flag */
			{ "notoolbar",  no_argument,		&notoolbar, 1 },
			{ "nomenu",     no_argument,		&nomenu,    1 },
			/* No flag */
			{ "help",       no_argument,		0, 'h' },
			{ "private",    no_argument,		0, 'p' },
			{ "useragent",  required_argument,	0, 'u' },
			{ "kiosk",      no_argument,		0, 'k' },
			{ "raw",        no_argument,		0, 'r' },
			{ "small",		no_argument,		0, 's' },
			{ 0, 0, 0, 0}
		};

		int index = 0;
		c = getopt_long (argc, argv, "hpu:krs", long_options, &index);

		/* Detect the end of the options */
		if (c == -1)
			break;

		switch (c) {
			case 0:
				/* Options with flag */
				break;

			case 'h':
				help();
				return 0;

			case 'p':
				private++;
				break;

			case 'u':
				useragent = optarg;
				break;

			case 'k':
				kiosk++;
				break;

			case 'r':
				notoolbar++;
				nomenu++;
				break;

			case 's':
				width = 640;
				height = 480;
				break;

			default:
				help();
				return 0;
		}
	}

	argc -= optind;
	argv += optind;


	/* Initialize GTK */
	gtk_init(NULL, NULL);

	/* Get a default bookmarks.txt if missing */
	if (! g_file_test(BOOKMARKS, G_FILE_TEST_EXISTS)) {
		system("install -m 0700 -d $HOME/.config/tazweb");
		system("install -m 0600 /usr/share/tazweb/bookmarks.txt \
			$HOME/.config/tazweb/bookmarks.txt");
	}

	/* Load the start page or the url in argument */
	uri =(char*)(argc == 1 ? argv[0] : WEBHOME);
	if (argv[0])
		check_requested_uri();

	tazweb_window = create_window(&webview);
	gtk_widget_show_all(tazweb_window);

	/* Handle cookies */
	if (! private) {
		session = webkit_get_default_session();
		snprintf(COOKIES, sizeof COOKIES, "%s", COOKIES);
		cookies_setup();
	}

	/* Fullscreen for Kiosk mode */
	if (kiosk)
		gtk_window_fullscreen(GTK_WINDOW(tazweb_window));

	webkit_web_view_load_uri(webview, uri);
	gtk_widget_grab_focus(GTK_WIDGET(webview));
	gtk_main();

	return 0;
}
