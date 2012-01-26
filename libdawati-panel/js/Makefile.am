mainjsdir = $(pkgdatadir)/js/
mainjs_DATA = \
	path.js \
	$(NULL)

BUILT_SOURCES = path.js

do_subst = sed -e 's|@abs_top_srcdir[@]|$(abs_top_srcdir)|g' \
               -e 's|@abs_top_builddir[@]|$(abs_top_builddir)|g' \
               -e 's|@localedir[@]|$(localedir)|g' \
               -e 's|@bindir[@]|$(bindir)|g' \
               -e 's|@libdir[@]|$(libdir)|g' \
               -e 's|@libexecdir[@]|$(libexecdir)|g' \
               -e 's|@pkglibdir[@]|$(pkglibdir)|g' \
               -e 's|@pkgdatadir[@]|$(pkgdatadir)|g'

path.js: Makefile path.js.in
	$(AM_V_GEN) $(do_subst) $(srcdir)/path.js.in > $@

CLEANFILES = \
    $(BUILT_SOURCES)

EXTRA_DIST = \
    path.js.in
