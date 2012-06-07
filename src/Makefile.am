# Note: plugindir is set in configure
plugin_LTLIBRARIES = libgstthrottle.la

# sources used to compile this plug-in
libgstthrottle_la_SOURCES = gstthrottle.c gstthrottle.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstthrottle_la_CFLAGS = $(GST_CFLAGS)
libgstthrottle_la_LIBADD = $(GST_LIBS)
libgstthrottle_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstthrottle_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstthrottle.h
