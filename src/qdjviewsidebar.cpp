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

#include "stdlib.h"
#include "math.h"

#include <QHeaderView>
#include <QList>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <libdjvu/ddjvuapi.h>
#include <libdjvu/miniexp.h>

#include "qdjvu.h"
#include "qdjvuwidget.h"
#include "qdjviewsidebar.h"
#include "qdjview.h"




// ----------------------------------------
// OUTLINE




QDjViewOutline::QDjViewOutline(QDjView *djview)
  : QWidget(djview), djview(djview), tree(0), loaded(false)
{
  connect(djview, SIGNAL(documentClosed()),
          this, SLOT(clear()) );
  connect(djview, SIGNAL(documentOpened(QDjVuDocument*)),
          this, SLOT(clear()) );
  connect(djview, SIGNAL(documentReady(QDjVuDocument*)),
          this, SLOT(refresh()) );
  connect(djview->getDjVuWidget(), SIGNAL(pageChanged(int)),
          this, SLOT(pageChanged(int)) );
  connect(djview->getDjVuWidget(), SIGNAL(layoutChanged()),
          this, SLOT(refresh()) );

  tree = new QTreeWidget(this);
  tree->setColumnCount(1);
  tree->setItemsExpandable(true);
  tree->setUniformRowHeights(true);
  tree->header()->hide();
  tree->header()->setStretchLastSection(true);
  tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tree->setSelectionBehavior(QAbstractItemView::SelectRows);
  tree->setSelectionMode(QAbstractItemView::SingleSelection);
  tree->setTextElideMode(Qt::ElideRight);
  connect(tree, SIGNAL(itemActivated(QTreeWidgetItem*, int)),
          this, SLOT(itemActivated(QTreeWidgetItem*)) );
  
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(tree);

  setWhatsThis(tr("<html><b>Document outline.</b><br/> "
                  "This panel display the document outline, "
                  "or the page names when the outline is not available, "
                  "Double-click any entry to jump to the selected page."
                  "</html>"));
  
  if (djview->pageNum() > 0)
    refresh();

}


void 
QDjViewOutline::clear()
{
  tree->clear();
  loaded = false;
}

void 
QDjViewOutline::refresh()
{
  QDjVuDocument *doc = djview->getDocument();
  if (doc && !loaded && djview->pageNum()>0)
    {
      miniexp_t outline = doc->getDocumentOutline();
      if (outline == miniexp_dummy)
        return;
      loaded = true;
      if (outline)
        {
          if (!miniexp_consp(outline) ||
              miniexp_car(outline) != miniexp_symbol("bookmarks"))
            {
              QString msg = tr("Outline data is corrupted");
              qWarning((const char*)msg.toLocal8Bit());
            }
          tree->clear();
          QTreeWidgetItem *root = new QTreeWidgetItem();
          fillItems(root, miniexp_cdr(outline));
          QTreeWidgetItem *item = root;
          while (item->childCount()==1 &&
                 item->data(0,Qt::UserRole).toInt() >= 0)
            item = item->child(0);
          while (item->childCount() > 0)
            tree->insertTopLevelItem(tree->topLevelItemCount(),
                                     item->takeChild(0) );
          delete root;
        }
      else
        {
          tree->clear();
          QTreeWidgetItem *root = new QTreeWidgetItem(tree);
          root->setText(0, tr("Pages"));
          root->setFlags(0);
          root->setData(0, Qt::UserRole, -1);
          for (int pageno=0; pageno<djview->pageNum(); pageno++)
            {
              QTreeWidgetItem *item = new QTreeWidgetItem(root);
              QString name = djview->pageName(pageno);
              item->setText(0, tr("Page %1").arg(name));
              item->setData(0, Qt::UserRole, pageno);
              item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
              item->setToolTip(0, tr("Go to page %1").arg(name));
              item->setWhatsThis(0, whatsThis());
            }
          tree->setItemExpanded(root, true);
        }
      pageChanged(djview->getDjVuWidget()->page());
    }
}


void 
QDjViewOutline::fillItems(QTreeWidgetItem *root, miniexp_t expr)
{
  while(miniexp_consp(expr))
    {
      miniexp_t s = miniexp_car(expr);
      expr = miniexp_cdr(expr);
      if (miniexp_consp(s) &&
          miniexp_consp(miniexp_cdr(s)) &&
          miniexp_stringp(miniexp_car(s)) &&
          miniexp_stringp(miniexp_cadr(s)) )
        {
          // fill item
          const char *name = miniexp_to_str(miniexp_car(s));
          const char *page = miniexp_to_str(miniexp_cadr(s));
          QTreeWidgetItem *item = new QTreeWidgetItem(root);
          item->setText(0, QString::fromUtf8(name));
          int pageno = -1;
          if (page[0]=='#' && page[1]!='+' && page[1]!='-')
            pageno = djview->pageNumber(QString::fromUtf8(page));
          item->setData(0, Qt::UserRole, pageno);
          item->setFlags(0);
          item->setWhatsThis(0, whatsThis());
          if (pageno >= 0)
            {
              QString name = djview->pageName(pageno);
              item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
              item->setToolTip(0, tr("Go to page %1").arg(name));
            }
          // recurse
          fillItems(item, miniexp_cddr(s));
        }
    }
}


void 
QDjViewOutline::pageChanged(int pageno)
{
  int fp = -1;
  QTreeWidgetItem *fi = 0;
  // find current selection
  QList<QTreeWidgetItem*> sel = tree->selectedItems();
  QTreeWidgetItem *si = 0;
  if (sel.size() == 1)
    si = sel[0];
  // current selection has priority
  if (si)
    searchItem(si, pageno, fi, fp);
  // search
  for (int i=0; i<tree->topLevelItemCount(); i++)
    searchItem(tree->topLevelItem(i), pageno, fi, fp);
  // select
  if (si && fi && si != fi)
    tree->setItemSelected(si, false);
  if (fi && si != fi)
    {
      tree->setCurrentItem(fi);
      tree->setItemSelected(fi, true);
      tree->scrollToItem(fi);
    }
}


void 
QDjViewOutline::searchItem(QTreeWidgetItem *item, int pageno, 
                           QTreeWidgetItem *&fi, int &fp)
{
  int page = item->data(0, Qt::UserRole).toInt();
  if (page>=0 && page<=pageno && page>fp)
    {
      fi = item;
      fp = page;
    }
  for (int i=0; i<item->childCount(); i++)
    searchItem(item->child(i), pageno, fi, fp);
}


void 
QDjViewOutline::itemActivated(QTreeWidgetItem *item)
{
  int pageno = item->data(0, Qt::UserRole).toInt();
  if (pageno >= 0 && pageno < djview->pageNum())
    djview->goToPage(pageno);
}








// ----------------------------------------
// THUMBNAILS









/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
