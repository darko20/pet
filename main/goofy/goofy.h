#ifndef GOOFY_H
#define GOOFY_H

#include <qdialog.h>
#include <qdragobject.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qmainwindow.h>
#include <qstring.h>
#include <qtooltip.h>

#include "grammar.h"
#include "dag.h"

class QListBox;
class QMultiLineEdit;
class QPopupMenu;
class QSocketNotifier;
class QString;
class QTextStream;
class QTimer;

class GoofyWindow: public QMainWindow
{
  Q_OBJECT
 public:
  GoofyWindow();
  ~GoofyWindow();

  void load_grammar(const char *fileName);
  void preprocess_grammar(const char *fileName);

protected:

  QPopupMenu *mFile, *mView, *mParse, *mOptions, *mHelp;

  int id_ViewType, id_ViewInstance, id_ViewLexentry, id_ViewRule;
  int id_ParseInput, id_ParseFile, id_ParseChart;

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
  QMultiLineEdit *e;
  QString filename;
  QStringList _sort_names;
  QStringList _inst_names;
  QStringList _rule_names;
  
  QTextStream *flop_stat_stream, *flop_err_stream;
  QSocketNotifier *flop_stat_sn;

  grammar *G;
  
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
  QListBox* _list_box; 
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
class TFSDrag : public QStoredDrag
{
 public:
  TFSDrag(dag_node *dag, QWidget *parent, const char *name = 0);
  ~TFSDrag() {}

  static bool canDecode(QDropEvent* e);
  static bool decode(QDropEvent* e, dag_node * &dag);
};

class TFSViewItem : public QObject, public QListViewItem
{
  Q_OBJECT
 public:
  TFSViewItem(QListView *parent, TFSViewItem *after, dag_node *, dag_node *root,
	      int tag = -1, int sort = -1);
  TFSViewItem(TFSViewItem *parent, TFSViewItem *after, dag_node *, dag_node *root,
	      int attr, int tag = -1, int sort = -1);
  TFSViewItem::~TFSViewItem();

  QString text( int column ) const;
  void setup();

  void paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align);
  void paintFocus(QPainter *p, const QColorGroup &cg, const QRect &r);
  int width(const QFontMetrics &fm, const QListView *lv, int c) const;

  void set_failure(unification_failure *f);
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

  unification_failure *_failure;

  QPopupMenu *_popupmenu;
  
  class TFSView *_tfs;

  void setup_text();
  
};

class TFSView : public QListView
{
  Q_OBJECT

 public:
  TFSView( QWidget * parent, bool failure, const char * name = 0 );
  ~TFSView();

  void new_coref(int tag, TFSViewItem *item);
  void register_coref(int tag, TFSViewItem *item);
  TFSViewItem *coref(int tag);
  TFSViewItem *path_value(list_int *path);

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
  TFSViewItem **_corefs;

  bool _failure;

  QPoint presspos;
  QListViewItem *oldCurrent;
  TFSViewItem *dropItem;
  
  QTimer autoopen_timer;

 protected slots:
  void openBranch();

// stuff required for autoscroll for drag'n'drop

 private:
  QTimer autoscroll_timer;
  int autoscroll_time;
  int autoscroll_accel;

  class TFSTip *_tip;

 public slots:
  void startAutoScroll();
  void stopAutoScroll();

 protected slots:
  void autoScroll();

};

class TFSTip : public QToolTip
{
 public:
  TFSTip(TFSView *parent);

 protected:
  void maybeTip(const QPoint &);

  class TFSView *_tfsview;
};

class DagView : public QDialog
{
  Q_OBJECT

 public:
  DagView(dag_node *dag, QString cap);
  DagView(dag_node *dag, list<unification_failure *> fails);

 private:
  dag_node *_dag;
  TFSView *_tfs;
};

#endif
