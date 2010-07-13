/* PET
 * Platform for Experimentation with efficient HPSG processing Techniques
 * (C) 1999 - 2002 Ulrich Callmeier uc@coli.uni-sb.de
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef GOOFY_H
#define GOOFY_H

#include <qdialog.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmainwindow.h>
#include <qstring.h>
#include <qtimer.h>
#include <QTextStream>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QDragEnterEvent>
#include <QListWidget>

#include <string>
#include <list>

using std::string;
using std::list;

#include "grammar.h"
#include "dag.h"

class QTextEdit;
class QMenu;
class QSocketNotifier;
class QString;
class QTextStream;
class QTimer;
class QTooltip;

class GoofyWindow: public QMainWindow
{
  Q_OBJECT
 public:
  GoofyWindow();
  ~GoofyWindow();

  void load_grammar(const char *fileName);
  void preprocess_grammar(const char *fileName);

protected:

  QMenu *mFile, *mView, *mParse, *mOptions, *mHelp;

  QAction* viewTypeAction;
  QAction* viewInstanceAction;
  QAction* viewLexentryAction;
  QAction* viewRuleAction;
  QAction* parseInputAction;
  QAction* parseFileAction;
  QAction* parseChartAction;

  void set_menu_state(bool);
  void not_implemented();

private slots:
  // menu entries
  void filePreprocess();
  void fileLoad();

  void viewType();
  void viewInstance();
  void viewLexentry();
  void viewRule();

  void parseInput();
  void parseFile();
  void parseChart();

  void optionsGeneral();
  void optionsPre();
  void optionsParser();

  void helpAbout();
  void helpCore();
  void aboutQt();

  void data_received(int fd);

private:
  QTextEdit *e;
  QString filename;
  QStringList _sort_names;
  QStringList _inst_names;
  QStringList _rule_names;
  
  QTextStream *flop_stat_stream, *flop_err_stream;
  QSocketNotifier *flop_stat_sn;

  tGrammar *G;
  
};

class StringSelDialog : public QDialog
{
  Q_OBJECT

 public:
  StringSelDialog(const QStringList &, const QString &what);
  inline QString &selected() { return _selected; }

 public slots:
  void setselected(const QString &name) { _selected = name; }

 private:
  QListWidget* _list_box; 
  QString _selected;
};

class StringInputDialog : public QDialog
{
  Q_OBJECT

 public:
  StringInputDialog(const QString &what);
  inline QString &selected() { _sel = _edit->text(); return _sel; }

 private:
  QString _sel;
  QLineEdit *_edit;
};

// this passes pointers --> doesn't work across applications
//class TFSDrag : public Q3StoredDrag
//{
// public:
//  TFSDrag(dag_node *dag, QWidget *parent, const char *name = 0);
//  ~TFSDrag() {}
//
//  static bool canDecode(QDropEvent* e);
//  static bool decode(QDropEvent* e, dag_node * &dag);
//};
#if 0
class TFSViewItem : public QObject, public QListWidget
{
  Q_OBJECT
 public:
  TFSViewItem(QListWidget *parent, TFSViewItem *after, dag_node *, dag_node *root,
              int tag = -1, int sort = -1);
  TFSViewItem(TFSViewItem *parent, TFSViewItem *after, dag_node *, dag_node *root,
              int attr, int tag = -1, int sort = -1);
  TFSViewItem::~TFSViewItem();

  QString text( int column ) const;
  void setup();

  void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align);
  void paintFocus(QPainter *p, const QColorGroup &cg, const QRect &r);
  int width(const QFontMetrics &fm, const QListWidget *lv, int c) const;

  void set_failure(failure *f);
  void register_coref(TFSViewItem *next);

  class TFSViewItem *deref();

  inline int attr() { return _attr; }
  inline list_int *path() { return _path; }
  inline int id() { return _id; }
  inline dag_node *dag() { return _dag; }
  inline dag_node *root() { return _root; }
  inline TFSViewItem *next_coref() { return _next_cref; }

  inline int max_attr_width() { return _max_attr_width; }
  inline void tell_attr_width(int w) { if(w > _max_attr_width) _max_attr_width = w; }

  void indent_attr();

  QString tip();

  void popup(const QPoint &pos);

  void open();

 public slots:
  void goto_coref(int id);

 private:
  static int _next_id;
  int _id;
  
  int _sort, _tag, _attr;
  TFSViewItem *_parent;

  int _w_inter, _w_attr, _w_tag, _w_sort, _w_total;
  QString _s_attr, _s_tag, _s_sort;
  QColor _color;
  
  int _max_attr_width;

  list_int *_path;
  dag_node *_dag, *_root;

  class TFSViewItem *_next_cref;

  failure *_failure;

  QMenu *_popupmenu;
  
  class TFSView *_tfs;

  void setup_text();
  
};
#endif

class TFSView : public QListView
{
  Q_OBJECT

 public:
  TFSView( QWidget * parent, bool failure, const char * name = 0 );
  ~TFSView();

#if 0
  void new_coref(int tag, TFSViewItem *item);
  void register_coref(int tag, TFSViewItem *item);
  TFSViewItem *coref(int tag);
  TFSViewItem *path_value(list_int *path);
#endif
 signals:
  void message( const QString& );

 protected:
  void contentsDragEnterEvent( QDragEnterEvent * );
  void contentsDragMoveEvent( QDragMoveEvent * );
  void contentsDragLeaveEvent( QDragLeaveEvent * );
  void contentsDropEvent( QDropEvent * );
  void contentsMousePressEvent( QMouseEvent* e );
  void contentsMouseMoveEvent( QMouseEvent* e );

  void do_unification(dag_node *root, dag_node *a, dag_node *b, list_int *p);

  int _ncorefs; int _corefs_alloc;
#if 0
  TFSViewItem **_corefs;
#endif
  bool _failure;

  QPoint presspos;
  QListWidgetItem *oldCurrent;
#if 0
  TFSViewItem *dropItem;
#endif
  QTimer autoopen_timer;

 protected slots:
  void openBranch();

// stuff required for autoscroll for drag'n'drop

 private:
  QTimer autoscroll_timer;
  int autoscroll_time;
  int autoscroll_accel;

 public slots:
  void startAutoScroll();
  void stopAutoScroll();

 protected slots:
  void autoScroll();

};

class DagView : public QDialog
{
  Q_OBJECT

 public:
  DagView(dag_node *dag, QString cap);
  DagView(dag_node *dag, list<failure *> fails);

 private:
  dag_node *_dag;
  TFSView *_tfs;
};

#endif
