#!/bin/sh
#
# TazWeb Bookmarks CGI handler
# Copyright (C) 2014 SliTaz GNU/Linux - BSD License
#
. /usr/lib/slitaz/httphelper.sh

script="$SCRIPT_NAME"
home="$(GET home)"
user="$(basename $home)"
config="/home/$user/.config/tazweb"
bookmarks="$config/bookmarks.txt"

# Security check
if [ "$REMOTE_ADDR" != "127.0.0.1" ]; then
	echo "Security exit" && exit 1
fi

# HTML 5 header with built-in minimal CSS
html_header() {
	cat << EOT
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="utf-8" />
	<title>TazWeb - Bookmarks</title>
	<style type="text/css">
		body { margin: 2% 10%; } .rm { color: #666; } ul { padding: 0; }
		.rm:hover { text-decoration: none; color: #B70000; }
		h1 { color: #666; border-bottom: 4px solid #666; }
		a { text-decoration: none; } a:hover { text-decoration: underline; }
		li { list-style-type: none; color: #666; line-height: 1.4em; padding: 0; }
		footer { font-size: 80%; border-top: 2px solid #666; padding: 5px 0; }
	</style>
</head>
<body>
<section id="content">

EOT
}

# HTML 5 footer
html_footer() {
	cat << EOT

</section>

<footer>
	<a href="$script?home=$home">Bookmarks</a> -
	<a href="$script?raw&amp;home=$home">bookmarks.txt</a>
</footer>

</body>
</html>
EOT
}

# Handle GET actions: continue or exit

case " $(GET) " in
	*\ raw\ *)
		# View bookmarks file
		header
		html_header
		echo "<h1>TazWeb: bookmarks.txt</h1>"
		echo "<pre>" 
		cat ${bookmarks}
		echo "</pre>"
		html_footer && exit 0 ;;
	*\ rm\ *)
		# Remove a bookmark item and continue
		url=$(GET rm)
		[ "$url" ] || continue
		sed -i s"#.*${url}.*##" ${bookmarks}
		sed -i "/^$/"d ${bookmarks} ;;
esac

# Show all bookmarks
header
html_header
echo '<h1>TazWeb Bookmarks</h1>'
echo '<ul>'
IFS="|"
cat ${bookmarks} | while read title url null
do
	cat << EOT
	<li><a class="rm" href="?rm=$url&amp;home=$home">&otimes;<a/>
	<a href="${url}">${title}<a/></li>
EOT
done
unset IFS
echo '</ul>'
html_footer

exit 0
