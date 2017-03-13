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

export TEXTDOMAIN='tazweb-lib'

# Parse cmdline options and store values in a variable
for opt in "$@"; do
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
	cat << EOT
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="utf-8" />
	<title>$title</title>
	<style type="text/css">
		body { margin: 2% 10%; font-size: 92%; } ul { padding: 0; }
		h1 { color: #ccc; border-bottom: 2px solid #ccc; }
		ul a { text-decoration: none; } ul a:hover { text-decoration: underline; }
		li { list-style-type: none; line-height: 1.4em; padding: 0; }
		footer { font-size: 80%; border-top: 2px solid #ccc; padding: 5px 0; color: #888; }
	</style>
</head>
<body>
<section id="content">
<h1>$title</h1>
EOT
}

# HTML 5 footer: html_footer content
html_footer() {
	cat << EOT
</section>
<footer>
	${@}
</footer>
</body>
</html>
EOT
}

# Generate bookmarks.html
html_bookmarks() {
	html_header "$(gettext 'Bookmarks')" > ${bm_html}
	echo '<ul>' >> ${bm_html}
	IFS="|"
	cat ${bm_txt} | while read title url null
	do
		cat >> ${bm_html} << EOT
	<li><a href="${url}">${title}</a></li>
EOT
	done; unset IFS
	echo '</ul>' >> ${bm_html}
	html_footer "$(cat $bm_txt | wc -l) $(gettext "Bookmarks") - $(date)" \
		>> ${bm_html}
	# Security fix from old cgi-bin bookmarks.cgi
	chown ${USER}.${USER} ${bm_txt}; chmod 0600 ${bm_txt}
}

edit_bookmarks() {
	yad --text-info \
		--center --width=640 --height=480 --filename=${bm_txt}
}

# Generate cookies.html (for direct view of cookies in TazWeb)
html_cookies() {
	html_header "$(gettext 'Cookies')" > ${cookies_html}
	echo '<pre>' >> ${cookies_html}
	IFS="|"
	cat ${cookies_txt} | while read line
	do
		cat >> ${cookies_html} << EOT
${line#\#HttpOnly_}
EOT
	done; unset IFS
	echo '</pre>' >> ${cookies_html}
	html_footer $(cat $cookies_txt | wc -l) $(gettext "Cookies") - $(date) \
		>> ${cookies_html}
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
	
	*) grep "[a-z]_*()" ${0} | awk '{print $1}' ;;

esac; exit 0
