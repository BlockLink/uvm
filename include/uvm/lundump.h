/*
** $Id: lundump.h,v 1.45 2015/09/08 15:41:05 roberto Exp $
** load precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#ifndef lundump_h
#define lundump_h

#include "uvm/llimits.h"
#include "uvm/lobject.h"
#include "uvm/lzio.h"


/* data to catch conversion errors */
#define LUAC_DATA	"\x19\x93\r\n\x1a\n"

#define LUAC_INT	0x5678
#define LUAC_NUM	cast_num(370.5)

#define MYINT(s)	(s[0]-'0')
#define LUAC_VERSION	(MYINT(LUA_VERSION_MAJOR)*16+MYINT(LUA_VERSION_MINOR))
#define LUAC_FORMAT	0	/* this is the official format */

/* load one chunk; from lundump.c */
LUAI_FUNC uvm_types::GcLClosure* luaU_undump(lua_State* L, ZIO* Z, const char* name);

/* dump one chunk; from ldump.c */
LUAI_FUNC int luaU_dump(lua_State* L, const uvm_types::GcProto* f, lua_Writer w,
    void* data, int strip);

LUAI_FUNC bool luaU_addMeter(lua_State* L, uvm_types::GcProto * p, bool isRecurse);

#endif
