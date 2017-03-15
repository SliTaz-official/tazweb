#!/bin/sh
#
# TazWeb Helper - Handle bookmarks and cookies
#
# Coding: No libtaz.sh and so it is usable on any Linux distro
#
# Copyright (C) 2017 SliTaz GNU/Linux - BSD License
# See AUTHORS and LICENSE for detailed information
#

config="$HOME/.config/tazweb"
bm_txt="$config/bookmarks.txt"
bm_html="$config/bookmarks.html"
cookies_txt="$config/cookies.txt"
cookies_html="$config/cookies.html"

export TEXTDOMAIN='tazweb'

# Parse cmdline options and store values in a variable
for opt in $@; do
	opt_name="${opt%%=*}"
	opt_name="${opt_name#--}"
	case "$opt" in
		--*=*)		export  $opt_name="${opt#*=}" ;;
		--*)		export  $opt_name="on" ;;
	esac
done

# HTML 5 header with built-in minimal CSS. Usage: html_header "title"
html_header() {
	local title="$1"
	cat <<EOT
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<title>$title</title>
	<style>
		body { margin: 2% 10%; font-size: 92%; }
		h1 { color: #CCC; border-bottom: 2px solid #CCC; }
		ul { padding: 0; }
		ul a { text-decoration: none; } ul a:hover { text-decoration: underline; }
		li { list-style-type: none; line-height: 1.4em; padding: 0; }
		footer { font-size: 80%; border-top: 2px solid #CCC; padding: 5px 0; color: #888; }
	</style>
</head>
<body>
	<section id="content">
		<h1>$title</h1>
EOT
}

# HTML 5 footer: html_footer content
html_footer() {
	cat <<EOT
	</section>
	<footer>
		$@
	</footer>
</body>
</html>
EOT
}

# Generate bookmarks.html
html_bookmarks() {
	{
		html_header "$(gettext 'Bookmarks')"
		echo '<ul>'

		IFS="|"
		while read title url null; do
			echo "<li><a href=\"$url\">$title</a></li>"
		done < $bm_txt
		unset IFS

		echo '</ul>'
		num=$(wc -l < $bm_txt)
		html_footer "$(printf "$(ngettext "%d bookmark" "%d bookmarks" "$num")" "$num") - $(date)"
	} > $bm_html

	# Security fix from old cgi-bin bookmarks.cgi
	chown $USER:$USER $bm_txt; chmod 0600 $bm_txt
}

edit_bookmarks() {
	yad --text-info \
		--center --width=640 --height=480 --filename=$bm_txt
}

# Generate cookies.html (for direct view of cookies in TazWeb)
html_cookies() {
	{
		html_header "$(gettext 'Cookies')"
		echo '<pre>'

		IFS="|"
		while read line; do
			echo "${line#\#HttpOnly_}"
		done < $cookies_txt
		unset IFS

		echo '</pre>'
		num=$(wc -l < $cookies_txt)
		html_footer "$(printf "$(ngettext "%d cookie" "%d cookies" "$num")" "$num") - $(date)"
	} > $cookies_html
}

clean_cookies() {
	> $cookies_txt
}


#
# Execute any shell_function
#
case "$1" in

	*_*)
		cmd=$1; shift; $cmd $@ ;;

	*) grep "[a-z]_*()" $0 | awk '{print $1}' ;;

esac
exit 0
