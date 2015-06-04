
mactoolsdir = ${libdir}/djview/mactools

mactools_LTLIBRARIES = mdimporter.la

mdimporter_la_SOURCES = main.c GetMetadataForFile.m

mdimporter_la_CFLAGS = ${DDJVUAPI_CFLAGS}
mdimporter_la_LIBADD = ${DDJVUAPI_LIBS} 
mdimporter_la_LDFLAGS = -module -avoid-version \
   -framework System -framework CoreFoundation \
   -framework CoreServices -framework Foundation

