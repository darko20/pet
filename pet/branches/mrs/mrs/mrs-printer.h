/* -*- Mode: C++ -*- */

#ifndef _MRS_PRINTER_H_
#define _MRS_PRINTER_H_

#include "pet-config.h"
#include "mrs.h"

#include <iostream>

void print_mrs_as(char format, dag_node *dag, std::ostream &out);

namespace mrs {

  class MRSPrinter {
  public:

    MRSPrinter(std::ostream &out) : _out(&out) { }

    virtual ~MRSPrinter() {
    }

    virtual void print(tMRS* mrs) = 0;

  protected:
    std::ostream *_out;
  };

  class MrxMRSPrinter : public MRSPrinter {
  public:
    MrxMRSPrinter(std::ostream &out) : MRSPrinter(out) { }

    virtual void print(tMRS* mrs);
    void print(tRel* rel);
    void print(tValue* val);
    void print_full(tValue* val);
    void print(tConstant* constant);
    void print(tVar* var);
    void print_full(tVar* var);
    void print(tHCons* hcons);
  };

  class SimpleMRSPrinter: public MRSPrinter {
  public:
    SimpleMRSPrinter(std::ostream &out) : MRSPrinter(out) { }

    virtual void print(tMRS* mrs);
    void print(tRel* rel);
    void print(tValue* val);
    void print_full(tValue* val);
    void print(tConstant* constant);
    void print(tVar* var);
    void print_full(tVar* var);
    void print(tHCons* hcons);

  private:
    std::map<int,tVar*> _vars;
  };

} // namespace mrs


#endif
