//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2009-  Leon Bottou
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

// $Id$

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

#include <QtCore/QVariant>
#include <QtCore/QMutexLocker>
#include <QtGui/QWidget>

#ifdef Q_WS_X11
#   include <X11/Xlib.h>

class QtNPStream;
class QtNPBindable;
#endif

struct QtNPInstance
{
    NPP npp;
    short fMode;
#ifdef Q_WS_WIN
    typedef HWND Widget;
#endif
#ifdef Q_WS_X11
    typedef Window Widget;
    Display *display;
#endif
#ifdef Q_WS_MAC
    typedef NPPort* Widget;
    QWidget *rootWidget;
#endif
    Widget window;
    QRect geometry;
    QString mimetype;
    QByteArray htmlID;
    union {
        QObject* object;
        QWidget* widget;
    } qt;

#ifdef NPDJVU
    // changes for npdjvu
    QList<QString> args;
    QList<QtNPStream*> streams;
#else
    QtNPStream *pendingStream;
    QtNPBindable* bindable;
    QObject *filter;
    QMap<QByteArray, QVariant> parameters;
    qint32 notificationSeqNum;
    QMutex seqNumMutex;
    qint32 getNotificationSeqNum()
        {
            QMutexLocker locker(&seqNumMutex);

            if (++notificationSeqNum < 0)
                notificationSeqNum = 1;
            return notificationSeqNum;
        }
#endif
};
