//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006  Leon Bottou
//C-
//C- This software is subject to, and may be distributed under, the
//C- GNU General Public License. The license should have
//C- accompanied the software or you may obtain a copy of the license
//C- from the Free Software Foundation at http://www.fsf.org .
//C-
//C- This program is distributed in the hope that it will be useful,
//C- but WITHOUT ANY WARRANTY; without even the implied warranty of
//C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//C- GNU General Public License for more details.
//C-  ------------------------------------------------------------------

// $Id$

#ifndef QDJVIEWPREFS_H
#define QDJVIEWPREFS_H

#include <Qt>
#include <QByteArray>
#include <QFlags>
#include <QObject>
#include <QSettings>
#include <QSize>
#include <QString>


#ifndef DJVIEW_ORG
# define DJVIEW_ORG "DjVuLibre"
#endif
#ifndef DJVIEW_APP
# define DJVIEW_APP "DjView"
#endif
#ifndef DJVIEW_VERSION
# define DJVIEW_VERSION 0x40000
#endif


class QDjViewPrefs : public QObject
{
  Q_OBJECT
  Q_ENUMS(Option Tool)

public:

  enum Option {
    SHOW_MENUBAR        = 0x000001,
    SHOW_TOOLBAR        = 0x000002,
    SHOW_SIDEBAR        = 0x000004,
    SHOW_STATUSBAR      = 0x000010,
    SHOW_SCROLLBARS     = 0x000020,
    SHOW_FRAME          = 0x000040,
    SHOW_MASK           = 0x000FFF, 
    LAYOUT_CONTINUOUS   = 0x001000,
    LAYOUT_SIDEBYSIDE   = 0x002000,
    LAYOUT_MASK         = 0x00F000, 
    HANDLE_MOUSE        = 0x100000,
    HANDLE_KEYBOARD     = 0x200000,
    HANDLE_LINKS        = 0x400000,
    HANDLE_CONTEXTMENU  = 0x800000,
  };

  enum Tool {
    TOOLBAR_TOP      = 0x80000000,
    TOOLBAR_BOTTOM   = 0x40000000,
    TOOLBAR_AUTOHIDE = 0x10000000,
    TOOL_MODECOMBO    = 0x00001,
    TOOL_MODEBUTTONS  = 0x00002,
    TOOL_ZOOMCOMBO    = 0x00004,
    TOOL_ZOOMBUTTONS  = 0x00008,
    TOOL_PAGECOMBO    = 0x00010,
    TOOL_PREVNEXT     = 0x00020,
    TOOL_FIRSTLAST    = 0x00040,
    TOOL_BACKFORW     = 0x00080,
    TOOL_NEW          = 0x00100,
    TOOL_OPEN         = 0x00200,
    TOOL_FIND         = 0x00400,
    TOOL_SELECT       = 0x00800,
    TOOL_SAVE         = 0x01000,
    TOOL_PRINT        = 0x02000,
    TOOL_LAYOUT       = 0x04000,
    TOOL_ROTATE       = 0x08000,
    TOOL_WHATSTHIS    = 0x10000,
  };

  Q_DECLARE_FLAGS(Options, Option)
  Q_DECLARE_FLAGS(Tools, Tool)
  
  static const Options defaultOptions;
  static const Tools   defaultTools;

  static QString versionString();
  QString modifiersToString(Qt::KeyboardModifiers);
  Qt::KeyboardModifiers stringToModifiers(QString);
  QString optionsToString(Options);
  Options stringToOptions(QString);
  QString toolsToString(Tools);
  Tools   stringToTools(QString);


  struct Saved {
    Saved();
    bool       remember;        //!< Save before closing.
    Options    options;         //!< Miscellaneous flags.
    int        zoom;            //!< Zoom factor.
    QByteArray state;           //!< Toolbar and sidebar geometry.
    int        sidebarTab;      //!< Selected sidebar tab. 
  };

  // Preferences remembered between program invocations.
  Saved      forEmbeddedPlugin; //!< Prefs for the embedded plugin mode.
  Saved      forFullPagePlugin; //!< Prefs for the full page plugin mode.
  Saved      forStandalone;     //!< Prefs for the standalone viewer mode.
  Saved      forFullScreen;     //!< Prefs for the full screen mode.
  QSize      windowSize;        //!< Size of the standalone window.

  // Preferences set via the preference dialog.
  Tools      tools;             //!< Toolbar composition.
  double     gamma;             //!< Gamma value of the target screen.
  qlonglong  cacheSize;         //!< Size of the decoded page cache.
  int        pixelCacheSize;    //!< Size of the pixel cache.
  int        lensSize;          //!< Size of the magnification lens.
  int        lensPower;         //!< Power of the magnification lens.
  QString    browserProgram;    //!< Preferred web browser.
  Qt::KeyboardModifiers modifiersForLens;   //!< Keys for the lens. 
  Qt::KeyboardModifiers modifiersForSelect; //!< Keys for selecting.
  Qt::KeyboardModifiers modifiersForLinks;  //!< Keys for showing the links.
  
  // Sidebar preferences
  int        thumbnailSize;
  bool       thumbnailSmart;
  bool       searchWordsOnly;
  bool       searchCaseSensitive;

  // Dialog preferences
  int        infoDialogTab;     //!< Active tab in info dialog.
  int        metaDialogTab;     //!< Active tab in meta dialog.
  double     printerGamma;      //!< Gamma value for the printer.
  QString    printerName;       //!< Name of the printer
  QString    printFile;         //!< Output file for print data
  bool       printReverse;      //!< Print revere flag
  bool       printCollate;      //!< Print collate flag
  
public:
  static QDjViewPrefs *instance(void);
  void load(void);
  void save(void);

private:
  QDjViewPrefs(void);
  void loadGroup(QSettings &s, QString name, Saved &saved);
  void saveGroup(QSettings &s, QString name, Saved &saved);
};


Q_DECLARE_OPERATORS_FOR_FLAGS(QDjViewPrefs::Options)
Q_DECLARE_OPERATORS_FOR_FLAGS(QDjViewPrefs::Tools)


#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" "qlonglong" )
   End:
   ------------------------------------------------------------- */
  
