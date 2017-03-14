/*
 * TazWeb code snippets: small regions of re-usable source code
 * 
 * This file is used for ideas, proposals and to keep removed code under
 * the hand.
 *
 * Copyright (C) 2011-2017 SliTaz GNU/Linux - BSD License
 * See AUTHORS and LICENSE for detailed information
 *
 */

/* Cmdline parsing with getopt to handle -p -s */
while ((c = getopt(argc, argv, "psk:u")) != -1) {
	switch (c) {
		case 'p':
			notoolbar++;
			break;
		
		case 's':
			nocookies++;
			break;
		
		case 'k':
			kiosk++;
			break;
		
		case 'u':
			useragent = optarg;
			break;
		
		default:
			help();
	}
}

/* 
 * 
 * Callback
 * 
 * 
 **********************************************************************/

/* Zoom out and in callback function
 * 
 * Not sure zooming is useful in menu, but with GDK keybinding or
 * mouse Ctrl+scroll it would be nice!
 * 
 */

static void
zoom_out_cb(GtkWidget *widget, WebKitWebView* webview)
{
	webkit_web_view_zoom_out(webview);
}

static void
zoom_in_cb(GtkWidget *widget, WebKitWebView* webview)
{
	webkit_web_view_zoom_in(webview);
}

/* Fullscreen and unfullscreen callback function 
 * 
 * Buggy: first window is going fullscreen
 * 
 */

static void
fullscreen_cb(GtkWidget* window, gpointer data)
{
	GdkWindowState state;
	state = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(mainwindow)));

	if(state & GDK_WINDOW_STATE_FULLSCREEN)
		gtk_window_unfullscreen(GTK_WINDOW(mainwindow));
	else
		gtk_window_fullscreen(GTK_WINDOW(mainwindow));
}

/* Add items to WebKit contextual menu */
static void
populate_menu_cb(WebKitWebView *webview, GtkMenu *menu, gpointer data)
{
	
	/* Zoom in */
	item = gtk_image_menu_item_new_with_label(_("Zoom in"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_ZOOM_IN, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(zoom_in_cb), webview);

	/* Zoom out */
	item = gtk_image_menu_item_new_with_label(_("Zoom out"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_ZOOM_OUT, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(zoom_out_cb), webview);
	
	/* Fullscreen */
	item = gtk_image_menu_item_new_with_label(_("Fullscreen mode"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
	gtk_image_new_from_stock(GTK_STOCK_FULLSCREEN, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(item, "activate", G_CALLBACK(fullscreen_cb), webview);

	/* Separator */
	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

}
