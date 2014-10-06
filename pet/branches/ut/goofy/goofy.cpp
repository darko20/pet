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

// goofy - interface for flop and cheap

#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <qapplication.h>
#include <qdragobject.h>
#include <qevent.h>
#include <qfiledialog.h>
#include <qheader.h>
#include <qlayout.h>
#include <qmainwindow.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qmultilineedit.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qsocketnotifier.h>
#include <qstatusbar.h>
#include <qtextstream.h>
#include <qtooltip.h>
#include <qwidget.h>
#include <qlistbox.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qcursor.h>

#include "cheap.h"
#include "goofy.h"
#include "grammar.h"
#include "types.h"
#include "tsdb++.h"
#include "errors.h"

// #define _POSIX_OPEN_MAX 10

extern int BI_TOP, BI_ATOM, BI_NIL, BIA_FIRST, BIA_REST;
extern int BIA_ARGS;

int slave() { return 0; }
int capi_register(int (*)(char *, int, char *, int, char *),
                         int (*)(int, char *, int, int,
                                 int, int, int),
                         int (*)(char *),
                         int (*)(int, char *)) { return 0 ;}

FILE *ferr, *fstatus;

int main(int argc, char *argv[])
{
  QApplication Goofy(argc, argv);

  ferr = fstatus = stderr;

  GoofyWindow *Main = new GoofyWindow();

  Goofy.setMainWidget(Main);
  Main->show();

  Goofy.connect( &Goofy, SIGNAL(lastWindowClosed()), &Goofy, SLOT(quit()) );

  try{
    if(argc == 2)
      Main->load_grammar(argv[1]);

    Goofy.exec();
  }

  catch(tError &e)
    {
      fprintf(stderr, "%s\n", e.getMessage().c_str());
      exit(1);
    }

  return 0;
}

GoofyWindow::GoofyWindow()
  : QMainWindow( 0, "Goofy", WDestructiveClose), _sort_names(), _inst_names(), _rule_names()
{
  mFile = new QPopupMenu(this);
  menuBar()->insertItem("&File", mFile);
  mFile->insertItem("&Preprocess", this, SLOT(filePreprocess()), CTRL+Key_P);
  mFile->insertItem("&Load", this, SLOT(fileLoad()), CTRL+Key_L);
  mFile->insertSeparator();
  mFile->insertItem("&Quit", qApp, SLOT( closeAllWindows() ), CTRL+Key_Q);

  mView = new QPopupMenu(this);
  menuBar()->insertItem("&View", mView);
  id_ViewType = mView->insertItem("&Type", this, SLOT(viewType()), CTRL+Key_T);
  id_ViewInstance = mView->insertItem("&Instance", this, SLOT(viewInstance()), CTRL+Key_I);
  id_ViewLexentry = mView->insertItem("&Lexicon Entry", this, SLOT(viewLexentry()), CTRL+Key_L);
  id_ViewRule = mView->insertItem("&Rule", this, SLOT(viewRule()), CTRL+Key_R);

  mParse = new QPopupMenu(this);
  menuBar()->insertItem("&Parse", mParse);
  id_ParseInput = mParse->insertItem("&Input", this, SLOT(parseInput()), CTRL+Key_I);
  id_ParseFile = mParse->insertItem("&File", this, SLOT(parseFile()), CTRL+Key_F);
  id_ParseChart = mParse->insertItem("Show &Chart", this, SLOT(parseChart()), CTRL+Key_C);

  mOptions = new QPopupMenu(this);
  menuBar()->insertItem("&Options", mOptions);
  mOptions->insertItem("&General", this, SLOT(optionsGeneral()), CTRL+Key_G);
  mOptions->insertItem("P&reprocessor", this, SLOT(optionsPre()), CTRL+Key_R);
  mOptions->insertItem("&Parser", this, SLOT(optionsParser()), CTRL+Key_P);

  mHelp = new QPopupMenu( this );
  menuBar()->insertSeparator();
  menuBar()->insertItem( "&Help", mHelp );

  mHelp->insertItem( "&About", this, SLOT(helpAbout()), Key_F1);
  mHelp->insertItem( "Dump Core", this, SLOT(helpCore()));
  mHelp->insertSeparator();
  mHelp->insertItem( "About&Qt", this, SLOT(aboutQt()));

  set_menu_state(false);
  mParse->setItemEnabled(id_ParseChart, false);

  e = new QMultiLineEdit( this, "main area" );
  e->setFocus();
  e->setReadOnly(true);
  setCentralWidget( e );
  statusBar()->message( "Ready");
  resize( 450, 600 );

  flop_stat_stream = flop_err_stream = 0;
  flop_stat_sn = 0;

  G = 0;
}

GoofyWindow::~GoofyWindow()
{
  if(G) delete G;
}


void GoofyWindow::filePreprocess()
{
  QString fn = QFileDialog::getOpenFileName(QString::null, QString("*.tdl"),
                                            this);
  if (!fn.isEmpty())
    preprocess_grammar(fn);
  else
    statusBar()->message("Aborted", 5000);
}

void GoofyWindow::fileLoad()
{
  QString fn = QFileDialog::getOpenFileName(QString::null, QString("*.gram"),
                                            this);
  if (!fn.isEmpty())
    load_grammar(fn);
  else
    statusBar()->message("Aborted", 5000);
}

void GoofyWindow::viewType()
{
  StringSelDialog *dialog = new StringSelDialog(_sort_names, "Type");
  int ret = dialog->exec();
  if(ret == QDialog::Accepted)
    {
      if(!dialog->selected().isEmpty())
        {
          int sort = lookup_type(dialog->selected());
          if(sort != -1)
            {
              DagView *dagview = new DagView(type_dag(sort)
                                             , QString(type_name(sort)));
              dagview->show();
            }
        }
    }
}

type_t lookup_instance(string name) {
  if (name[0] != '$') { name = "$" + name; }
  return lookup_type(name.c_str());
}

void GoofyWindow::viewInstance()
{
  StringSelDialog *dialog = new StringSelDialog(_inst_names, "Instance");
  int ret = dialog->exec();
  if(ret == QDialog::Accepted)
    {
      if(!dialog->selected().isEmpty())
        {
          // _fix_me_ we maybe have to massage the string dialog->selected()
          // to start with a '$' sign ??
          int sort = lookup_instance(dialog->selected().latin1());
          if(sort != -1)
            {
              DagView *dagview = new DagView(type_dag(sort)
                                             , QString(type_name(sort)));
              dagview->show();
            }
        }
    }
}

void GoofyWindow::viewLexentry()
{
  StringInputDialog *dialog = new StringInputDialog("Lexicon Entry");
  int ret = dialog->exec();
  if(ret == QDialog::Accepted)
    {
      if(!dialog->selected().isEmpty())
        {
          /* _fix_me_ This is a bit more tricky now, i'll postpone this
          tokenlist L(dialog->selected());

          for(le_iter l(Grammar, 0, L); l.valid(); ++l)
            {
              fs f;
              f = l.current()->instantiate();

              if(f.valid())
                {
                  DagView *dagview = new DagView(f.dag(),
                                                 QString(l.current()->name()) +
                                                 QString(" + ") + QString(l.current()->affixname()));
                  dagview->show();
                }
            }
          */
        }
    }
}

void GoofyWindow::viewRule()
{
  StringSelDialog *dialog = new StringSelDialog(_rule_names, "Rule");
  int ret = dialog->exec();
  if(ret == QDialog::Accepted)
    {
      if(!dialog->selected().isEmpty())
        {
          int sort = lookup_instance(dialog->selected().latin1());
          if(sort != -1)
            {
              DagView *dagview = new DagView(type_dag(sort), dialog->selected());
              dagview->show();
            }
        }
    }
}

void GoofyWindow::parseInput()
{
  not_implemented();
}

void GoofyWindow::parseFile()
{
  not_implemented();
}

void GoofyWindow::parseChart()
{
  not_implemented();
}

void GoofyWindow::optionsGeneral()
{
  not_implemented();
}

void GoofyWindow::optionsPre()
{
  not_implemented();
}

void GoofyWindow::optionsParser()
{
  not_implemented();
}

void GoofyWindow::helpAbout()
{
  QMessageBox::about(this, "Goofy",
                     "Gooey for cheap and flop\n1999 Ulrich Callmeier");
}

void GoofyWindow::helpCore()
{
  if(QMessageBox::critical(this, "Goofy", "Sure?", "&Yeah", "Ahh, &No", 0, 1) == 0)
    {
      int *null = 0;
      *null = 0;
    }
}

void GoofyWindow::aboutQt()
{
  QMessageBox::aboutQt(this, "Goofy");
}

void GoofyWindow::not_implemented()
{
  QMessageBox::about(this, "Goofy",
                     "This is not yet implemented");
}

void GoofyWindow::set_menu_state(bool f)
{
  mView->setItemEnabled(id_ViewType, f);
  mView->setItemEnabled(id_ViewLexentry, f);
  mView->setItemEnabled(id_ViewRule, f);

  mParse->setItemEnabled(id_ParseInput, f);
  mParse->setItemEnabled(id_ParseFile, f);
}

void GoofyWindow::data_received(int fd)
{
  if(flop_stat_stream->eof())
    {
      flop_stat_sn->setEnabled(false);
    }
  else
    {
      QString l = flop_stat_stream->readLine();
      e->append(l);
    }
}

void GoofyWindow::preprocess_grammar(const char *fn)
{
  pid_t pid;
  int pipe_stat[2], pipe_err[2];
  QFile *file_stat, *file_err;

  if(flop_stat_stream)
    delete flop_stat_stream;

  if(flop_err_stream)
    delete flop_err_stream;

  if(flop_stat_sn)
    delete flop_stat_sn;

  if(::pipe(pipe_stat) < 0)
    throw tError("cannot pipe()");

  if(::pipe(pipe_err) < 0)
    throw tError("cannot pipe()");

  if((pid = ::fork()) < 0)
    {
      throw tError("cannot fork()");
    }
  else if(pid == 0) // child
    {
      if(pipe_stat[1] != 2)
        ::dup2(pipe_stat[1], 2);

      for(int i = _POSIX_OPEN_MAX; i >= 0; i--)
        if(i != 2 && i != pipe_err[1])
          ::close(i);

      QString arg1;
      arg1.sprintf("-errors-to=%d", pipe_err[1]);

      if(::execlp("flop", "flop", arg1.ascii(), fn,  0) == -1)
        {
          exit(1);
        }
    }
  else // parent
    {
      ::close(pipe_stat[1]);
      ::close(pipe_err[1]);

      file_stat = new QFile();
      file_err = new QFile();

      file_stat->open(IO_ReadOnly, pipe_stat[0]);
      file_err->open(IO_ReadOnly, pipe_err[0]);

      flop_stat_sn = new QSocketNotifier(pipe_stat[0], QSocketNotifier::Read);

      QObject::connect(flop_stat_sn,SIGNAL(activated(int)),
                       this, SLOT(data_received(int)));

      flop_stat_stream = new QTextStream(file_stat);
      flop_err_stream = new QTextStream(file_err);

      //      if(::waitpid(pid, &status, 0) == pid)
      //        fprintf(stderr, "got child exit status\n");
    }
}

void GoofyWindow::load_grammar(const char *fn)
{
  if(G)
    delete G;
  if(cheap_settings)
    delete cheap_settings;

  try {
    cheap_settings = new settings("cheap", fn);
    G = new tGrammar( fn );
  }

  catch(tError &e)
    {
      QMessageBox::warning(this, "Goofy",
                           QString("error loading `") + QString(fn)
                           + QString("':\n") + QString(e.getMessage().c_str())
                           + "\n\n", "Ok");

      return;
    }

  Grammar = G;

  setCaption( fn );
  set_menu_state(true);

  for(int i = 0; i < nstatictypes; ++i)
    {
      _sort_names.append(QString(type_name(i)));
    }
  _sort_names.sort();
  /*
  for(int i = first_instance; i < nsorts; ++i)
    {
      _inst_names.append(QString(type_name[i]));
    }
  _sort_names.sort();
  */
  for(rule_iter rules(Grammar); rules.valid(); ++rules)
    {
      _rule_names.append(QString(rules.current()->printname()));
    }
  _rule_names.sort();

  QString s;
  s.sprintf( "Loaded `%s'", fn );

  statusBar()->message(s);
}

// StringSelDialog

StringSelDialog::StringSelDialog(const QStringList &L, const QString &what)
  : QDialog(0, "string chooser", true, WStyle_Title), _selected()
{
  _list_box = new QListBox(this);

  _list_box->insertStringList(L);

  QPushButton *okbutton = new QPushButton("OK", this);
  QObject::connect(okbutton, SIGNAL(clicked()), this, SLOT(accept()));

  QPushButton *cancelbutton = new QPushButton("Cancel", this);
  QObject::connect(cancelbutton, SIGNAL(clicked()), this, SLOT(reject()));

  QVBoxLayout *vbox1 = new QVBoxLayout(this, 5);
  vbox1->addWidget(_list_box);

  QHBoxLayout *hbox1 = new QHBoxLayout(vbox1, 5);
  hbox1->addWidget(okbutton);
  hbox1->addWidget(cancelbutton);

  QObject::connect(_list_box, SIGNAL(highlighted(const QString &)), this, SLOT(setselected(const QString &)));

  setCaption("Select " + what);
}

// StringInputDialog

StringInputDialog::StringInputDialog(const QString &what)
  : QDialog(0, "string chooser", true, WStyle_Title), _sel()
{
  _edit = new QLineEdit(this);

  QPushButton *okbutton = new QPushButton("OK", this);
  QObject::connect(okbutton, SIGNAL(clicked()), this, SLOT(accept()));

  QPushButton *cancelbutton = new QPushButton("Cancel", this);
  QObject::connect(cancelbutton, SIGNAL(clicked()), this, SLOT(reject()));

  QVBoxLayout *vbox1 = new QVBoxLayout(this, 5);
  vbox1->addWidget(_edit);

  QHBoxLayout *hbox1 = new QHBoxLayout(vbox1, 5);
  hbox1->addWidget(okbutton);
  hbox1->addWidget(cancelbutton);

  setCaption("Input " + what);
}

// TFS Tool Tips

TFSTip::TFSTip(TFSView *parent)
  : QToolTip(parent)
{
  _tfsview = parent;
}

void TFSTip::maybeTip(const QPoint &p)
{
  TFSViewItem *it = dynamic_cast<TFSViewItem*>(_tfsview->itemAt(p));
  if(it)
    {
      QString s = it->tip();

      tip(_tfsview->itemRect(it), s);
    }
}

// TFSDrag

TFSDrag::TFSDrag(dag_node *dag, QWidget *parent, const char *name)
  : QStoredDrag("dag/cheap", parent, name )
{
  QByteArray data;
  QDataStream s(data, IO_WriteOnly);

  s << qApp->sessionId();
  s << (int) dag;

  setEncodedData( data );
}

bool TFSDrag::canDecode(QDropEvent* e)
{
  return e->provides("dag/cheap");
}

bool TFSDrag::decode(QDropEvent* e, dag_node *&dag )
{
  QByteArray payload = e->data("dag/cheap");
  QDataStream s(payload, IO_ReadOnly);

  QString sess; s >> sess;
  int addr; s >> addr;

  if(sess == qApp->sessionId())
    {
      dag = (dag_node *) addr;
      e->accept();
      return TRUE;
    }

  return FALSE;
}

QString path_string(list_int *p)
{
  QString s;
  bool f = true;

  while(p)
    {
      if(!f) s.append("."); else f = false;
      s.append(attrname[first(p)]);
      p = rest(p);
    }

  return s;
}

// TFSViewItems

int TFSViewItem::_next_id = 0;

TFSViewItem::TFSViewItem(QListView *parent, TFSViewItem *after
                         , dag_node *dag, dag_node *root
                         , int tag, int sort)
  : QListViewItem(parent, after), _s_attr(), _s_tag(), _s_sort()
{
  _id = _next_id++;

  _sort = sort;
  _tag = tag;

  _dag = dag;
  _root = root;

  _attr = -1;
  _parent = 0;
  _path = 0;

  _next_cref = 0;

  _failure = 0;

  _max_attr_width = 0;
  _popupmenu = 0;

  _tfs = dynamic_cast<TFSView *> (parent);

  setup_text();
}

TFSViewItem::TFSViewItem(TFSViewItem *parent, TFSViewItem *after, dag_node *dag, dag_node *root,
                         int attr, int tag, int sort)
  : QListViewItem(parent, after), _s_attr(), _s_tag(), _s_sort()
{
  _id = _next_id++;

  _sort = sort;
  _tag = tag;

  _dag = dag;
  _root = root;

  _attr = attr;
  _parent = parent;
  _path = append(copy_list(parent->path()), attr);
  _parent->setExpandable( TRUE );

  _next_cref = 0;

  _failure = 0;

  _max_attr_width = 0;
  _popupmenu = 0;

  _tfs = parent->_tfs;

  setup_text();
}

void TFSViewItem::setup_text()
{
  QListView *lv = listView();
  QFontMetrics fm = lv->fontMetrics();

  if(_failure)
    _color = QColor("red");
  else
    _color = lv->colorGroup().text();

  _w_inter = fm.width(" ");

  _w_attr = _w_tag = _w_sort = 0;
  _w_total = lv->itemMargin();

  if(_attr >= 0)
    {
      _s_attr = QString(attrname[_attr]);
      _w_attr = fm.boundingRect(0, 0, 10000, fm.height(), AlignVCenter, _s_attr).width();
      _w_total += _w_attr + _w_inter;
    }

  if(_tag >= 0)
    {
      _s_tag.setNum(_tag);
      _s_tag.prepend("#");

      if(_sort >= 0)
        {
          _s_tag.append(":");
          _w_total += _w_inter;
        }

      _w_tag = fm.boundingRect(0, 0, 10000, fm.height(), AlignVCenter, _s_tag).width();
      _w_total += _w_tag;
    }

  if(_sort >= 0 || _failure || _dag == FAIL)
    {
      if(_dag == FAIL)
        _s_sort = QString("(failed)");
      else if(_failure)
        {
          if(_failure->type() == unification_failure::CLASH)
            _s_sort = QString("(sort clash)");
          else if(_failure->type() == unification_failure::CONSTRAINT)
            _s_sort = QString("(constraint clash)");
          else if(_failure->type() == unification_failure::CYCLE)
            _s_sort = QString("(cycle)");
          else
            _s_sort = QString("(failed)");
        }
      else
        _s_sort = QString(type_name(_sort));

      _w_sort = fm.boundingRect(0, 0, 10000, fm.height(), AlignVCenter, _s_sort).width();
      _w_total += _w_sort;
    }

  _w_total += lv->itemMargin();

  if(_parent)
    _parent->tell_attr_width(_w_attr);
}

TFSViewItem::~TFSViewItem()
{
  if(_path)
    free_list(_path);
  if(_popupmenu)
    delete _popupmenu;
}

void TFSViewItem::register_coref(TFSViewItem *next)
{
  if(_next_cref)
    _next_cref->register_coref(next);
  else
    _next_cref = next;
}

TFSViewItem *TFSViewItem::deref()
{
  if(_tag >= 0)
    {
      if(_tfs)
        return _tfs->coref(_tag);
      else
        return 0;
    }
  else
    return this;
}

void TFSViewItem::popup(const QPoint &pos)
{
  if(_tag < 0) return;

  if(!_popupmenu)
    {
      _popupmenu = new QPopupMenu();

      connect(_popupmenu, SIGNAL(activated(int)), this, SLOT(goto_coref(int)));

      TFSViewItem *def = _tfs->coref(_tag);
      int i = 0;
      _popupmenu->insertItem(path_string(def->path()), i++);
      _popupmenu->insertSeparator();
      while((def = def->next_coref()))
        {
          _popupmenu->insertItem(path_string(def->path()), i++);
        }
    }

  _popupmenu->exec(pos);
}

void TFSViewItem::open()
{
  setOpen(TRUE);
  if(_parent)
    _parent->open();
}

void TFSViewItem::goto_coref(int id)
{
  TFSViewItem *c = _tfs->coref(_tag);
  int i = 0;
  while(i < id && c)
    c = c->next_coref(), i++;

  if(i == id)
    {
      c->open();
      _tfs->setSelected(c, true);
      _tfs->center(_tfs->contentsX(), _tfs->itemPos(c));
    }
}

QString TFSViewItem::tip()
{
  QString s = path_string(path());

  if(_failure)
    {
      s.append(": ");
      switch(_failure->type())
        {
        case unification_failure::CLASH:
          s.append("clash ");
          s.append(type_name(_failure->s1()));
          s.append(" & ");
          s.append(type_name(_failure->s2()));
          break;
        case unification_failure::CYCLE:
          {
            s.append("cycle");

            list<list_int *> paths = _failure->cyclic_paths();

            fprintf(stderr, "cyclic paths:\n");
            for(list<list_int *>::iterator pit = paths.begin()
                  ; pit != paths.end(); ++pit)
              {
                fprintf(stderr, "  ");
                print_path(stderr, *pit);
                fprintf(stderr, "\n");
              }

            break;
          }
        case unification_failure::CONSTRAINT:
          {
            s.append("constraint clash (");
            int meet = glb(_failure->s1(), _failure->s2());
            s.append(meet == -1 ? "bottom" : type_name(meet));
            s.append(")");
            break;
          }
        default:
          s.append("unknown failure");
          break;
        }
    }

  return s;
}

void TFSViewItem::set_failure(unification_failure *f)
{
  _failure = f;
  setup_text();
}

void TFSViewItem::indent_attr()
{
  if(!_parent) return;
  if(_parent->max_attr_width() != _w_attr)
    {
      _w_total += (_parent->max_attr_width() - _w_attr);
      _w_attr = _parent->max_attr_width();
      widthChanged();
    }
}

void TFSViewItem::setup()
{
  QListViewItem::setup();
}

QString TFSViewItem::text(int column) const
{
  return QString().setNum(_id);
}

void TFSViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int mwidth, int align)
{
  if(!p) return;

  if(column != 0)
    {
      QListViewItem::paintCell(p, cg, column, mwidth, align);
      return;
    }

  QListView *lv = listView();

  p->fillRect(0, 0, mwidth, height(), cg.base());

  int marg = lv ? lv->itemMargin() : 1;

  if(isSelected())
    {
      p->fillRect(0, 0, _w_total, height(), cg.brush(QColorGroup::Highlight));
      p->setPen(cg.highlightedText());
    }
  else
    {
      p->setPen(_color);
    }

  if(_w_attr > 0)
    {
      p->drawText( marg, 0,
                   _w_attr, height(), AlignVCenter, _s_attr);
    }

  if(_w_tag > 0)
    {
      p->drawText( marg + _w_attr + (_w_attr ? _w_inter : 0), 0,
                   _w_tag, height(), AlignVCenter, _s_tag);
    }

  if(_w_sort > 0)
    {
      p->drawText( marg + _w_attr + (_w_attr ? _w_inter : 0) + _w_tag + (_w_tag ? _w_inter : 0), 0,
                   _w_sort, height(), AlignVCenter, _s_sort);
    }
}

/*
void TFSViewItem::paintFocus(QPainter *p, const QColorGroup &cg, const QRect &r)
{
  QRect r1(r);
  r1.setWidth(_w_total);
  listView()->style().drawFocusRect( p, r1, cg, isSelected()? & cg.highlight() : & cg.base(), isSelected() );
}
*/

int TFSViewItem::width(const QFontMetrics &fm, const QListView *lv, int c) const
{
  return _w_total;
}

// TFSView

TFSView::TFSView(QWidget * parent, bool failure, const char * name)
  : QListView(parent, name), oldCurrent(0), dropItem(0), autoopen_timer(this), autoscroll_timer(this)
{
  _failure = failure;

  _ncorefs = _corefs_alloc = 0;
  _corefs = 0;

  if(!_failure)
    {
      setAcceptDrops(true);
      viewport()->setAcceptDrops(true);
    }

  _tip = new TFSTip(this);

  connect(&autoopen_timer, SIGNAL(timeout()), this, SLOT(openBranch()));
  connect(&autoscroll_timer, SIGNAL(timeout()), this, SLOT(autoScroll()));
}

TFSView::~TFSView()
{
}

void TFSView::new_coref(int tag, TFSViewItem *item)
{
  assert(tag >= 0);

  if(tag > _ncorefs) _ncorefs = tag;

  if(_ncorefs >= _corefs_alloc)
    {
      int i = _corefs_alloc;
      _corefs_alloc += 16;
      _corefs = (TFSViewItem **) realloc(_corefs, sizeof(TFSViewItem *) * _corefs_alloc);
      for(; i < _corefs_alloc; ++i)
        _corefs[i] = 0;
    }

  _corefs[_ncorefs++] = item;
}

void TFSView::register_coref(int tag, TFSViewItem *item)
{
  assert(tag >= 0 && tag <_ncorefs);
  assert(_corefs[tag] != 0);

  _corefs[tag]->register_coref(item);
}

TFSViewItem *TFSView::coref(int tag)
{
  assert(tag >= 0 && tag <_ncorefs);
  return _corefs[tag];
}

TFSViewItem *TFSView::path_value(list_int *path)
{
  TFSViewItem *v = dynamic_cast<TFSViewItem *> (firstChild());

  while(path)
    {
      TFSViewItem *c = dynamic_cast<TFSViewItem *> (v->firstChild());
      v = 0;

      while(c)
        {
          if(c -> attr() == first(path))
            {
              v = c->deref();
              break;
            }
          c = dynamic_cast<TFSViewItem *> (c->nextSibling());
        }

      if(!v) return 0;
      path = rest(path);
    }

  return v;
}

void TFSView::openBranch()
{
  autoopen_timer.stop();

  if(dropItem && !dropItem->isOpen())
    {
      dropItem->setOpen(TRUE);
      dropItem->repaint();
    }
}

static const int autoopenTime = 750;

void TFSView::contentsDragEnterEvent( QDragEnterEvent *e )
{
  if(!TFSDrag::canDecode(e))
    {
        e->ignore();
        return;
    }

    oldCurrent = currentItem();

    TFSViewItem *i = dynamic_cast<TFSViewItem*>(itemAt(contentsToViewport(e->pos())));
    if(i)
      {
        dropItem = i;
        autoopen_timer.start(autoopenTime);
      }
}

static const int autoscroll_margin = 16;
static const int initialScrollTime = 30;
static const int initialScrollAccel = 5;

void TFSView::startAutoScroll()
{
  if(!autoscroll_timer.isActive())
    {
      autoscroll_time = initialScrollTime;
      autoscroll_accel = initialScrollAccel;
      autoscroll_timer.start(autoscroll_time);
    }
}

void TFSView::stopAutoScroll()
{
  autoscroll_timer.stop();
}

void TFSView::autoScroll()
{
  QPoint p = viewport()->mapFromGlobal(QCursor::pos());

  if(autoscroll_accel-- <= 0 && autoscroll_time)
    {
      autoscroll_accel = initialScrollAccel;
      autoscroll_time--;
      autoscroll_timer.start(autoscroll_time);
    }

  int l = QMAX(1,(initialScrollTime-autoscroll_time));
  int dx = 0,dy = 0;

  if(p.y() < autoscroll_margin)
    dy = -l;
  else if (p.y() > visibleHeight()-autoscroll_margin)
    dy = +l;

  if(p.x() < autoscroll_margin)
    dx = -l;
  else if (p.x() > visibleWidth()-autoscroll_margin)
    dx = +l;

  if(dx||dy)
    scrollBy(dx,dy);
  else
    stopAutoScroll();
}

void TFSView::contentsDragMoveEvent( QDragMoveEvent *e )
{
  if(!TFSDrag::canDecode(e))
    {
      e->ignore();
      return;
    }

  QPoint vp = contentsToViewport(e->pos());
  QRect inside_margin(autoscroll_margin, autoscroll_margin,
                      visibleWidth()-autoscroll_margin*2,
                      visibleHeight()-autoscroll_margin*2);

  TFSViewItem *i = dynamic_cast<TFSViewItem *>(itemAt(vp));

  if(i)
    {
      setSelected(i,TRUE);
      if (!inside_margin.contains(vp))
        {
          startAutoScroll();
          e->accept(QRect(0,0,0,0)); // Keep sending move events
          autoopen_timer.stop();
        }
      else
        {
          e->accept();
          if(i != dropItem)
            {
              autoopen_timer.stop();
              dropItem = i;
              autoopen_timer.start(autoopenTime);
            }
        }

      switch ( e->action() )
        {
        case QDropEvent::Copy:
          e->acceptAction();
          break;
        case QDropEvent::Move:
        case QDropEvent::Link:
        default:
          break;
        }
    }
  else
    {
      e->ignore();
      autoopen_timer.stop();
      dropItem = 0L;
    }
}

void TFSView::contentsDragLeaveEvent( QDragLeaveEvent *e )
{
  autoopen_timer.stop();
  stopAutoScroll();
  dropItem = 0L;

  setCurrentItem(oldCurrent);
  setSelected(oldCurrent, TRUE);
}

void TFSView::do_unification(dag_node *root, dag_node *a, dag_node *b, list_int *p)
{
  struct dag_node *res;
  struct dag_alloc_state s;

  b = dag_full_copy(b);
  dag_invalidate_changes();

  dag_alloc_mark(s);
  res = dag_unify(root, a, b, 0);

  if(res == FAIL)
    {
      dag_alloc_release(s);

      dag_node *result = root;
      list<unification_failure *> fails = dag_unify_get_failures(a, b, true, p, &result);

      DagView *dagview = new DagView(result, fails);
      dagview->show();
    }
  else
    {
      DagView *dagview = new DagView(res, "unification result");
      dagview->show();
    }
}

void TFSView::contentsDropEvent( QDropEvent * e )
{
  autoopen_timer.stop();
  stopAutoScroll();

  if(!TFSDrag::canDecode(e))
    {
      e->ignore();
      return;
    }

  TFSViewItem *item = dynamic_cast<TFSViewItem *> (itemAt(contentsToViewport(e->pos())));
  if (item)
    {
      dag_node *dag;
      if(TFSDrag::decode( e, dag ))
        {
          if(e->action() == QDropEvent::Copy)
            e->acceptAction();

          e->accept();

          do_unification(item->root(), item->dag(), dag, item->path());
          return;
        }
      else
        e->ignore();
    }
  else
    e->ignore();
}

void TFSView::contentsMousePressEvent(QMouseEvent* e)
{
  if(e->button() == RightButton)
    {
      TFSViewItem *item = dynamic_cast<TFSViewItem *> (itemAt(contentsToViewport(e->pos())));
      if (item)
        {
          item->popup(QCursor::pos());
        }
    }
  else
    {
      QListView::contentsMousePressEvent(e);
      presspos = e->pos();
    }
}

void TFSView::contentsMouseMoveEvent(QMouseEvent* e)
{
  if(_failure)
    return;

  if((e->pos() - presspos).manhattanLength() > 4)
    {
      TFSViewItem *item = dynamic_cast<TFSViewItem*> (itemAt(contentsToViewport(presspos)));

      if (item)
        {
          TFSDrag *d = new TFSDrag(item->dag(), viewport());
          d->dragCopy();
        }
    }
}

static int crefnr;

TFSViewItem *dag_make_listview(TFSView *tfs, TFSViewItem *parent, TFSViewItem *after, int attr,
                               dag_node *dag, dag_node *root)
{
  int coref;
  struct dag_arc *arc;
  TFSViewItem *item;

  if(dag == FAIL)
    {
      if(attr >= 0)
        item = new TFSViewItem(parent, after, dag, root, attr);
      else
        item = new TFSViewItem(tfs, after, dag, root);

      return item;
    }

  dag = dag_deref(dag);
  coref = dag_get_visit(dag) - 1;

  if(coref < 0) // dag is coreferenced, already printed
    {
      if(attr >= 0)
        item = new TFSViewItem(parent, after, dag, root, attr, -(coref+1));
      else
        item = new TFSViewItem(tfs, after, dag, root, -(coref+1));

      tfs->register_coref(-(coref+1), item);

      return item;
    }
  else if(coref > 0) // dag is coreferenced, not printed yet
    {
      coref = -dag_set_visit(dag, -(crefnr++));
    }
  else
    coref = -1;

  if(attr >= 0)
    item = new TFSViewItem(parent, after, dag, root, attr, coref, dag->type);
  else
    item = new TFSViewItem(tfs, after, dag, root, coref, dag->type);

  if(coref >= 0)
    tfs->new_coref(coref, item);

  if((arc = dag->arcs))
    {
      struct dag_node **print_attrs = (struct dag_node **)
        malloc(sizeof(struct dag_node *) * nattrs);

      int i, maxatt = 0;

      for(i = 0; i < nattrs; ++i)
        print_attrs[i] = 0;

      while(arc)
        {
          print_attrs[arc->attr] = arc->val;
          maxatt = arc->attr > maxatt ? arc->attr : maxatt;
          arc=arc->next;
        }

      TFSViewItem *last = 0;
      for(int j = 0; j <= maxatt; ++j) if(print_attrs[j])
        {
          last = dag_make_listview(tfs, item, last, j, print_attrs[j], root);
        }

      item->setOpen(true);

#if 0
      QListViewItem *chld = item->firstChild();
      while(chld)
        {
          dynamic_cast<TFSViewItem*>(chld)->indent_attr();
          chld = chld->nextSibling();
        }
#endif

      free(print_attrs);
    }

  return item;
}

// DagView Dialog

DagView::DagView(dag_node *dag, QString caption)
  : QDialog(0, "dag viewer", false)
{
  _dag = dag;

  _tfs = new TFSView(this, false);

  _tfs->addColumn("");
  _tfs->setColumnWidthMode(0, QListView::Maximum);
  _tfs->header()->hide();
  _tfs->setSorting(-1);
  _tfs->setRootIsDecorated(true);
  _tfs->setItemMargin(2);

  dag_mark_coreferences(dag); crefnr = 1;
  dag_make_listview(_tfs, 0, 0, -1, dag, dag);
  dag_invalidate_changes();

  QVBoxLayout *vbox1 = new QVBoxLayout(this, 5, 5);
  vbox1->addWidget(_tfs);
  _tfs -> show();

  setCaption( caption );
}

void dag_mark_failure(TFSView *tfs, unification_failure *failure)
{
  TFSViewItem *it = tfs->path_value(failure->path());

  if(it)
    it -> set_failure(failure);
  else
    {
      fprintf(ferr, "cannot display ");
      failure->print(ferr);
      fprintf(ferr, "\n");
    }
}

DagView::DagView(dag_node *dag, list<unification_failure *> fails)
  : QDialog(0, "dag viewer", false)
{
  _dag = dag;

  _tfs = new TFSView(this, true);

  _tfs->addColumn("");
  _tfs->setColumnWidthMode(0, QListView::Maximum);
  _tfs->header()->hide();
  _tfs->setSorting(-1);
  _tfs->setRootIsDecorated(true);
  _tfs->setItemMargin(2);

  if(dag != FAIL)
    dag_mark_coreferences(dag);

  crefnr = 1;
  dag_make_listview(_tfs, 0, 0, -1, dag, dag);

  dag_invalidate_changes();

  for(list<unification_failure *>::iterator failure = fails.begin()
        ; failure != fails.end(); ++failure)
    dag_mark_failure(_tfs, *failure);

  QVBoxLayout *vbox1 = new QVBoxLayout(this, 5, 5);
  vbox1->addWidget(_tfs);
  _tfs -> show();

  setCaption("Failure Viewer");
}

