//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006-  Leon Bottou
//C-
//C- This software is subject to, and may be distributed under, the
//C- GNU General Public License, either version 2 of the license,
//C- or (at your option) any later version. The license should have
//C- accompanied the software or you may obtain a copy of the license
//C- from the Free Software Foundation at http://www.fsf.org .
//C-
//C- This program is distributed in the hope that it will be useful,
//C- but WITHOUT ANY WARRANTY; without even the implied warranty of
//C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//C- GNU General Public License for more details.
//C-  ------------------------------------------------------------------

#if AUTOCONF
# include "config.h"
#endif

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QLineEdit>
#include <QMetaEnum>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QPointer>
#include <QRect>
#include <QRegExpValidator>
#include <QSettings>
#include <QStringList>
#include <QTranslator>
#include <QVariant>
#include <QWhatsThis>

#include "qdjviewprefs.h"
#include "qdjview.h"
#include "qdjvu.h"
#include "djview.h"



// ========================================
// QDJVIEWPREFS
// ========================================


/*! \class QDjViewPrefs
   \brief Holds the preferences for the \a QDjView instances. */

/*! \enum QDjViewPrefs::Option
   \brief Miscellanenous flags.
   This enumeration defines flags describing 
   (i) the visibility of the menubar, toolbar, etc.
   (ii) the page layout flags continuous and side by side.
   (iii) the interactive capabilities of the djvu widget. */

/*! \enum QDjViewPrefs::Tool
   \brief Toolbar flags.
   This enumeration defines flags describing
   the composition of the toolbar. */

/*! \class QDjViewPrefs::Saved
   \brief Mode dependent preferences.
   This structure holds the preferences that are 
   saved separately for each viewer mode.
   There are four copies of this structure,
   for the standalone viewer mode, for the full screen mode,
   for the full page plugin mode, and for the embedded plugin mode. */


/*! The default toolbar composition. */

const QDjViewPrefs::Tools 
QDjViewPrefs::defaultTools 
= TOOLBAR_TOP | TOOLBAR_BOTTOM
| TOOL_ZOOMCOMBO | TOOL_ZOOMBUTTONS
| TOOL_PAGECOMBO | TOOL_PREVNEXT | TOOL_FIRSTLAST 
| TOOL_BACKFORW
| TOOL_OPEN | TOOL_FIND | TOOL_SELECT
| TOOL_PRINT | TOOL_LAYOUT
| TOOL_WHATSTHIS;


/*! The default set of miscellaneous flags. */

const QDjViewPrefs::Options
QDjViewPrefs::defaultOptions 
= SHOW_MENUBAR | SHOW_TOOLBAR | SHOW_SIDEBAR 
| SHOW_STATUSBAR | SHOW_SCROLLBARS 
| SHOW_FRAME | SHOW_MAPAREAS
| HANDLE_MOUSE | HANDLE_KEYBOARD | HANDLE_LINKS 
| HANDLE_CONTEXTMENU;


static QPointer<QDjViewPrefs> preferences;


/*! Returns the single preference object. */

QDjViewPrefs *
QDjViewPrefs::instance(void)
{
  QMutex mutex;
  QMutexLocker locker(&mutex);
  if (! preferences)
    {
      preferences = new QDjViewPrefs();
      preferences->load();
    }
  return preferences;
}


QDjViewPrefs::Saved::Saved(void)
  : remember(true),
    options(defaultOptions),
    zoom(QDjVuWidget::ZOOM_FITWIDTH)
{
}


QDjViewPrefs::QDjViewPrefs(void)
  : QObject(QCoreApplication::instance()),
    windowSize(640,400),
    tools(defaultTools),
    gamma(2.2),
    white(QColor(Qt::white)),
    resolution(100),
    cacheSize(10*1024*1024),
    pixelCacheSize(256*1024),
    lensSize(300),
    lensPower(3),
    enableAnimations(true),
    advancedFeatures(false),
    invertLuminance(false),
    mouseWheelZoom(false),
    modifiersForLens(Qt::ControlModifier|Qt::ShiftModifier),
    modifiersForSelect(Qt::ControlModifier),
    modifiersForLinks(Qt::ShiftModifier),
    thumbnailSize(64),
    thumbnailSmart(true),
    searchWordsOnly(true),
    searchCaseSensitive(false),
    searchRegExpMode(false),
    infoDialogTab(0),
    metaDialogTab(0),
    printerGamma(0.0),
    printReverse(false),
    printCollate(true)
{
  Options mss = (SHOW_MENUBAR|SHOW_STATUSBAR|SHOW_SIDEBAR);
  forFullScreen.options &= ~(mss|SHOW_SCROLLBARS|SHOW_TOOLBAR|SHOW_SIDEBAR);
  forFullPagePlugin.options &= ~(mss);
  forEmbeddedPlugin.options &= ~(mss|SHOW_TOOLBAR);
  forEmbeddedPlugin.remember = false;
}


/*! Return a string with the djview version. */

QString 
QDjViewPrefs::versionString()
{
  QString version = QString("%1.%2");
  version = version.arg((DJVIEW_VERSION>>16)&0xFF);
  version = version.arg((DJVIEW_VERSION>>8)&0xFF);
  if (DJVIEW_VERSION & 0xFF)
    version = version + QString(".%1").arg(DJVIEW_VERSION&0xFF);
  return version;
}


/*! Return a string describing a set of modifier keys. */

QString 
QDjViewPrefs::modifiersToString(Qt::KeyboardModifiers m)
{
  QStringList l;
#ifdef Q_WS_MAC
  if (m & Qt::MetaModifier)
    l << "Control";
  if (m & Qt::AltModifier)
    l << "Alt";
  if (m & Qt::ControlModifier)
    l << "Command";
#else
  if (m & Qt::MetaModifier)
    l << "Meta";
  if (m & Qt::ControlModifier)
    l << "Control";
  if (m & Qt::AltModifier)
    l << "Alt";
#endif
  if (m & Qt::ShiftModifier)
    l << "Shift";
  return l.join("+");
}

/*! Return the modifiers described in string \a s. */

Qt::KeyboardModifiers 
QDjViewPrefs::stringToModifiers(QString s)
{
  Qt::KeyboardModifiers m = Qt::NoModifier;
  QStringList l = s.split("+");
  foreach(QString key, l)
    {
      key = key.toLower();
      if (key == "shift")
        m |= Qt::ShiftModifier;
#ifdef Q_WS_MAC
      else if (key == "control")
        m |= Qt::MetaModifier;
      else if (key == "command")
        m |= Qt::ControlModifier;
#else
      else if (key == "meta")
        m |= Qt::MetaModifier;
      else if (key == "control")
        m |= Qt::ControlModifier;
#endif
      else if (key == "alt")
        m |= Qt::AltModifier;
    }
  return m;
}


/*! Return a string describing a set of option flags. */

QString 
QDjViewPrefs::optionsToString(Options options)
{
  const QMetaObject *mo = metaObject();
  QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("Option"));
  return me.valueToKeys(static_cast<int>(options));
}

/*! Return the option flags described by a string. */

QDjViewPrefs::Options 
QDjViewPrefs::stringToOptions(QString s)
{
  const QMetaObject *mo = metaObject();
  QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("Option"));
  int options = me.keysToValue(s.toLatin1());
  return QFlag(options);
}


/*! Return a string describing a set of toolbar flags. */

QString 
QDjViewPrefs::toolsToString(Tools tools)
{
  const QMetaObject *mo = metaObject();
  QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("Tool"));
  return me.valueToKeys(tools);
}


/*! Return the toolbar flags described by a string. */

QDjViewPrefs::Tools   
QDjViewPrefs::stringToTools(QString s)
{
  const QMetaObject *mo = metaObject();
  QMetaEnum me = mo->enumerator(mo->indexOfEnumerator("Tool"));
  int tools = me.keysToValue(s.toLatin1());
  return QFlag(tools);
}


void 
QDjViewPrefs::loadGroup(QSettings &s, QString name, Saved &saved)
{
  s.beginGroup(name);
  if (s.contains("remember"))
    saved.remember = s.value("remember").toBool();
  if (s.contains("options"))
    saved.options = stringToOptions(s.value("options").toString());
  if (s.contains("zoom"))
    saved.zoom = s.value("zoom").toInt();
  if (s.contains("state"))
    saved.state = s.value("state").toByteArray();
  s.endGroup();
  // we always want these options
  saved.options |= (QDjViewPrefs::HANDLE_MOUSE |
                    QDjViewPrefs::HANDLE_KEYBOARD |
                    QDjViewPrefs::HANDLE_LINKS );
  
}


/*! Load saved preferences and set all member variables. */

void
QDjViewPrefs::load()
{
  QSettings s;

  int versionFix = 0;
  if (s.contains("versionFix"))
    versionFix = s.value("versionFix").toInt();

  loadGroup(s, "forEmbeddedPlugin", forEmbeddedPlugin);
  loadGroup(s, "forFullPagePlugin", forFullPagePlugin);
  loadGroup(s, "forStandalone", forStandalone);
  loadGroup(s, "forFullScreen", forFullScreen);
  
  if (s.contains("windowSize"))
    windowSize = s.value("windowSize").toSize();
  if (s.contains("tools"))
    tools = stringToTools(s.value("tools").toString());
  if (versionFix < 0x40200)
    tools |= TOOL_BACKFORW;
  if (s.contains("gamma"))
    gamma = s.value("gamma").toDouble();
  if (s.contains("white"))
    white = QColor(s.value("white").toString());
  if (s.contains("resolution"))
    resolution = s.value("resolution").toInt();
  if (s.contains("cacheSize"))
    cacheSize = s.value("cacheSize").toLongLong();
  if (s.contains("pixelCacheSize"))
    pixelCacheSize = s.value("pixelCacheSize").toInt();
  if (s.contains("lensSize"))
    lensSize = s.value("lensSize").toInt();
  if (s.contains("lensPower"))
    lensPower = s.value("lensPower").toInt();
  if (s.contains("browserProgram"))
    browserProgram = s.value("browserProgram").toString();
  if (s.contains("proxyUrl"))
    proxyUrl = s.value("proxyUrl").toString();
  if (s.contains("enableAnimations"))
    enableAnimations = s.value("enableAnimations").toBool();
  if (s.contains("advancedFeatures"))
    advancedFeatures = s.value("advancedFeatures").toBool();
  if (s.contains("invertLuminance"))
    invertLuminance = s.value("invertLuminance").toBool();
  if (s.contains("mouseWheelZoom"))
    mouseWheelZoom = s.value("mouseWheelZoom").toBool();
  if (s.contains("language"))
    languageOverride = s.value("language").toString();
  if (s.contains("modifiersForLens"))
    modifiersForLens 
      = stringToModifiers(s.value("modifiersForLens").toString());
  if (s.contains("modifiersForSelect"))
    modifiersForSelect 
      = stringToModifiers(s.value("modifiersForSelect").toString());
  if (s.contains("modifiersForLinks"))
    modifiersForLinks 
      = stringToModifiers(s.value("modifiersForLinks").toString());
  
  if (s.contains("thumbnailSize"))
    thumbnailSize = s.value("thumbnailSize").toInt();
  if (s.contains("thumbnailSmart"))
    thumbnailSmart = s.value("thumbnailSmart").toBool();

  if (s.contains("searchWordsOnly"))
    searchWordsOnly = s.value("searchWordsOnly").toBool();
  if (s.contains("searchCaseSensitive"))
    searchCaseSensitive = s.value("searchCaseSensitive").toBool();
  if (s.contains("searchRegExpMode"))
    searchRegExpMode = s.value("searchRegExpMode").toBool();
  
  if (s.contains("infoDialogTab"))
    infoDialogTab = s.value("infoDialogTab").toInt();
  if (s.contains("metaDialogTab"))
    metaDialogTab = s.value("metaDialogTab").toInt();
  if (s.contains("printerGamma"))
    printerGamma = s.value("printerGamma").toDouble();
  if (s.contains("printerName"))
    printerName = s.value("printerName").toString();
  if (s.contains("printFile"))
    printFile = s.value("printFile").toString();
  if (s.contains("printCollate"))
    printCollate = s.value("printCollate").toBool();
  if (s.contains("printReverse"))
    printCollate = s.value("printReverse").toBool();

  if (s.contains("recentFiles"))
    recentFiles = s.value("recentFiles").toStringList();
}


void 
QDjViewPrefs::saveGroup(QSettings &s, QString name, Saved &saved)
{
  s.beginGroup(name);
  s.setValue("remember", saved.remember);
  s.setValue("options", optionsToString(saved.options));
  s.setValue("zoom", saved.zoom);
  s.setValue("state", saved.state);
  s.endGroup();
}


/*! Save all member variables for later use. */

void
QDjViewPrefs::save(void)
{
  QSettings s;
  
  s.setValue("version", versionString());

  int versionFix = 0;
  if (s.contains("versionFix"))
    versionFix = s.value("versionFix").toInt();
  if (DJVIEW_VERSION > versionFix)
    s.setValue("versionFix", DJVIEW_VERSION);
  
  saveGroup(s, "forEmbeddedPlugin", forEmbeddedPlugin);
  saveGroup(s, "forFullPagePlugin", forFullPagePlugin);
  saveGroup(s, "forStandalone", forStandalone);
  saveGroup(s, "forFullScreen", forFullScreen);

  s.setValue("windowSize", windowSize);
  s.setValue("tools", toolsToString(tools));
  s.setValue("gamma", gamma);
  s.setValue("white", white.name());
  s.setValue("resolution", resolution);
  s.setValue("cacheSize", cacheSize);
  s.setValue("pixelCacheSize", pixelCacheSize);
  s.setValue("lensSize", lensSize);
  s.setValue("lensPower", lensPower);
  s.setValue("browserProgram", browserProgram);
  s.setValue("proxyUrl", proxyUrl.toString());
  s.setValue("enableAnimations", enableAnimations);
  s.setValue("advancedFeatures", advancedFeatures);
  s.setValue("invertLuminance", invertLuminance);
  s.setValue("mouseWheelZoom", mouseWheelZoom);
  s.setValue("modifiersForLens", modifiersToString(modifiersForLens));
  s.setValue("modifiersForSelect", modifiersToString(modifiersForSelect));
  s.setValue("modifiersForLinks", modifiersToString(modifiersForLinks));
  s.setValue("language", languageOverride);

  s.setValue("thumbnailSize", thumbnailSize);
  s.setValue("thumbnailSmart", thumbnailSmart);

  s.setValue("searchWordsOnly", searchWordsOnly);
  s.setValue("searchCaseSensitive", searchCaseSensitive);
  s.setValue("searchRegExpMode", searchRegExpMode);
  
  s.setValue("infoDialogTab", infoDialogTab);
  s.setValue("metaDialogTab", metaDialogTab);
  s.setValue("printerGamma", printerGamma);
  s.setValue("printerName", printerName);
  s.setValue("printFile", printFile);
  s.setValue("printCollate", printCollate);
  s.setValue("printReverse", printReverse);

  s.setValue("recentFiles", recentFiles);
}



/*! Emit the updated() signal. */

void
QDjViewPrefs::update()
{
  emit updated();
}


/*! \fn QDjViewPrefs::updated() 
  This signal indicates that viewers should reload the preferences. */







// ========================================
// QDJVIEWGAMMAWIDGET
// ========================================


class QDjViewGammaWidget : public QFrame
{
  Q_OBJECT
public:
  QDjViewGammaWidget(QWidget *parent = 0);
  double gamma() const;
public slots:
  void setGamma(double);
  void setGammaTimesTen(int);
protected:
  virtual void paintEvent(QPaintEvent *event);
private:
  void paintRect(QPainter &painter, QRect r, bool strip);
  double g;
};


QDjViewGammaWidget::QDjViewGammaWidget(QWidget *parent)
  : QFrame(parent), g(2.2)
{
  setMinimumSize(64,64);
  setFrameShape(QFrame::Panel);
  setFrameShadow(QFrame::Sunken);
  setLineWidth(1);
}


double 
QDjViewGammaWidget::gamma() const
{
  return g;
}


void 
QDjViewGammaWidget::setGamma(double gamma)
{
  g = qBound(0.3, gamma, 5.0);
  update();
}


void 
QDjViewGammaWidget::setGammaTimesTen(int gamma)
{
  setGamma((double)gamma/10);
}


void 
QDjViewGammaWidget::paintEvent(QPaintEvent *event)
{
  int w = width() - 16;
  int h = height() - 16;
  if (w >= 8 && h >= 8)
    {
      QPoint c = rect().center();
      QPainter painter;
      painter.begin(this);
      int m = qMin( w, h ) / 4;
      m = m + m;
      QRect r(c.x()-m, c.y()-m, m, m);
      painter.fillRect(rect(), Qt::white);
      paintRect(painter, r.translated(m,0), true);
      paintRect(painter, r.translated(0,m+1), true);
      paintRect(painter, r.translated(m,m), false);
      paintRect(painter, r, false);
      painter.end();
      QFrame::paintEvent(event);
    }
}


void 
QDjViewGammaWidget::paintRect(QPainter &painter, QRect r, bool strip)
{
  if (strip)
    {
      // draw stripes
      painter.setPen(QPen(Qt::black, 1));
      painter.setRenderHint(QPainter::Antialiasing, false);
      for(int i=r.top(); i<=r.bottom(); i+=2)
        painter.drawLine(r.left(), i, r.right(), i);
    }
  else
    {
      // see DjVuImage.cpp
      double gamma = g;
      if (gamma < 0.1)
        gamma = 0.1;
      else if (gamma > 10)
        gamma = 10;
      // see GPixmap.cpp
      double x = ::pow(0.5, 1.0/gamma);
      int gray = (int)floor(255.0 * x + 0.5);
      // fill
      gray = qBound(0, gray, 255);
      painter.fillRect(r, QColor(gray,gray,gray));
    }
}




// ========================================
// QDJVIEWMODIFIERSCOMBOBOX
// ========================================


class QDjViewModifiersComboBox : public QComboBox
{
  Q_OBJECT
  Q_PROPERTY(Qt::KeyboardModifiers value READ value WRITE setValue)
public:
  QDjViewModifiersComboBox(QWidget *parent);
  Qt::KeyboardModifiers value() const;
public slots:
  void setValue(Qt::KeyboardModifiers m);
  void insertValue(Qt::KeyboardModifiers m);
protected slots:
  void editingFinished();
private:
  static const char *none;
};

const char* QDjViewModifiersComboBox::none = "None";


QDjViewModifiersComboBox::QDjViewModifiersComboBox(QWidget *parent)
  : QComboBox(parent)
{
  setEditable(true);
  setInsertPolicy(QComboBox::NoInsert);
  // validator
  QStringList keynames;
  keynames << "Shift";
  keynames << "Control";
  keynames << "Alt";
#ifdef Q_WS_MAC
  keynames << "Command";
#else
  keynames << "Meta";
#endif  
  QString keys = "(" + keynames.join("|") + ")";
  keys = keys + "(\\+" + keys + ")*";
  keys = keys + "|" + none;
  QRegExp re( keys, Qt::CaseInsensitive);
  setValidator(new QRegExpValidator(re, this));
  // menu entries
  insertValue(Qt::ShiftModifier);
  insertValue(Qt::ControlModifier);
  insertValue(Qt::AltModifier);
  insertValue(Qt::MetaModifier);
  insertValue(Qt::ShiftModifier|Qt::ControlModifier);
  insertValue(Qt::ControlModifier|Qt::AltModifier);
  setValue(Qt::NoModifier);
  connect(lineEdit(), SIGNAL(editingFinished()),
          this, SLOT(editingFinished()) );
}


void 
QDjViewModifiersComboBox::editingFinished()
{
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  QString text = lineEdit()->text();
  Qt::KeyboardModifiers m = prefs->stringToModifiers(text);
  text = (m) ? prefs->modifiersToString(m) : none;
  setEditText(text);
}


Qt::KeyboardModifiers
QDjViewModifiersComboBox::value() const
{
  const_cast<QDjViewModifiersComboBox*>(this)->editingFinished();
  QString text = lineEdit()->text();
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  return prefs->stringToModifiers(text);
}


void 
QDjViewModifiersComboBox::setValue(Qt::KeyboardModifiers m)
{
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  QString text = (m) ? prefs->modifiersToString(m) : none;
  setEditText(text);
}


void 
QDjViewModifiersComboBox::insertValue(Qt::KeyboardModifiers m)
{
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  QString text = (m) ? prefs->modifiersToString(m) : none;
  QVariant vm(m);
  if (findData(vm) < 0)
    insertItem(count(), text, vm);
}



// ========================================
// QDJVIEWPREFSDIALOG
// ========================================


#include "ui_qdjviewprefsdialog.h"


static QPointer<QDjViewPrefsDialog> prefsDialog;


/*! Returns the single preference dialog. */

QDjViewPrefsDialog *
QDjViewPrefsDialog::instance()
{
  QMutex mutex;
  QMutexLocker locker(&mutex);
  if (! prefsDialog)
    {
      QDjViewPrefsDialog *main = new QDjViewPrefsDialog();
      main->setWindowTitle(tr("Preferences[*] - DjView"));
      main->setAttribute(Qt::WA_DeleteOnClose, true);
      main->setAttribute(Qt::WA_QuitOnClose, false);
      prefsDialog = main;
    }
  return prefsDialog;
}


struct QDjViewPrefsDialog::Private
{
  QDjVuContext          *context;
  Ui::QDjViewPrefsDialog ui;
  QDjViewPrefs::Saved    saved[4];
  int                    currentSaved;
};


QDjViewPrefsDialog::~QDjViewPrefsDialog()
{
  delete d;
}


static inline void
setHelp(QWidget *w, QString s)
{
  w->setWhatsThis(s);
}


QDjViewPrefsDialog::QDjViewPrefsDialog()
  : QDialog(), d(new Private)
{
  // sanitize
  d->context = 0;
  d->currentSaved = -1;
  
  // prepare ui
  setAttribute(Qt::WA_GroupLeader, true);
  d->ui.setupUi(this);
  
  // connect buttons (some are connected in the ui file)
  connect(d->ui.applyButton, SIGNAL(clicked()),
          this, SLOT(apply()) );
  connect(d->ui.gammaSlider, SIGNAL(valueChanged(int)),
          d->ui.gammaWidget, SLOT(setGammaTimesTen(int)) );
  connect(d->ui.forceResolutionCheckBox, SIGNAL(toggled(bool)),
          this, SLOT(forceResolutionChanged()) );
  connect(d->ui.resetButton, SIGNAL(clicked()),
          this, SLOT(reset()) );
  connect(d->ui.cacheClearButton, SIGNAL(clicked()),
          this, SLOT(cacheClear()) );
  connect(d->ui.modeComboBox, SIGNAL(activated(int)),
          this, SLOT(modeComboChanged(int)) );
  connect(d->ui.zoomCombo->lineEdit(), SIGNAL(editingFinished()),
          this, SLOT(zoomComboEdited()) );

  // modified
  connectModified(d->ui.screenTab);
  connectModified(d->ui.rememberCheckBox);
  connectModified(d->ui.showGroupBox);
  connectModified(d->ui.modeGroupBox);
  connectModified(d->ui.keysTab);
  connectModified(d->ui.lensTab);
  connectModified(d->ui.networkTab);
  connectModified(d->ui.advancedTab);
  
  // whatsthis
  setHelp(d->ui.screenTab,
          tr("<html><b>Screen gamma correction.</b>"
             "<br>The best color rendition is achieved"
             " by adjusting the gamma correction slider"
             " and choosing the position that makes the"
             " gray square as uniform as possible."
             "<p><b>Screen resolution.</b>"
             "<br>This option forces a particular resolution instead of using"
             " the unreliable resolution advertised by the operating system."
             " Forcing the resolution to 100 dpi matches the behavior"
             " of the djvulibre command line tools."
             "</html>") );
  
  setHelp(d->ui.interfaceTab,
          tr("<html><b>Initial interface setup.</b>"
             "<br>DjView can run as a standalone viewer,"
             " as a full screen viewer, as a full page browser plugin,"
             " or as a plugin embedded inside a html page."
             " For each case, check the <tt>Remember</tt> box to"
             " automatically save and restore the interface setup."
             " Otherwise, specify an initial configuration."
             "</html>") );

  setHelp(d->ui.keysTab,
          tr("<html><b>Modifiers keys.</b>"
             "<br>Define which combination of modifier keys"
             " will show the manifying lens, temporarily enable"
             " the selection mode, or highlight the hyperlinks."
             "</html>") );

  setHelp(d->ui.lensTab,
          tr("<html><b>Magnifying lens.</b>"
             "<br>The magnifying lens appears when you depress"
             " the modifier keys specified in tab <tt>Keys</tt>."
             " This panel lets you choose the power and the size"
             " of the magnifying lens."
             "</html>") );
  
  setHelp(d->ui.advancedTab,
          tr("<html><b>Caches.</b>"
             "<br>The <i>pixel cache</i> stores image data"
             " located outside the visible area."
             " This cache makes panning smoother."
             " The <i>decoded page cache</i> contains partially"
             " decoded pages. It provides faster response times"
             " when navigating a multipage document or when returning"
             " to a previously viewed page. Clearing this cache"
             " might be useful to reflect a change in the page"
             " data without restarting the DjVu viewer."

             "<p><b>Miscellaneous.</b>"
             "<br>Forcing a manual color correction can be useful"
             " when using ancient printers."
             " The advanced features check box enables a small number"
             " of additional menu entries useful for authoring DjVu files."
             "</html>"));

  setHelp(d->ui.networkTab,
          tr("<html><b>Network proxy settings.</b>"
             "<br>These proxy settings are used when"
             " the standalone djview viewer accesses"
             " a djvu document through a http url."
             " The djview plugin always uses the proxy"
             " settings of the web browser."
             "</html>") );
}


void
QDjViewPrefsDialog::connectModified(QWidget *w)
{
  if (w->inherits("QComboBox"))
    connect(w, SIGNAL(editTextChanged(QString)), this, SLOT(setModified()));
  if (w->inherits("QComboBox"))
    connect(w, SIGNAL(currentIndexChanged(int)), this, SLOT(setModified()));
  if (w->inherits("QSpinBox"))
    connect(w, SIGNAL(valueChanged(int)), this, SLOT(setModified()));
  if (w->inherits("QAbstractButton"))
    connect(w, SIGNAL(toggled(bool)), this, SLOT(setModified()));
  if (w->inherits("QAbstractSlider"))
    connect(w, SIGNAL(valueChanged(int)), this, SLOT(setModified()));
  QObject *child;
  foreach(child, w->children())
    if (child->isWidgetType())
      connectModified(qobject_cast<QWidget*>(child));
}

void
QDjViewPrefsDialog::load(QDjView *djview)
{
  // load preferences into ui
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  d->context = &djview->getDjVuContext();
  // 1- screen tab
  d->ui.gammaSlider->setValue((int)(prefs->gamma * 10));
  int res = prefs->resolution;
  d->ui.forceResolutionCheckBox->setChecked(res > 0);
  d->ui.resolutionSpinBox->setValue((res>0) ? res : logicalDpiY());
  d->ui.invertLuminanceCheckBox->setChecked(prefs->invertLuminance);
  // 2- interface tab
  d->saved[0] = prefs->forStandalone;
  d->saved[1] = prefs->forFullScreen;
  d->saved[2] = prefs->forFullPagePlugin;
  d->saved[3] = prefs->forEmbeddedPlugin;
  djview->fillZoomCombo(d->ui.zoomCombo);
  int n = 0;
  if (djview->getViewerMode() == QDjView::EMBEDDED_PLUGIN)
    n = 3;
  else if (djview->getViewerMode() == QDjView::FULLPAGE_PLUGIN)
    n = 2;
  else if (djview->getFullScreen()) 
    n = 1;
  modeComboChanged(n);
  d->ui.modeComboBox->setCurrentIndex(n);
  // 3- keys tab
  d->ui.keysForLensCombo->setValue(prefs->modifiersForLens);
  d->ui.keysForSelectCombo->setValue(prefs->modifiersForSelect);
  d->ui.keysForLinksCombo->setValue(prefs->modifiersForLinks);
  d->ui.wheelScrollButton->setChecked(! prefs->mouseWheelZoom);
  d->ui.wheelZoomButton->setChecked(prefs->mouseWheelZoom);
  // 4- lens tab
  bool lens = (prefs->lensPower > 0) && (prefs->lensSize > 0);
  d->ui.lensEnableCheckBox->setChecked(lens);
  d->ui.lensPowerSpinBox->setValue(lens ? qBound(1,prefs->lensPower,10) : 3);
  d->ui.lensSizeSpinBox->setValue(lens ? qBound(50,prefs->lensSize,500) : 300);
  // 5- network tab
  bool proxy = prefs->proxyUrl.isValid();
  if (prefs->proxyUrl.scheme() != "http" ||
      prefs->proxyUrl.host().isEmpty())
    proxy = false;
  if (proxy)
    {
      QUrl url = prefs->proxyUrl;
      int port = url.port();
      d->ui.proxyHostLineEdit->setText(url.host());
      d->ui.proxyPortSpinBox->setValue(port>=0 ? port : 8080);
      d->ui.proxyUserLineEdit->setText(url.userName());
      d->ui.proxyPasswordLineEdit->setText(url.password());
    }
  d->ui.proxyCheckBox->setChecked(proxy);
  // 6- advanced tab
  qreal k256 = 256 * 1024;
  qreal k1024 = 1024 * 1024;
  d->ui.pixelCacheSpinBox->setValue(qRound(prefs->pixelCacheSize / k256));
  d->ui.pageCacheSpinBox->setValue(qRound(prefs->cacheSize / k1024));
  double pgamma = prefs->printerGamma;
  loadLanguageComboBox(prefs->languageOverride);
  d->ui.printerManualCheckBox->setChecked(pgamma > 0);
  d->ui.printerGammaSpinBox->setValue((pgamma > 0) ? pgamma : 2.2);
  d->ui.animationCheckBox->setChecked(prefs->enableAnimations);
  d->ui.advancedCheckBox->setChecked(prefs->advancedFeatures);
  // no longer modified
  setWindowModified(false);
  d->ui.applyButton->setEnabled(false);
}


void
QDjViewPrefsDialog::loadLanguageComboBox(QString lang)
{
  // supported languages
  static const char *languages[] = { 
    "cs","de","en","fr", "it", "ja","ru","uk", "zh", 0 } ;
  // get application
  QComboBox *cb = d->ui.languageComboBox;
  QCheckBox *db = d->ui.languageCheckBox;
  QDjViewApplication *app = qobject_cast<QDjViewApplication*>(qApp);
  db->setEnabled(app != 0);
  cb->clear();
  // prepare combobox
  static struct { const char *src; const char *com; } thisLang[] = {
    QT_TRANSLATE_NOOP3("Generic", "thisLanguage", "Name of THIS language") };
  int index = -1;
  if (app) 
    for (int i=0; languages[i]; i++)
      {
        QString nlang = languages[i];
        QString xlang = QString::null;
        QTranslator *trans = new QTranslator();
        if (app->loadTranslator(trans, "djview", QStringList(nlang)))
          xlang = trans->translate("Generic", thisLang[0].src);
        else if (nlang == "en" || nlang.startsWith("en"))
          xlang = "English";
        if (xlang.size() && lang == nlang)
          index = cb->count();
        if (xlang.size())
          cb->addItem(xlang, nlang);
        delete trans;
      }
  db->setChecked(index >= 0 && lang.size() > 0);
  cb->setCurrentIndex(index); qDebug() << index;
}

void
QDjViewPrefsDialog::apply()
{
  // save ui into preferences
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  // 1- screen tab
  prefs->gamma = d->ui.gammaSlider->value() / 10.0;
  prefs->resolution = 0;
  if (d->ui.forceResolutionCheckBox->isChecked())
    prefs->resolution = d->ui.resolutionSpinBox->value();
  prefs->invertLuminance = d->ui.invertLuminanceCheckBox->isChecked();
  // 2- interface tab
  int n = d->ui.modeComboBox->currentIndex();
  modeComboChanged(n);
  prefs->forStandalone = d->saved[0];
  prefs->forFullScreen = d->saved[1];
  prefs->forFullPagePlugin = d->saved[2];
  prefs->forEmbeddedPlugin = d->saved[3];
  // 3- keys
  prefs->modifiersForLens = d->ui.keysForLensCombo->value();
  prefs->modifiersForSelect = d->ui.keysForSelectCombo->value();
  prefs->modifiersForLinks = d->ui.keysForLinksCombo->value();
  prefs->mouseWheelZoom = d->ui.wheelZoomButton->isChecked();
  // 4- lens tab
  bool lens = d->ui.lensEnableCheckBox->isChecked();
  prefs->lensPower = (lens) ? d->ui.lensPowerSpinBox->value() : 0;
  prefs->lensSize = d->ui.lensSizeSpinBox->value();
  // 5- network tab
  prefs->proxyUrl = QUrl();
  if (d->ui.proxyCheckBox->isChecked())
    {
      prefs->proxyUrl.setScheme("http");
      prefs->proxyUrl.setHost(d->ui.proxyHostLineEdit->text());
      prefs->proxyUrl.setPort(d->ui.proxyPortSpinBox->value());
      prefs->proxyUrl.setUserName(d->ui.proxyUserLineEdit->text());
      prefs->proxyUrl.setPassword(d->ui.proxyPasswordLineEdit->text());
    }
  // 6- advanced tab
  prefs->pixelCacheSize = d->ui.pixelCacheSpinBox->value() * 256 * 1024;
  prefs->cacheSize = d->ui.pageCacheSpinBox->value() * 1024 * 1024;
  prefs->languageOverride = QString::null;
  if (d->ui.languageCheckBox->isChecked()) 
    {
      int n = d->ui.languageComboBox->currentIndex();
      prefs->languageOverride = d->ui.languageComboBox->itemData(n).toString();
    }
  prefs->printerGamma = 0;
  if (d->ui.printerManualCheckBox->isChecked())
    prefs->printerGamma = d->ui.printerGammaSpinBox->value();
  prefs->enableAnimations = d->ui.animationCheckBox->isChecked();
  prefs->advancedFeatures = d->ui.advancedCheckBox->isChecked();
  // broadcast change
  setWindowModified(false);
  d->ui.applyButton->setEnabled(false);
  prefs->save();
  prefs->update();
}


void
QDjViewPrefsDialog::reset()
{
  // 1- gamma tab
  d->ui.gammaSlider->setValue(22);
  d->ui.forceResolutionCheckBox->setChecked(true);
  d->ui.resolutionSpinBox->setValue(100);
  // 2- interface tab
  QDjViewPrefs::Saved defsaved;
  QDjViewPrefs::Options optMSS = (QDjViewPrefs::SHOW_MENUBAR|
                                  QDjViewPrefs::SHOW_STATUSBAR|
                                  QDjViewPrefs::SHOW_SIDEBAR);
  QDjViewPrefs::Options optS = QDjViewPrefs::SHOW_SCROLLBARS;
  QDjViewPrefs::Options optT = QDjViewPrefs::SHOW_TOOLBAR;
  d->saved[0] = defsaved;
  d->saved[1] = defsaved;
  d->saved[1].options &= ~(optMSS|optS|optT);
  d->saved[2] = defsaved;
  d->saved[2].options &= ~(optMSS);
  d->saved[3] = defsaved;
  d->saved[3].options &= ~(optMSS|optT);
  d->saved[3].remember = false;
  d->currentSaved = -1;
  modeComboChanged(d->ui.modeComboBox->currentIndex());
  // 3- keys tab
  d->ui.keysForLensCombo->setValue(Qt::ShiftModifier|Qt::ControlModifier);
  d->ui.keysForSelectCombo->setValue(Qt::ControlModifier);
  d->ui.keysForLinksCombo->setValue(Qt::ShiftModifier);
  // 4- lens tab
  d->ui.lensEnableCheckBox->setChecked(true);
  d->ui.lensPowerSpinBox->setValue(3);
  d->ui.lensSizeSpinBox->setValue(300);
  // 5- network tab
  d->ui.proxyCheckBox->setChecked(false);
  // 6- advanced tab
  d->ui.pixelCacheSpinBox->setValue(1);
  d->ui.pageCacheSpinBox->setValue(10);
  d->ui.languageCheckBox->setChecked(false);
  d->ui.printerManualCheckBox->setChecked(false);
  d->ui.printerGammaSpinBox->setValue(2.2);
  d->ui.advancedCheckBox->setChecked(false);
}


void
QDjViewPrefsDialog::setModified()
{
  d->ui.applyButton->setEnabled(true);
  setWindowModified(true);
}


void
QDjViewPrefsDialog::done(int result)
{
  if (result == QDialog::Accepted)
    apply();
  QDialog::done(result);
}
 

void 
QDjViewPrefsDialog::cacheClear()
{
  if (d->context)
    ddjvu_cache_clear(*d->context);
}


template<class T> static inline void
set_reset(QFlags<T> &x, T y, bool plus, bool minus=true)
{
  if (plus)
    x |= y;
  else if (minus)
    x &= ~y;
}


void 
QDjViewPrefsDialog::modeComboChanged(int n)
{
  if (d->currentSaved >=0 && d->currentSaved <4)
    {
      // save ui values
      QDjViewPrefs::Saved &saved = d->saved[d->currentSaved];
      saved.remember = d->ui.rememberCheckBox->isChecked();
      set_reset(saved.options, QDjViewPrefs::SHOW_MENUBAR,
                d->ui.menuBarCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::SHOW_TOOLBAR,
                d->ui.toolBarCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::SHOW_SCROLLBARS,
                d->ui.scrollBarsCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::SHOW_STATUSBAR,
                d->ui.statusBarCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::SHOW_SIDEBAR,
                d->ui.sideBarCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::SHOW_FRAME,
                d->ui.frameCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::SHOW_MAPAREAS,
                d->ui.mapAreasCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::LAYOUT_CONTINUOUS,
                d->ui.continuousCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::LAYOUT_SIDEBYSIDE,
                d->ui.sideBySideCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::LAYOUT_COVERPAGE,
                d->ui.coverPageCheckBox->isChecked() );
      set_reset(saved.options, QDjViewPrefs::LAYOUT_RIGHTTOLEFT,
                d->ui.rtolCheckBox->isChecked() );
      // zoom
      bool okay;
      int zoomIndex = d->ui.zoomCombo->currentIndex();
      QString zoomText = d->ui.zoomCombo->lineEdit()->text();
      int zoom = zoomText.replace(QRegExp("\\s*%?$"),"").trimmed().toInt(&okay);
      if (okay && zoom>=QDjVuWidget::ZOOM_MIN && zoom<=QDjVuWidget::ZOOM_MAX)
        saved.zoom = zoom;
      else if (zoomIndex >= 0)
        saved.zoom = d->ui.zoomCombo->itemData(zoomIndex).toInt();
    }
  d->currentSaved = n;
  if (d->currentSaved >=0 && d->currentSaved <4)
    {
      // save modified flag
      bool modified = isWindowModified();
      // load ui values
      QDjViewPrefs::Saved &saved = d->saved[d->currentSaved];
      d->ui.rememberCheckBox->setChecked(saved.remember);
      QDjViewPrefs::Options opt = saved.options;
      d->ui.menuBarCheckBox->setChecked(opt & QDjViewPrefs::SHOW_MENUBAR);
      d->ui.toolBarCheckBox->setChecked(opt & QDjViewPrefs::SHOW_TOOLBAR);
      d->ui.scrollBarsCheckBox->setChecked(opt & QDjViewPrefs::SHOW_SCROLLBARS);
      d->ui.statusBarCheckBox->setChecked(opt & QDjViewPrefs::SHOW_STATUSBAR);
      d->ui.sideBarCheckBox->setChecked(opt & QDjViewPrefs::SHOW_SIDEBAR);
      d->ui.frameCheckBox->setChecked(opt & QDjViewPrefs::SHOW_FRAME);
      d->ui.mapAreasCheckBox->setChecked(opt & QDjViewPrefs::SHOW_MAPAREAS);
      d->ui.continuousCheckBox->setChecked(opt & QDjViewPrefs::LAYOUT_CONTINUOUS);
      d->ui.sideBySideCheckBox->setChecked(opt & QDjViewPrefs::LAYOUT_SIDEBYSIDE);
      d->ui.coverPageCheckBox->setChecked(opt & QDjViewPrefs::LAYOUT_COVERPAGE);
      d->ui.rtolCheckBox->setChecked(opt & QDjViewPrefs::LAYOUT_RIGHTTOLEFT);
      // zoom
      int zoomIndex = d->ui.zoomCombo->findData(QVariant(saved.zoom));
      d->ui.zoomCombo->clearEditText();
      d->ui.zoomCombo->setCurrentIndex(zoomIndex);
      if (zoomIndex < 0 &&
          saved.zoom >= QDjVuWidget::ZOOM_MIN && 
          saved.zoom <= QDjVuWidget::ZOOM_MAX)
        d->ui.zoomCombo->setEditText(QString("%1%").arg(saved.zoom));
      else if (zoomIndex >= 0)
        d->ui.zoomCombo->setEditText(d->ui.zoomCombo->itemText(zoomIndex));
      // reset modified flag
      setWindowModified(modified);
    }
}


void 
QDjViewPrefsDialog::zoomComboEdited()
{
  bool okay;
  QString zoomText = d->ui.zoomCombo->lineEdit()->text();
  int zoom = zoomText.replace(QRegExp("\\s*%?$"),"").trimmed().toInt(&okay);
  if (d->ui.zoomCombo->findText(zoomText) >= 0)
    d->ui.zoomCombo->setEditText(zoomText);
  else if (okay && zoom >= QDjVuWidget::ZOOM_MIN 
           && zoom <= QDjVuWidget::ZOOM_MAX)
    d->ui.zoomCombo->setEditText(QString("%1%").arg(zoom));
}



void 
QDjViewPrefsDialog::forceResolutionChanged()
{
  if (! d->ui.forceResolutionCheckBox->isChecked())
    d->ui.resolutionSpinBox->setValue(physicalDpiY());
}



// ----------------------------------------
// MOC

#include "qdjviewprefs.moc"


/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
