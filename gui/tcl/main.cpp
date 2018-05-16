#include "tk.h"
#include <string>
extern "C" int Nstclgui_Init(Tcl_Interp *interp);

bool checkError(int status, Tcl_Interp *interp)
{
   if( status == TCL_OK ) return false;
   printf("ERROR: %s\n", interp->result);
   return true;
}

std::string rootPath;

int Tcl_AppInit(Tcl_Interp *interp)
{
   std::string tmp = rootPath;
   printf("Tcl_Init\n");
   int status = Tcl_Init(interp);
   if( checkError(status, interp) ) return TCL_ERROR;
   tmp += "library/auto.tcl";
   status = Tcl_EvalFile(interp, tmp.c_str());
   if( checkError(status, interp) ) return TCL_ERROR;

   printf("Tk_Init\n");
   status = Tk_Init(interp);
   if( checkError(status, interp) ) return TCL_ERROR;
   tmp = rootPath + "library/optMenu.tcl";
   status = Tcl_EvalFile(interp, tmp.c_str());

   Nstclgui_Init(interp);
   printf("Tcl_EvalFile\n");
   tmp = rootPath + "stars.tcl";
	Tcl_Eval(interp, "set isExe 1");
   status = Tcl_EvalFile(interp, tmp.c_str());
   if( checkError(status, interp) ) return TCL_ERROR;
   return TCL_OK;
}

int main(int argc, char **argv)
{
   unsigned newLen = std::string(argv[0]).find_last_of("/\\");
   char *tmpPath = new char[newLen+1];
   strncpy(tmpPath, argv[0], newLen);
   tmpPath[newLen] = 0;
   rootPath = tmpPath;
   rootPath += '/';
   delete[] tmpPath;

   printf("Main\n");
   /*Tcl_Interp *interp = Tcl_CreateInterp();
   if( !interp ) return -1;

   int status = Tk_Init(interp);
   if( status != TCL_OK ) return -1;

   status = Tcl_EvalFile(interp, "code.tcl");
   if( status != TCL_OK ) return -1;*/

   Tk_Main(argc, argv, Tcl_AppInit);
   return 0;
}

