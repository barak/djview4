
# program djview
# - can we do better than pretending djview is a script?

bin_SCRIPTS = djview

djview: Makefile.qmake FORCE
	${MAKE} -f Makefile.qmake djview

FORCE: 

.PHONY: FORCE

EXTRA_DIST = \
	images \
	Doxyfile \
	djview.pro \
	djview.rc \
	djview.plist \
	djview.cpp\
	djview.h\
	qdjviewauthdialog.ui\
	qdjview.cpp\
	qdjviewdialogs.cpp\
	qdjviewdialogs.h\
	qdjviewerrordialog.ui\
	qdjviewexportdialog.ui\
	qdjviewexporters.cpp\
	qdjviewexporters.h\
	qdjviewexportprn.ui\
	qdjviewexportps1.ui\
	qdjviewexportps2.ui\
	qdjviewexportps3.ui\
	qdjviewexporttiff.ui\
	qdjview.h\
	qdjviewinfodialog.ui\
	qdjviewmetadialog.ui\
	qdjviewplugin.cpp\
	qdjviewplugin.h\
	qdjviewprefs.cpp\
	qdjviewprefsdialog.ui\
	qdjviewprefs.h\
	qdjviewprintdialog.ui\
	qdjview.qrc\
	qdjviewsavedialog.ui\
	qdjviewsidebar.cpp\
	qdjviewsidebar.h\
	qdjvu.cpp\
	qdjvu.h\
	qdjvunet.cpp\
	qdjvunet.h\
	qdjvuwidget.cpp\
	qdjvuwidget.h\
	qdjvuwidget.qrc\
	tiff2pdf.c\
	tiff2pdf.h\
	version.h

# man page

dist_man1_MANS = djview.1

# translations
# - Maybe we should do them all at once and avoid listing 
#   the translation files in both djview.pro and Makefile.am

translationsdir = $(datadir)/djvu

translations_DATA = djview_cs.qm djview_de.qm djview_es.qm
translations_DATA += djview_fr.qm djview_ru.qm djview_uk.qm
translations_DATA += djview_zh_cn.qm djview_zh_tw.qm

EXTRA_DIST += djview_cs.ts djview_de.ts djview_es.ts 
EXTRA_DIST += djview_fr.ts djview_ru.ts djview_uk.ts
EXTRA_DIST += djview_zh_cn.ts djview_zh_tw.ts

lrelease_verbose = $(lrelease_verbose_@AM_V@)
lrelease_verbose_ = $(lrelease_verbose_@AM_DEFAULT_V@)
lrelease_verbose_0 = @echo "  QM      " $@;

lrelease_process = ( trap "$(RM) $@.tmp/* ; $(RMDIR) $@.tmp" 0; \
  $(MKDIR) $@.tmp && cp $< $@.tmp && \
  echo "TRANSLATIONS="`basename $<` > $@.tmp/trans.pro && \
  ( cd $@.tmp && $(LRELEASE) trans.pro ) && \
  ( cmp -s $@.tmp/$@ $@ || cp $@.tmp/$@ $@ ) )

.ts.qm:
	$(lrelease_verbose)$(lrelease_process)

SUFFIXES = .qm .ts

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

clean-local: clean-qmake clean-qm

distclean-local: distclean-qmake clean-qm

clean-qm:
	-${RM} *.qm 
	-${RMDIR} *.qm.tmp/*
	-${RMDIR} *.qm.tmp

clean-qmake:
	-test "${srcdir}" == "." || ${RM} djview.pro
	-test -f Makefile.qmake && ${MAKE} -f Makefile.qmake clean

distclean-qmake: Makefile.qmake
	-test "${srcdir}" == "." || ${RM} djview.pro
	-test -f Makefile.qmake && ${MAKE} -f Makefile.qmake distclean
	-$(RM) Makefile.qmake

.PHONY: clean-qm clean-qmake distclean-qmake 

MOSTLYCLEANFILES = djview


