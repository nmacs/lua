.EXPORT_ALL_VARIABLES:

ifndef HOSTBUILD
LUA_STACK_SIZE    := 65536
CFLAGS            += -Wl,-elf2flt="-s$(LUA_STACK_SIZE)"
endif

ifdef CONFIG_LIB_CYASSL
CAYSSL            := $(ROOTDIR)/lib/cyassl/output
SSL_CFLAGS        := -Wl,-lcyassl -L$(CAYSSL)/lib -I$(CAYSSL)/include
else
SSL_CFLAGS        := -Wl,-lssl -Wl,-lcrypto
endif

LUA_DIR           := lua-5.1.5
LUAFILESYSTEM_DIR := luafilesystem-1.5.0
LUASOCKET_DIR     := luasocket-2.0.2
LUACOXPCALL_DIR   := coxpcall-1.13
LUAXAVANTE_DIR    := xavante-2.2.1
JSON_DIR          := json4lua-0.9.50
SQLITE_DIR        := lsqlite3_svn08
UCILUA_DIR        := libuci
BITSTRING_DIR     := bitstring-1.0
LSYSLOG_DIR       := lsyslog
LSIGNALS_DIR      := lsignals
LWATCHDOG_DIR     := lwatchdog
LAIO_DIR          := laio
LNTP_DIR          := lntp
LUASEC_DIR        := luasec
LCRYPTO_DIR       := lcrypto
LUATWITTER_DIR    := LuaTwitter-0.9.2
SMSPDU_DIR        := smspdu
LCRON_DIR         := lcron
MQUEUE_DIR        := mqueue
MESSAGEPACK_DIR   := MessagePack
LUAEX_DIR         := lua-ex

USE_SCHEDULER     := 1

LUA_INC           := "-I$(CURDIR)/$(LUA_DIR)/src"
CFLAGS            += $(LUA_INC) -DAUTOCONF -DLUA_STATIC_MODULES -DCOCO_MIN_CSTACKSIZE=1024
ifdef HOSTBUILD
ifndef FOR_WINDOWS
#CFLAGS            += -DUSE_VALGRIND=1
endif
CFLAGS            += -g
endif

lua_libs =

ifdef CONFIG_LIB_LUA_LUAFILESYSTEM
	CFLAGS          += -Wl,-llfs -L$(CURDIR)/$(LUAFILESYSTEM_DIR)/src
	lua_libs        += luafilesystem
endif

ifdef CONFIG_LIB_LUA_LUASOCKET
	ifdef USE_SCHEDULER
		CFLAGS  += -DSOCKET_SCHEDULER=1
	endif
	CFLAGS          += -Wl,-lsocket -Wl,-lmime -L$(CURDIR)/$(LUASOCKET_DIR)/src
	lua_libs        += luasocket
	ifdef FOR_WINDOWS
		CFLAGS      += -Wl,-lws2_32
	endif
endif

ifdef CONFIG_LIB_LUA_SQLITE
	CFLAGS          += -Wl,-llsqlite3 -Wl,-lsqlite3 -L$(CURDIR)/$(SQLITE_DIR)
	lua_libs        += luasqlite
endif

ifdef CONFIG_LIB_LUA_UCI
	CFLAGS          += -Wl,-lucilua -Wl,-luci -L$(CURDIR)/$(UCILUA_DIR)
	lua_libs        += luauci
endif

ifdef CONFIG_LIB_LUA_BITSTRING
	CFLAGS          += -Wl,-lbitstring -L$(CURDIR)/$(BITSTRING_DIR)/src/bitstring/.libs
	lua_libs        += luabitstring
endif

ifdef CONFIG_LIB_LUA_LSYSLOG
	CFLAGS          += -Wl,-llsyslog -L$(CURDIR)/$(LSYSLOG_DIR)
	lua_libs        += lsyslog
endif

ifdef CONFIG_LIB_LUA_LSIGNALS
	CFLAGS          += -Wl,-llsignals -L$(CURDIR)/$(LSIGNALS_DIR)
	lua_libs        += lsignals
endif

ifdef CONFIG_LIB_LUA_LWATCHDOG
	CFLAGS          += -Wl,-llwatchdog -L$(CURDIR)/$(LWATCHDOG_DIR)
	lua_libs        += lwatchdog
endif

ifdef CONFIG_LIB_LUA_LAIO
	CFLAGS          += -Wl,-llaio -L$(CURDIR)/$(LAIO_DIR)
	lua_libs        += laio
endif

ifdef CONFIG_LIB_LUA_LNTP
	CFLAGS          += -Wl,-llntp -L$(CURDIR)/$(LNTP_DIR)
	lua_libs        += lntp
endif

ifdef CONFIG_LIB_LUA_LUASEC
	ifdef USE_SCHEDULER
		CFLAGS  += -DSOCKET_SCHEDULER=1
	endif
	CFLAGS          += -Wl,-lluasec -L$(CURDIR)/$(LUASEC_DIR) -I$(CURDIR)/$(LUASOCKET_DIR)/src $(SSL_CFLAGS)
	lua_libs        += luasec
endif

ifdef CONFIG_LIB_LUA_LCRYPTO
	CFLAGS          += -Wl,-llcrypto -L$(CURDIR)/$(LCRYPTO_DIR) -I$(CURDIR)/$(LCRYPTO_DIR)/src $(SSL_CFLAGS)
	lua_libs        += lcrypto
endif

ifdef CONFIG_LIB_LUA_SMSPDU
	CFLAGS          += -Wl,-lsmspdu -L$(CURDIR)/$(SMSPDU_DIR)
	lua_libs        += smspdu
endif

ifdef CONFIG_LIB_LUA_CRON
	CFLAGS          += -Wl,-llcron -L$(CURDIR)/$(LCRON_DIR)
	lua_libs        += lcron
endif

ifdef CONFIG_LIB_LUA_LUAEX
	CFLAGS          += -Wl,-lex -L$(CURDIR)/$(LUAEX_DIR)
	lua_libs        += luaex
endif

.PHONY: all lua repo romfs

all: lua

lua: $(LUA_DIR)/src/autoconf.h $(lua_libs)
	$(MAKE) -C $(LUA_DIR)/src MYCFLAGS="-DCOCO_MIN_CSTACKSIZE=1024" all

.PHONY: lua_x86
lua_x86:
	mkdir -p $(LUA_DIR)-x86
	$(MAKE) -C $(LUA_DIR)-x86 -f $(CURDIR)/$(LUA_DIR)/src/Makefile \
		SRC_DIR=$(CURDIR)/$(LUA_DIR)/src MYCFLAGS="-DLUA_USE_POSIX -m32" \
		CC=gcc RANLIB=ranlib AR=ar all

$(LUA_DIR)/src/autoconf.h:
	ln -sf $(ROOTDIR)/config/autoconf.h $(LUA_DIR)/src/autoconf.h

.PHONY: luafilesystem
luafilesystem:
	$(MAKE) -C $(LUAFILESYSTEM_DIR) static

.PHONY: luasocket
luasocket:
	$(MAKE) -C $(LUASOCKET_DIR)/src libsocket.a libmime.a

.PHONY: luasqlite
luasqlite:
	$(MAKE) -C $(SQLITE_DIR) liblsqlite3.a

.PHONY: luauci
luauci:
	$(MAKE) -C $(UCILUA_DIR) static

.PHONY: luabitstring
luabitstring: $(BITSTRING_DIR)/Makefile
	$(MAKE) -C $(BITSTRING_DIR)

$(BITSTRING_DIR)/Makefile: Makefile
	cd $(BITSTRING_DIR) && ./configure --disable-shared --enable-static

.PHONY: lsyslog
lsyslog: $(LSYSLOG_DIR)/Makefile
	$(MAKE) -C $(LSYSLOG_DIR)

.PHONY: lsignals
lsignals: $(LSIGNALS_DIR)/Makefile
	$(MAKE) -C $(LSIGNALS_DIR)

.PHONY: lwatchdog
lwatchdog: $(LWATCHDOG_DIR)/Makefile
	$(MAKE) -C $(LWATCHDOG_DIR)

.PHONY: laio
laio: $(LAIO_DIR)/Makefile
	$(MAKE) -C $(LAIO_DIR)

.PHONY: lntp
lntp: $(LNTP_DIR)/Makefile
	$(MAKE) -C $(LNTP_DIR)

.PHONY: luasec
luasec: $(LUASEC_DIR)/Makefile
	$(MAKE) -C $(LUASEC_DIR)

.PHONY: lcrypto
lcrypto: $(LCRYPTO_DIR)/Makefile
	$(MAKE) -C $(LCRYPTO_DIR)

.PHONY: smspdu
smspdu: $(SMSPDU_DIR)/Makefile
	$(MAKE) -C $(SMSPDU_DIR)

.PHONY: lcron
lcron: $(LCRON_DIR)/Makefile
	$(MAKE) -C $(LCRON_DIR)

.PHONY: luaex
luaex: $(LUAEX_DIR)/Makefile
	$(MAKE) -C $(LUAEX_DIR)

############################################################################

clean:
	-$(MAKE) -C $(LUA_DIR) clean
	-$(MAKE) -C $(LUAFILESYSTEM_DIR) clean
	-$(MAKE) -C $(LUASOCKET_DIR) clean
	-$(MAKE) -C $(SQLITE_DIR) clean
	-$(MAKE) -C $(UCILUA_DIR) clean
	-$(MAKE) -C $(BITSTRING_DIR) clean
	-rm -f $(BITSTRING_DIR)/Makefile
	-$(MAKE) -C $(LSYSLOG_DIR) clean
	-$(MAKE) -C $(LSIGNALS_DIR) clean
	-$(MAKE) -C $(LWATCHDOG_DIR) clean
	-$(MAKE) -C $(LAIO_DIR) clean
	-$(MAKE) -C $(LNTP_DIR) clean
	-$(MAKE) -C $(LUASEC_DIR) clean
	-$(MAKE) -C $(LCRYPTO_DIR) clean
	-$(MAKE) -C $(SMSPDU_DIR) clean
	-$(MAKE) -C $(LCRON_DIR) clean
	-$(MAKE) -C $(LUAEX_DIR) clean
	-rm -rf $(LUA_DIR)-x86
	-rm -rf $(LUA_DIR)-native
	-rm -f $(LUA_DIR)/src/autoconf.h

romfs:
	$(ROMFSINST) -e CONFIG_LIB_LUA_SHELL -d $(LUA_DIR)/src/lua /bin/lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASOCKET -d $(LUASOCKET_DIR)/src/socket.lua /usr/local/share/lua/5.1/socket.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASOCKET -d $(LUASOCKET_DIR)/src/http.lua /usr/local/share/lua/5.1/socket/http.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASOCKET -d $(LUASOCKET_DIR)/src/wget.lua /usr/local/share/lua/5.1/socket/wget.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASOCKET -d $(LUASOCKET_DIR)/src/url.lua /usr/local/share/lua/5.1/socket/url.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASOCKET -d $(LUASOCKET_DIR)/src/ltn12.lua /usr/local/share/lua/5.1/ltn12.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASOCKET -d $(LUASOCKET_DIR)/src/mime.lua /usr/local/share/lua/5.1/mime.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASOCKET -d $(LUASOCKET_DIR)/etc/cosocket.lua /usr/local/share/lua/5.1/socket/cosocket.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASOCKET -d $(LUASOCKET_DIR)/src/smtp.lua /usr/local/share/lua/5.1/socket/smtp.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASOCKET -d $(LUASOCKET_DIR)/src/tp.lua /usr/local/share/lua/5.1/socket/tp.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUAXAVANTE -d $(LUAXAVANTE_DIR)/src/xavante/xavante.lua /usr/local/share/lua/5.1/xavante.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUAXAVANTE -d $(LUAXAVANTE_DIR)/src/xavante /usr/local/share/lua/5.1/xavante
	$(ROMFSINST) -e CONFIG_LIB_LUA_JSON -d $(JSON_DIR)/json/json.lua /usr/local/share/lua/5.1/json.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_JSON -d $(JSON_DIR)/json/jsonrpc.lua /usr/local/share/lua/5.1/jsonrpc.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_JSON -d $(JSON_DIR)/json/rpc.lua /usr/local/share/lua/5.1/json/rpc.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASEC -d $(LUASEC_DIR)/ssl.lua /usr/local/share/lua/5.1/ssl.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUASEC -d $(LUASEC_DIR)/https.lua /usr/local/share/lua/5.1/ssl/https.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUATWITTER -d $(LUATWITTER_DIR)/twitter.lua /usr/local/share/lua/5.1/twitter.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_LUATWITTER -d $(LUATWITTER_DIR)/oauth.lua /usr/local/share/lua/5.1/oauth.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_CRON -d $(LCRON_DIR)/cron.lua /usr/local/share/lua/5.1/cron.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_MQUEUE -d $(MQUEUE_DIR)/mqueue.lua /usr/local/share/lua/5.1/mqueue.lua
	$(ROMFSINST) -e CONFIG_LIB_LUA_MESSAGEPACK -d $(MESSAGEPACK_DIR)/MessagePack.lua /usr/local/share/lua/5.1/MessagePack.lua

repo:
	$(REPOINST) -e CONFIG_LIB_LUA_SHELL $(LUA_DIR)/src/lua /bin/lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASOCKET $(LUASOCKET_DIR)/src/socket.lua /usr/local/share/lua/5.1/socket.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASOCKET $(LUASOCKET_DIR)/src/http.lua /usr/local/share/lua/5.1/socket/http.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASOCKET $(LUASOCKET_DIR)/src/wget.lua /usr/local/share/lua/5.1/socket/wget.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASOCKET $(LUASOCKET_DIR)/src/url.lua /usr/local/share/lua/5.1/socket/url.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASOCKET $(LUASOCKET_DIR)/src/ltn12.lua /usr/local/share/lua/5.1/ltn12.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASOCKET $(LUASOCKET_DIR)/src/mime.lua /usr/local/share/lua/5.1/mime.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASOCKET $(LUASOCKET_DIR)/etc/cosocket.lua /usr/local/share/lua/5.1/socket/cosocket.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASOCKET $(LUASOCKET_DIR)/src/smtp.lua /usr/local/share/lua/5.1/socket/smtp.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASOCKET $(LUASOCKET_DIR)/src/tp.lua /usr/local/share/lua/5.1/socket/tp.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUAXAVANTE $(LUAXAVANTE_DIR)/src/xavante/xavante.lua /usr/local/share/lua/5.1/xavante.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUAXAVANTE $(LUAXAVANTE_DIR)/src/xavante /usr/local/share/lua/5.1/xavante
	$(REPOINST) -e CONFIG_LIB_LUA_JSON $(JSON_DIR)/json/json.lua /usr/local/share/lua/5.1/json.lua
	$(REPOINST) -e CONFIG_LIB_LUA_JSON $(JSON_DIR)/json/jsonrpc.lua /usr/local/share/lua/5.1/jsonrpc.lua
	$(REPOINST) -e CONFIG_LIB_LUA_JSON -d $(JSON_DIR)/json/rpc.lua /usr/local/share/lua/5.1/json/rpc.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASEC $(LUASEC_DIR)/ssl.lua /usr/local/share/lua/5.1/ssl.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUASEC $(LUASEC_DIR)/https.lua /usr/local/share/lua/5.1/ssl/https.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUATWITTER $(LUATWITTER_DIR)/twitter.lua /usr/local/share/lua/5.1/twitter.lua
	$(REPOINST) -e CONFIG_LIB_LUA_LUATWITTER $(LUATWITTER_DIR)/oauth.lua /usr/local/share/lua/5.1/oauth.lua
	$(REPOINST) -e CONFIG_LIB_LUA_CRON $(LCRON_DIR)/cron.lua /usr/local/share/lua/5.1/cron.lua
	$(REPOINST) -e CONFIG_LIB_LUA_MQUEUE $(MQUEUE_DIR)/mqueue.lua /usr/local/share/lua/5.1/mqueue.lua
	$(REPOINST) -e CONFIG_LIB_LUA_MESSAGEPACK $(MESSAGEPACK_DIR)/MessagePack.lua /usr/local/share/lua/5.1/MessagePack.lua
	lua-compile $(CONTENT)

romfs_user:

