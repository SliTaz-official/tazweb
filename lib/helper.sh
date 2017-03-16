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
	<link rel="stylesheet" href="/usr/share/doc/tazweb/style.css">
</head>
<body>
	<header>
		<h1>$1</h1>
	</header>
	<main>
EOT
}

# HTML 5 footer: html_footer content
html_footer() {
	cat <<EOT
	</main>
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
		done < ${bm_txt}
		unset IFS

		echo '</ul>'
		num=$(wc -l < $bm_txt)
		html_footer "$(printf "$(ngettext "%d bookmark" "%d bookmarks" "$num")" "$num") - $(date)"
	} > ${bm_html}

	# Security fix from old cgi-bin bookmarks.cgi
	chown $USER:$USER ${bm_txt}; chmod 0600 ${bm_txt}
}

# List all bookmarks
bookmarks_list() {
	text=
	cat ${bm_txt} | while read title url; do
		echo -e "$title\n$url"
	done | yad --list \
		--title="$(gettext 'TazWeb Bookmarks')" \
		--text="$(gettext 'Click on a value to edit - Right click to remove a bookmark')\n" \
		--text-align=center \
		--mouse --width=640 --height=480 \
		--skip-taskbar \
		--window-icon=/usr/share/icons/hicolor/32x32/apps/tazweb.png \
		--editable --print-all \
		--tooltip-column=2 \
		--search-column=1 \
		--column="$(gettext 'Title')" \
		--column="$(gettext 'URL')"
}

# Rebuild bookmarks.txt since some entries may have been edited and remove
# selected (TRUE) entries.
bookmarks_handler() {
	IFS="|"
	bookmarks_list | while read title url null; do
		echo "$title|$url" >> ${bm_txt}.tmp
	done; unset IFS
	if [ -f "${bm_txt}.tmp" ]; then
		mv -f ${bm_txt}.tmp ${bm_txt}
	fi
}

# Generate cookies.html (for direct view of cookies in TazWeb)
html_cookies() {
	{
		html_header "$(gettext 'Cookies')"
		echo '<pre style="overflow: auto;">'

		while read line; do
			echo "${line#\#HttpOnly_}"
		done < ${cookies_txt}

		echo '</pre>'
		num=$(wc -l < $cookies_txt)
		html_footer "$(printf "$(ngettext "%d cookie" "%d cookies" "$num")" "$num") - $(date)"
	} > ${cookies_html}
}

clean_cookies() {
	rm ${cookies_txt}; touch ${cookies_txt}
}

#
# Execute any shell_function
#
case "$1" in

	*_*)
		cmd=${1}; shift; ${cmd} ${@} ;;

	*) grep "[a-z]_*()" $0 | awk '{print $1}' ;;

esac
exit 0
