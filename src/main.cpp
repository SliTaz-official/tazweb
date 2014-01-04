/*
 * TazWeb-Qt is a radically simple web browser providing a single window.
 * Commented line code starts with // and comments are between * *
 *
 * Copyright (C) 2011-2014 SliTaz GNU/Linux - BSD License
 * See AUTHORS and LICENSE for detailed information
 * 
 */
#include <QtGui>
#include <QtWebKit>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	QWebView view;
	view.show();
	//view.setUrl(QUrl("file:///usr/share/webhome/index.html"));
	view.load(QUrl("file:///usr/share/webhome/index.html"));
	return app.exec();
}
