# Note: plugindir is set in configure
plugin_LTLIBRARIES = libgstthrottle.la libgstsplitter.la

# sources used to compile this plug-in
libgstthrottle_la_SOURCES = gstthrottle.c gstthrottle.h
libgstsplitter_la_SOURCES = gstsplitter.c gstsplitter.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstthrottle_la_CFLAGS = $(GST_CFLAGS)
libgstthrottle_la_LIBADD = $(GST_LIBS)
libgstthrottle_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstthrottle_la_LIBTOOLFLAGS = --tag=disable-static

libgstsplitter_la_CFLAGS = $(GST_CFLAGS)
libgstsplitter_la_LIBADD = $(GST_LIBS)
libgstsplitter_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstsplitter_la_LIBTOOLFLAGS = --tag=disable-static


# headers we need but don't want installed
noinst_HEADERS = gstthrottle.h gstsplitter.h
