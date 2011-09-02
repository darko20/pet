/* -*- Mode: C++ -*- */

#ifndef _MRS_PRINTER_H_
#define _MRS_PRINTER_H_

#include "mrs.h"

#include <iostream>

namespace mrs {

  class MrsPrinter {
  public:

    MrsPrinter(std::ostream &out) : _out(&out) { }

    virtual ~MrsPrinter() {
    }

    virtual void print(tMrs* mrs) = 0;

  protected:
    std::ostream *_out;
  };

  class MrxMrsPrinter : public MrsPrinter {
  public:
    MrxMrsPrinter(std::ostream &out) : MrsPrinter(out) { }

    virtual void print(tMrs* mrs);
    void print(tEp* rel);
    void print(tValue* val);
    void print_full(tValue* val);
    void print(tConstant* constant);
    void print(tVar* var);
    void print_full(tVar* var);
    void print(tHCons* hcons);
  };

  class SimpleMrsPrinter: public MrsPrinter {
  public:
    SimpleMrsPrinter(std::ostream &out) : MrsPrinter(out) { }

    virtual void print(tMrs* mrs);
    void print(tEp* rel);
    void print(tValue* val);
    void print_full(tValue* val);
    void print(tConstant* constant);
    void print(tVar* var);
    void print_full(tVar* var);
    void print(tHCons* hcons);

  private:
    std::map<int,tVar*> _vars;
  };

  class HtmlMrsPrinter : public MrsPrinter {
  public:
    HtmlMrsPrinter(std::ostream &out) 
      : MrsPrinter(out), _inline_headers(false) { }
    HtmlMrsPrinter(std::ostream &out, bool _inline) 
      : MrsPrinter(out), _inline_headers(_inline) { }

    virtual void print(tMrs* mrs);
    void print(tEp* rel);
    void print(tValue* val);
    void print_full(tValue* val);
    void print(tConstant* constant);
    void print(tVar* var);
    void print_full(tVar* var);
    void print(tHCons* hcons);

  private:
    bool _inline_headers;

    std::string getHeader();
  };


} // namespace mrs


#endif
