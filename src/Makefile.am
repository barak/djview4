
# include data gathered from djview.pro file

include djview.am

djview.am: djview.pro
	$(AWK) -f $(top_srcdir)/config/qmsources.awk < $< > $@

# program djview
# - can we do better than pretending djview is a script?

bin_SCRIPTS = djview

djview: Makefile.qmake FORCE
	${MAKE} -f Makefile.qmake djview

FORCE: 

.PHONY: FORCE

EXTRA_DIST = images Doxyfile djview.pro $(qmake_sources)

# man page

dist_man1_MANS = djview.1

# translations
# - Maybe we should do them all at once and avoid listing 
#   the translation files in both djview.pro and Makefile.am

translationsdir = $(datadir)/djvu

translations_DATA = $(qmake_qm)

EXTRA_DIST += $(qmake_ts)

lrelease_verbose = $(lrelease_verbose_@AM_V@)
lrelease_verbose_ = $(lrelease_verbose_@AM_DEFAULT_V@)
lrelease_verbose_0 = @

$(qmake_qm): qm.dir/lrelease-stamp
	$(lrelease_verbose)cmp -s qm.dir/$@ $@ || cp qm.dir/$@ $@

qm.dir/lrelease-stamp: $(qmake_ts) djview
	$(lrelease_verbose)test -d qm.dir || $(MKDIR) qm.dir ; \
	 for ts in $(qmake_ts); do cp $(srcdir)/$$ts qm.dir; done ; \
	 for ts in $(qmake_ts); do echo "TRANSLATIONS += $$ts" ; done > qm.dir/ts.pro
	$(lrelease_verbose)cd qm.dir && $(LRELEASE) ts.pro
	$(lrelease_verbose)touch $@

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
		VPATH='$(srcdir)' -o - djview.pro |\
	  $(AWK) -f $(top_srcdir)/config/qmsilent.awk > $@

# cleaning

mostlyclean-local: mostlyclean-qmake mostlyclean-qm

mostlyclean-qmake:
	-test "${srcdir}" == "." || ${RM} djview.pro
	-test -f Makefile.qmake && ${MAKE} -f Makefile.qmake clean
	-$(RM) Makefile.qmake

mostlyclean-qm:
	-${RM} *.qm qm.dir/*
	-${RMDIR} qm.dir


.PHONY: mostlyclean-qmake mostlyclean-qm 

MOSTLYCLEANFILES = djview


