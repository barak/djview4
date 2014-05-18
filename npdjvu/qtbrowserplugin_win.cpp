/****************************************************************************
**
** Copyright (C) 2003-2008 Trolltech ASA. All rights reserved.
**
** This file is part of a Qt Solutions component.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** Trolltech sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#include <QtGui>

#include "qtbrowserplugin.h"
#include "qtbrowserplugin_p.h"

#include <windows.h>
#include "qtnpapi.h"

#if QT_VERSION >= 0x050000
# define QT_WA(unicode, ansi) unicode
#endif

static HHOOK hhook = 0;
static bool ownsqapp = false;


static const uint keytbl[256] = {  // Keyboard mapping table
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_Cancel,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_Backspace, Qt::Key_Tab, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_Clear, Qt::Key_Return, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_Shift, Qt::Key_Control, Qt::Key_Alt, Qt::Key_Pause,
  Qt::Key_CapsLock, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_Escape,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_Mode_switch,
  Qt::Key_Space, Qt::Key_PageUp, Qt::Key_PageDown, Qt::Key_End,
  Qt::Key_Home, Qt::Key_Left, Qt::Key_Up, Qt::Key_Right,
  Qt::Key_Down, Qt::Key_Select, Qt::Key_Printer, Qt::Key_Execute,
  Qt::Key_Print, Qt::Key_Insert, Qt::Key_Delete, Qt::Key_Help,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_Meta,
  Qt::Key_Meta, Qt::Key_Menu, Qt::Key_unknown, Qt::Key_Sleep,
  Qt::Key_0, Qt::Key_1, Qt::Key_2, Qt::Key_3,
  Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7,
  Qt::Key_8, Qt::Key_9, Qt::Key_Asterisk, Qt::Key_Plus,
  Qt::Key_Comma, Qt::Key_Minus, Qt::Key_Period, Qt::Key_Slash,
  Qt::Key_F1, Qt::Key_F2, Qt::Key_F3, Qt::Key_F4,
  Qt::Key_F5, Qt::Key_F6, Qt::Key_F7, Qt::Key_F8,
  Qt::Key_F9, Qt::Key_F10, Qt::Key_F11, Qt::Key_F12,
  Qt::Key_F13, Qt::Key_F14, Qt::Key_F15, Qt::Key_F16,
  Qt::Key_F17, Qt::Key_F18, Qt::Key_F19, Qt::Key_F20,
  Qt::Key_F21, Qt::Key_F22, Qt::Key_F23, Qt::Key_F24,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_NumLock, Qt::Key_ScrollLock, Qt::Key_unknown, Qt::Key_Massyo,
  Qt::Key_Touroku, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_Shift, Qt::Key_Shift, Qt::Key_Control, Qt::Key_Control,
  Qt::Key_Alt, Qt::Key_Alt, Qt::Key_Back, Qt::Key_Forward,
  Qt::Key_Refresh, Qt::Key_Stop, Qt::Key_Search, Qt::Key_Favorites,
  Qt::Key_HomePage, Qt::Key_VolumeMute, Qt::Key_VolumeDown, Qt::Key_VolumeUp,
  Qt::Key_MediaNext, Qt::Key_MediaPrevious, Qt::Key_MediaStop, Qt::Key_MediaPlay,
  Qt::Key_LaunchMail, Qt::Key_LaunchMedia, Qt::Key_Launch0, Qt::Key_Launch1,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown, Qt::Key_unknown,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_Play, Qt::Key_Zoom,
  Qt::Key_unknown, Qt::Key_unknown, Qt::Key_Clear, 0
};

static int qt_translateKeyCode(int vk)
{
  if (vk >= 0 && vk <= 255)
    return keytbl[vk];
  return Qt::Key_unknown;
}

LRESULT CALLBACK FilterProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    if (qApp)
	qApp->sendPostedEvents(0, -1);
    if (nCode < 0 || !(wParam & PM_REMOVE))
        return CallNextHookEx(hhook, nCode, wParam, lParam);
    MSG *msg = (MSG*)lParam;
    bool processed = false;
    // (some) support for key-sequences via QAction and QShortcut
    if(msg->message == WM_KEYDOWN || msg->message == WM_SYSKEYDOWN) {
      QWidget *focusWidget = QWidget::find(msg->hwnd);
        if (focusWidget) {
            int key = msg->wParam;
            if (!(key >= 'A' && key <= 'Z') && !(key >= '0' && key <= '9'))
                key = qt_translateKeyCode(msg->wParam);
            Qt::KeyboardModifiers modifiers = 0;
            int modifierKey = 0;
            if (GetKeyState(VK_SHIFT) < 0) {
                modifierKey |= Qt::SHIFT;
                modifiers |= Qt::ShiftModifier;
            }
            if (GetKeyState(VK_CONTROL) < 0) {
                modifierKey |= Qt::CTRL;
                modifiers |= Qt::ControlModifier;
            }
            if (GetKeyState(VK_MENU) < 0) {
                modifierKey |= Qt::ALT;
                modifiers |= Qt::AltModifier;
            }
            QKeyEvent override(QEvent::ShortcutOverride, key, modifiers);
            override.ignore();
            QApplication::sendEvent(focusWidget, &override);
            processed = override.isAccepted();
            QKeySequence shortcutKey(modifierKey + key);
            if (!processed) {
                QList<QAction*> actions = focusWidget->window()->findChildren<QAction*>();
                for (int i = 0; i < actions.count() && !processed; ++i) {
                    QAction *action = actions.at(i);
                    if (!action->isEnabled() || action->shortcut() != shortcutKey)
                        continue;
                    QShortcutEvent event(shortcutKey, 0);
                    processed = QApplication::sendEvent(action, &event);
                }
            }
            if (!processed) {
                QList<QShortcut*> shortcuts = focusWidget->window()->findChildren<QShortcut*>();
                for (int i = 0; i < shortcuts.count() && !processed; ++i) {
                    QShortcut *shortcut = shortcuts.at(i);
                    if (!shortcut->isEnabled() || shortcut->key() != shortcutKey)
                        continue;
                    QShortcutEvent event(shortcutKey, shortcut->id());
                    processed = QApplication::sendEvent(shortcut, &event);
                }
            }
        }
    }
    return CallNextHookEx(hhook, nCode, wParam, lParam);
}

extern "C" bool qtns_event(QtNPInstance *, NPEvent *)
{
    return false;
}

extern "C" void qtns_initialize(QtNPInstance*)
{
    if (!qApp) {
        ownsqapp = true;
	static int argc=0;
	static char **argv={ 0 };
	(void)new QApplication(argc, argv);
        QT_WA({
	    hhook = SetWindowsHookExW( WH_GETMESSAGE, FilterProc, 0, GetCurrentThreadId() );
        }, {
	    hhook = SetWindowsHookExA( WH_GETMESSAGE, FilterProc, 0, GetCurrentThreadId() );
        });
    }
}

extern "C" void qtns_destroy(QtNPInstance *)
{
}

extern "C" void qtns_shutdown()
{
    if (!ownsqapp)
        return;

    // check if qApp still runs widgets (in other DLLs)
    QWidgetList widgets = qApp->allWidgets();
    int count = widgets.count();
    for (int w = 0; w < widgets.count(); ++w) {
        // ignore all Qt generated widgets
        QWidget *widget = widgets.at(w);
        if (widget->windowFlags() & Qt::Desktop)
            count--;
    }
    if (count) // qApp still used
        return;

    delete qApp;
    ownsqapp = false;
    if ( hhook )
        UnhookWindowsHookEx( hhook );
    hhook = 0;
}

extern "C" void qtns_embed(QtNPInstance *This)
{
    Q_ASSERT(qobject_cast<QWidget*>(This->qt.object));

    LONG oldLong = GetWindowLong(This->window, GWL_STYLE);
    ::SetWindowLong(This->window, GWL_STYLE, oldLong | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    ::SetWindowLong(This->qt.widget->winId(), GWL_STYLE, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    ::SetParent(This->qt.widget->winId(), This->window);
}

extern "C" void qtns_setGeometry(QtNPInstance *This, const QRect &rect, const QRect &)
{
    Q_ASSERT(qobject_cast<QWidget*>(This->qt.object));

    This->qt.widget->setGeometry(QRect(0, 0, rect.width(), rect.height()));
}

/*
extern "C" void qtns_print(QtNPInstance * This, NPPrint *printInfo)
{
    NPWindow* printWindow = &(printInfo->print.embedPrint.window);
    void* platformPrint = printInfo->print.embedPrint.platformPrint;
    // #### Nothing yet.
}
*/
