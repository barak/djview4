
# plugin

pluginsdir = ${libdir}/mozilla/plugins

plugins_LTLIBRARIES = nsdejavu.la

nsdejavu_la_SOURCES = nsdejavu.c 
nsdejavu_la_LDFLAGS = -module -rpath ${pluginsdir} -avoid-version
nsdejavu_la_CPPFLAGS = -DDIR_BINDIR=\"$(bindir)\" -I${srcdir} -I${srcdir}/npsdk
nsdejavu_la_CFLAGS = $(OPTS) $(NSDEJAVU_CFLAGS)
nsdejavu_la_LIBADD = $(NSDEJAVU_LIBS)

EXTRA_DIST = npsdk README COPYING

# man page

man1_MANS = nsdejavu.1

EXTRA_DIST += nsdejavu.1.in

sed_process = eval "nsdejavu=`cat nsdejavu.x`" && $(SED) \
 -e "s,PLUGINSDIR,${pluginsdir}," \
 -e "s,NSDEJAVUSO,$${nsdejavu}," \
 < $< > $@ || (rm $@; exit 1)

man_verbose = $(man_verbose_@AM_V@)
man_verbose_ = $(man_verbose_@AM_DEFAULT_V@)
man_verbose_0 = @echo "  MAN     " $@;

nsdejavu.x: nsdejavu.la
	@eval `grep '^dlname=' nsdejavu.la` && \
	  echo $${dlname} | sed -e 's/\([-.][0-9][0-9]*\)*//g' > $@ 
	-@if test -z "`cat nsdejavu.x`" ; then \
	   echo "-----------------------------------------------";\
	   echo "*** Libtool cannot build the plugin 'nsdejavu'.";\
	   echo "*** Did you configure with '--disable-shared'? ";\
	   echo "-----------------------------------------------";\
	  fi

nsdejavu.1: nsdejavu.1.in nsdejavu.x Makefile
	$(AM_V_at)rm -f $@
	$(man_verbose)$(sed_process)

MOSTLYCLEANFILES = nsdejavu.1 nsdejavu.x
