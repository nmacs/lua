/*
** $Id: linit.c,v 1.14.1.1 2007/12/27 13:02:25 roberto Exp $
** Initialization of libraries for lua.c
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"

#ifdef AUTOCONF
#  include "autoconf.h"
#endif

#ifdef LUA_STATIC_MODULES

#ifdef CONFIG_LIB_LUA_LUAFILESYSTEM
int luaopen_lfs (lua_State *L);
#endif

#ifdef CONFIG_LIB_LUA_LUASOCKET
int luaopen_socket_core(lua_State *L);
#endif

static const luaL_Reg modules[] = {
#ifdef CONFIG_LIB_LUA_LUAFILESYSTEM
  {"lfs", luaopen_lfs},
#endif
#ifdef CONFIG_LIB_LUA_LUASOCKET
	{"socket.core", luaopen_socket_core},
#endif
	{NULL, NULL}
};

LUALIB_API void luaL_preload_static_modules (lua_State *L) {
  const luaL_Reg *module = modules;

  lua_getfield(L, LUA_GLOBALSINDEX, "package");
  if (!lua_istable(L, -1))
    luaL_error(L, LUA_QL("package") " must be a table");

  lua_getfield(L, -1, "preload");
  if (!lua_istable(L, -1))
    luaL_error(L, LUA_QL("package.preload") " must be a table");

  for (; module->func; module++) {
    lua_pushstring(L, module->name);
    lua_pushcfunction(L, module->func);
    lua_settable(L, -3);
  }
}

#endif
