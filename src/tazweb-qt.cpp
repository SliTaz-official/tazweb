/*
 * TazWeb-Qt is a radically simple web browser providing a single window.
 * Commented line code starts with // and comments are between * *
 *
 * Copyright (C) 2011-2017 SliTaz GNU/Linux - BSD License
 * See AUTHORS and LICENSE for detailed information
 * 
 */
#include <QtGui>
#include <QtWebKit>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);
	QApplication::setWindowIcon(QIcon::fromTheme("tazweb"));
	QFile file(QDir::homePath() + "/.config/slitaz/subox.conf");
	QString msg, line;
	QString msg2("\n ENTER/ok -> tazpanel, ESC/cancel -> bookmarks/webhome");
	QUrl url;
	if (argc > 1) { url = QUrl::fromUserInput(argv[1]); }
	else {
		if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			msg = "Using subox pass... Load successfully" + msg2;
			QTextStream in(&file);
			while (!in.atEnd()) { line = in.readLine(); }
			file.close(); }
		else {	msg = file.fileName() + " not found.\nroot password requested:" + msg2;
			line = "root"; }
		bool ok;
		QString text = QInputDialog::getText(0, "TazWeb-Qt: TazPanel authentication",
			msg, QLineEdit::Password, line, &ok);
		if (ok && !text.isEmpty()) {
			QApplication::setWindowIcon(QIcon::fromTheme("tazpanel"));
			url = QUrl("http://root:" + text + "@tazpanel:82"); }
		else {
			if (QFile::exists(QDir::homePath() + "/.config/tazweb/bookmarks.txt"))
			url = QUrl("http://localhost/cgi-bin/bookmarks.cgi?home=" + QDir::homePath());
			else
			url = QUrl("file:///usr/share/webhome/index.html"); }
	}
	QWebView view;
/*
	view.show();
	//view.setUrl(QUrl("file:///usr/share/webhome/index.html"));
	view.load(QUrl("file:///usr/share/webhome/index.html"));
*/	
	//view.settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
	//view.settings()->setAttribute(QWebSettings::ZoomTextOnly, true);
	//view.setTextSizeMultiplier(1);
	view.showMaximized();
	view.load(url);
	return app.exec();
}
