/**
* lua module injector header in uvm
* @author
*/

#ifndef lcompile_h
#define lcompile_h

#include "uvm/lprefix.h"


#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "uvm/lua.h"
#include "uvm/lobject.h"

void luaL_PrintFunctionToFile(FILE *out, const Proto* f, int full);
void luaL_PrintFunction(const Proto* f, int full);
void luaL_PrintConstant(FILE *out, const Proto* f, int i);
void luaL_PrintDebug(FILE *out, const Proto* f);
void luaL_PrintHeader(FILE *out, const Proto* f);
void luaL_PrintCode(FILE *out, const Proto* f);
void luaL_PrintString(FILE *out, const TString* ts);

#define luaU_print	luaL_PrintFunction
#define luaU_fprint luaL_PrintFunctionToFile

#define PROGNAME	"luac"		/* default program name */
#define OUTPUT		PROGNAME ".out"	/* default output file */

#endif