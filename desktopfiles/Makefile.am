#icons

SVGICONS = \
scalable/mimetypes/djvulibre-djview4.svgz

PNGICONS = \
32x32/mimetypes/djvulibre-djview4.png \
64x64/mimetypes/djvulibre-djview4.png 

iconsdir = $(datadir)/icons/hicolor

nobase_icons_DATA = $(PNGICONS) $(SVGICONS)

EXTRA_DIST = \
  djvu.svg \
  djview.svg \
  prebuilt-hi32-djview4.png \
  prebuilt-hi64-djview4.png

if HAVE_CONVERSION_RSVG
convert_icons_process = \
s=`echo $@ | sed -e 's/[a-z]*\([0-9]*\).*/\1/'`; \
${RSVG} \
-w $${s} \
-h $${s} -o $@ $<
endif

if HAVE_CONVERSION_INKSCAPE
convert_icons_process = \
s=`echo $@ | sed -e 's/[a-z]*\([0-9]*\).*/\1/'`; \
${INKSCAPE} \
--without-gui \
--export-width=$${s} \
--export-height=$${s} \
--export-png=$@ $<
endif

if HAVE_CONVERSION_CONVERT
convert_icons_process = \
s=`echo $@ | sed -e 's/[a-z]*\([0-9]*\).*/\1/'`; \
${CONVERT} \
-geometry $${s}x$${s} \
-depth 8 \
-background none $< $@
endif

png_verbose = $(png_verbose_@AM_V@)
png_verbose_ = $(png_verbose_@AM_DEFAULT_V@)
png_verbose_0 = @echo "  PNG     " $@;

${PNGICONS}: djview.svg Makefile
	$(AM_V_at)$(MKDIR_P) `dirname $@`
if HAVE_CONVERSION_TOOL
	$(png_verbose)$(convert_icons_process)
else
	$(AM_V_at)s=`echo $@ | sed -e 's/[a-z]*\([0-9]*\).*/\1/'`; \
	  cp ${top_srcdir}/desktopfiles/prebuilt-hi$${s}-djview4.png $@
endif

svg_process = gzip --no-name <$< >$@

svg_verbose = $(svg_verbose_@AM_V@)
svg_verbose_ = $(svg_verbose_@AM_DEFAULT_V@)
svg_verbose_0 = @echo "  SVG     " $@;

$(SVGICONS): djview.svg Makefile
	$(AM_V_at)$(MKDIR_P) `dirname $@`
	$(AM_V_at)rm -f $@
	$(svg_verbose)$(svg_process)

# install desktop file into application dir

applicationsdir = $(datadir)/applications
dist_applications_DATA = djvulibre-djview4.desktop

# clean

MOSTLYCLEANFILES = $(PNGICONS) $(SVGICONS) 

mostlyclean-local:
	$(AM_V_at)for n in $(PNGICONS) $(SVGICONS) ; do m=`dirname "$$n"` ; \
	    while test "$$m" != "." ; do rmdir $$m ; m=`dirname "$$m"` ; done ; \
	done

