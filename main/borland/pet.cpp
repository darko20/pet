#pragma hdrstop
#include <condefs.h>

//---------------------------------------------------------------------------
USEUNIT("cheap\cheap.cpp");
USEUNIT("cheap\qc.cpp");
USEUNIT("cheap\chart.cpp");
USEUNIT("cheap\dag-tomabechi.cpp");
USEUNIT("cheap\errors.cpp");
USEUNIT("cheap\fs.cpp");
USEUNIT("cheap\grammar.cpp");
USEUNIT("cheap\item.cpp");
USEUNIT("cheap\options.cpp");
USEUNIT("cheap\parse.cpp");
USEUNIT("cheap\agenda.cpp");
USEUNIT("cheap\task.cpp");
USEUNIT("cheap\tokenlist.cpp");
USEUNIT("cheap\tsdb++.cpp");
USEUNIT("common\bitcode.cpp");
USEUNIT("common\mfile.c");
USEUNIT("common\dag-alloc.cpp");
USEUNIT("common\dag-arced.cpp");
USEUNIT("common\dag-common.cpp");
USEUNIT("common\dag-io.cpp");
USEUNIT("common\failure.cpp");
USEUNIT("common\grammar-dump.cpp");
USEUNIT("common\lex-tdl.cpp");
USEUNIT("common\settings.cpp");
USEUNIT("common\sorts.cpp");
USEUNIT("common\chunk-alloc.cpp");
USEUNIT("common\lex-io.cpp");
USEUNIT("cheap\k2y.cpp");
USELIB("C:\LEDA\libL.lib");
USEUNIT("cheap\mrs.cpp");
USEUNIT("cheap\yy.cpp");
//---------------------------------------------------------------------------
extern int real_main(int argc, char *argv[]);

#pragma argsused
int main(int argc, char* argv[])
{
        return real_main(argc, argv);
}
