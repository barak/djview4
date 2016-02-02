
# include data gathered from djview.pro file

include $(srcdir)/djview.am

$(srcdir)/djview.am: djview.pro
	$(AWK) -f $(top_srcdir)/config/qmsources.awk < $< > $@

# program djview
# - can we do better than pretending djview is a script?

bin_SCRIPTS = djview

djview: Makefile.qmake FORCE
	${MAKE} -f Makefile.qmake

FORCE: 

.PHONY: FORCE

EXTRA_DIST = images Doxyfile djview.pro $(qmake_sources)

# man page

dist_man1_MANS = djview.1

# translations
# - Maybe we should do them all at once and avoid listing 
#   the translation files in both djview.pro and Makefile.am

translationsdir = $(datadir)/djvu/djview4

translations_DATA = $(qmake_qm)

EXTRA_DIST += $(qmake_ts)

lrelease_verbose = $(lrelease_verbose_@AM_V@)
lrelease_verbose_ = $(lrelease_verbose_@AM_DEFAULT_V@)
lrelease_verbose_0 = @echo "  LRELEASE" $@;

$(qmake_qm): djview

SUFFIXES=.ts .qm

.ts.qm:
	$(lrelease_verbose)$(LRELEASE) -silent $< -qm $@

# invoke qmake

qmake_verbose = $(qmake_verbose_@AM_V@)
qmake_verbose_ = $(qmake_verbose_@AM_DEFAULT_V@)
qmake_verbose_0 = @echo "  QMAKE   " $@;

qmake_allflags = $(QMAKEFLAGS) -I.. -I$(srcdir) -DDIR_DATADIR=\\\"$(datadir)\\\"

Makefile.qmake: ${srcdir}/djview.pro Makefile
	@cmp -s $< djview.pro || cp $< djview.pro
	$(qmake_verbose)$(QMAKE) $(QMAKEARG)\
		QMAKE_UIC=$(UIC) \
		QMAKE_MOC=$(MOC) \
		QMAKE_RCC=$(RCC) \
		QMAKE_LFLAGS+='$(LDFLAGS)' \
		QMAKE_CFLAGS+='$(CFLAGS) $(qmake_allflags)' \
		QMAKE_CXXFLAGS+='$(CXXFLAGS) $(qmake_allflags)' \
		QMAKE_CPPFLAGS+='$(CPPFLAGS) $(qmake_allflags)' \
		LIBS+='$(QMAKELIBS)' \
		VPATH='$(srcdir)' -o $@0 djview.pro
	$(AWK) -f $(top_srcdir)/config/qmsilent.awk < $@0 > $@ && ${RM} $@0

# cleaning

mostlyclean-local: mostlyclean-qmake

mostlyclean-qmake:
	-test "${srcdir}" == "." || ${RM} djview.pro
	-test -f Makefile.qmake && ${MAKE} -f Makefile.qmake clean
	-$(RM) Makefile.qmake0
	-$(RM) Makefile.qmake

.PHONY: mostlyclean-qmake mostlyclean-qm 

MOSTLYCLEANFILES = djview $(qmake_qm)


