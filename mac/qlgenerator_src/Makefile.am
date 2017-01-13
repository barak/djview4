
mactoolsdir = ${libdir}/djview/mactools

mactools_LTLIBRARIES = qlgenerator.la

qlgenerator_la_SOURCES = \
  GeneratePreviewForURL.m GenerateThumbnailForURL.m main.c

qlgenerator_la_OBJCFLAGS = -w
qlgenerator_la_CFLAGS = ${DDJVUAPI_CFLAGS}
qlgenerator_la_LIBADD = ${DDJVUAPI_LIBS} 

qlgenerator_la_LDFLAGS = -module -avoid-version \
  -framework System -framework CoreFoundation -framework CoreServices \
  -framework Foundation -framework Cocoa -framework QuickLook

