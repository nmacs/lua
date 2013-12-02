#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/times.h>
#include <syslog.h>

//#define DEBUG 1

#if DEBUG
#  include <syslog.h>
#  define dprint(s, ...) { printf(s, ##__VA_ARGS__); /*usleep(10000);*/ }
//#  define dprint(s, ...) { fprintf(stderr, s, ##__VA_ARGS__); usleep(100000); }
#else
#  define dprint(...) while(0) {}
#endif

#define lscheduler_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "lcoco.h"
#include "ltimer.h"

#define time_after(a,b)  ((long)(b) - (long)(a) < 0)
#define time_before(a,b) time_after(b, a)

#define MAXEVENTS 128

struct wait_ctx {
	struct epoll_event *event;
	struct timer_list *timer;
	lua_State *L_thread;
	int fd;
	int suspended;
};

static int epoll_fd;
static size_t MS_PER_TICK;
static int exit_scheduler;

#if DEBUG
static void dump_timeouts()
{
	/*struct timeout_ctx *item;
	dprint("timeouts: head:%p\n", &timeout_head);
	for (item = timeout_head.next; item != &timeout_head; item = item->next) {
		dprint("timer:%p, expire:%u, next:%p, data:%p\n", item, item->expire, item->next, item->data);
	}
	dprint("done\n");*/
}
#else
#define dump_timeouts() while(0) {}
#endif

static int init_epoll()
{
	int ret = epoll_create1(0);
	if (ret < 0)
		return ret;
	epoll_fd = ret;
	return 0;
}

#if 0
static void insert_timeout(struct timeout_ctx* ctx)
{
	struct timeout_ctx *insert_before;
	
	dprint("insert_timeout: ctx:%p\n", ctx);
	
	for (insert_before = timeout_head.next; insert_before != &timeout_head; insert_before = insert_before->next) {
		if (time_before(ctx->expire, insert_before->expire))
			break;
	}

	ctx->next = insert_before;
	ctx->prev = insert_before->prev;
	insert_before->prev->next = ctx;
	insert_before->prev = ctx;
}

static void remove_timeout(struct timeout_ctx* ctx)
{
	dprint("remove_timeout ctx:%p\n", ctx);
	
	if (ctx == NULL || ctx->next == ctx)
		return;

	ctx->next->prev = ctx->prev;
	ctx->prev->next = ctx->next;
	ctx->prev = ctx;
	ctx->next = ctx;
}

static int get_timeout(void)
{
	clock_t now = times(0);
	struct timeout_ctx *first = timeout_head.next;
	if (first != &timeout_head) {
		long diff = (long)first->expire - (long)now;
		if (diff <= 0)
			return 0;
		return diff * MS_PER_TICK;
	}
	else
		return -1;
}
#endif

static void set_timeout(struct timer_list* timer, int timeout, void *data)
{
        unsigned long expire = (unsigned long)times(0) + ((unsigned long)timeout + (MS_PER_TICK-1)) / MS_PER_TICK;
        dprint("set_timeout: timeout:%i, expire:%lu\n", timeout, expire);
        set_timer(timer, expire, 0, data);
        add_timer(timer);
}

LUALIB_API int luaL_wait(lua_State *L, int fd, int write, int timeout)
{
	int status;
	int delfd = 0;
	struct wait_ctx ctx;
	struct timer_list timer;
	struct epoll_event event;

	dprint("luaL_wait: co:%p, ctx:%p, fd:%i, write:%i, timeout:%i\n", L, &ctx, fd, write, timeout);

	if (timeout >= 0) {
		set_timeout(&timer, timeout, &ctx);
		ctx.timer = &timer;
	}
	else
		ctx.timer = 0;
	ctx.L_thread = L;
	ctx.fd = fd;
	ctx.suspended = 0;

	if (fd >= 0) {
		int ret;
		event.events = write ? EPOLLOUT : EPOLLIN;
		event.data.ptr = &ctx;
		ctx.event = &event;
		ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
		if (ret) {
			if (errno == ENOENT) {
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)) {
					del_timer(ctx.timer);
					return -errno;
				}
				delfd = 1;
			}
			else {
				del_timer(ctx.timer);
				return -errno;
			}
		}
	}
	else
		ctx.event = NULL;
	
	lua_pushlightuserdata(L, (void*)&ctx);
	lua_pushthread(L);
	lua_settable(L, LUA_REGISTRYINDEX);
	luaCOCO_settls(L, (unsigned long)&ctx);

	dprint("yield: co:%p\n", L);
	lua_yield(L, 0);
	dprint("resumed: co:%p\n", L);
	status = luaL_checkinteger(L, -1);
	lua_pop(L, 1);
	
	luaCOCO_settls(L, 0);
	lua_pushlightuserdata(L, (void*)&ctx);
	lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	if (delfd) {
		struct epoll_event event = {0};
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
	}

	return status;
}

static int auxwait(lua_State *L, int fd, int write, int timeout)
{
	int ret = luaL_wait(L, fd, write, timeout);
	if (ret < 0) {
		lua_pushnil(L);
		lua_pushstring(L, "error");
		lua_pushfstring(L, "wait error: %d", ret);
		return 3;
	}
	else if (ret == 0) {
		lua_pushnil(L);
		lua_pushstring(L, "timeout");
		return 2;
	}

	lua_pushboolean(L, 1);
	lua_pushfstring(L, "status:%d", ret);

	return 2;
}

LUALIB_API int luaL_addfd(lua_State *L, int fd)
{
	struct epoll_event event = {0};
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event)) {
		return -errno;
	}
	return 0;
}

LUALIB_API int luaL_delfd(lua_State *L, int fd)
{
	struct epoll_event event = {0};
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event)) {
		return -errno;
	}
	return 0;
}

static void on_thread_error(lua_State *L, lua_State *co, const char* err)
{
        syslog(LOG_ERR, "SCHEDULER: thread[%p]: %s\n", co, err);
}

static int do_resume_thread(lua_State *L, lua_State *co, int nargs)
{
	int ret;
	dprint("do_resume_thread L:%p, co:%p, nargs:%i\n", L, co, nargs);
	lua_xmove(L, co, nargs);
	lua_setthis(co);
	dprint("do_resume_thread SP:%p\n", &ret);
	ret = lua_resume(co, nargs);
	dprint("do_resume_thread resume ret:%i\n", ret);
	lua_setthis(L);
	dprint("do_resume_thread 1\n");
	if (ret == LUA_YIELD) {
		dprint("do_resume_thread 2 top:%i\n", lua_gettop(co));
		//lua_pop(co, lua_gettop(co));
		dprint("do_resume_thread 3\n");
		return 0;
	}

	if (ret != 0) {
	        const char *error;
		dprint("do_resume_thread 4\n");
		lua_xmove(co, L, 1);
		on_thread_error(L, co, lua_tostring(L, -1));
	}
	else {
		dprint("do_resume_thread 5\n");
		lua_pushstring(L, "coroutine exited without errors");
		on_thread_error(L, co, "coroutine exited without errors");
	}
	return -1;
}

static int resume_thread(lua_State *L, struct wait_ctx *ctx, int status)
{
	lua_State *co = ctx->L_thread;
	dprint("resume_thread L:%p, co:%p, ctx:%p, status:%i, suspended:%i\n", 
	       L, co, ctx, status, ctx->suspended);
	if (ctx->suspended)
		return 0;
	if (ctx->timer)
	  del_timer(ctx->timer);
	lua_pushinteger(L, status);
	return do_resume_thread(L, co, 1);
}

static int l_loop(lua_State *L)
{
	struct epoll_event events[MAXEVENTS];
	unsigned long now;
	unsigned long timeout;
	int ret;
	struct list_base expired;
	struct list_base *item, *tmp;

	while (1) {
		if (exit_scheduler) {
			lua_pushfstring(L, "scheduler terminated");
			return 1;
		}
		
		dprint("loop: gettop:%i\n", lua_gettop(L));
		now = (unsigned long)times(0);
		timeout = get_next_timeout(now) * MS_PER_TICK;
		dprint("loop: wait for events timeout:%lu\n", timeout);
		ret = epoll_wait(epoll_fd, events, MAXEVENTS, timeout);
		dprint("loop: wait ret:%i\n", ret);
		if (ret > 0) {
			int i;
			for (i = 0; i < ret; i++) {
				struct wait_ctx *ctx = events[i].data.ptr;
				int status = events[i].events & (EPOLLHUP | EPOLLERR) ? -1 : 1;
				if (resume_thread(L, ctx, status))
					lua_pop(L, 1);
			}
		}
		else if (ret < 0 && errno != EINTR) {
			lua_pushfstring(L, "loop error:%d\n", errno);
			return 1;
		}

		now = (unsigned long)times(0);
		//dprint("loop: now:%lu\n", now);
		list_init(&expired);
		process_timers_ex(&expired, now);
		list_for_each_safe(&expired, item, tmp) {
		        struct timer_list *timer = container_of(item, struct timer_list, list);
		        if (resume_thread(L, (struct wait_ctx*)timer->data, 0))
                                lua_pop(L, 1);
		}
	}

	return 0;
}

static int l_wait(lua_State *L)
{
	int fd = luaL_optinteger(L, 1, -1);
	static const char* operations[] = {"read", "write", NULL};
	int op = luaL_checkoption(L, 2, "read", operations);
	int timeout = luaL_optinteger(L, 3, -1);
	return auxwait(L, fd, op, timeout);
}

static int l_msleep(lua_State *L)
{
	int timeout = luaL_optinteger(L, 1, -1);
	return auxwait(L, -1, 0, timeout);
}

static int l_run(lua_State *L)
{
	lua_State *co = lua_tothread(L, 1);
	luaL_argcheck(L, co, 1, "coroutine expected");
	if (do_resume_thread(L, co, lua_gettop(L) - 1)) {
		lua_pushnil(L);
		lua_pushvalue(L, -2);
		return 2;
	}
	else {
		lua_pushboolean(L, 1);
		return 1;
	}
}

static int l_addfd(lua_State *L)
{
	int fd = luaL_checkinteger(L, 1);
	int ret = luaL_addfd(L, fd);
	if (ret < 0) {
		lua_pushnil(L);
		lua_pushfstring(L, "addfd error:%d", -ret);
		return 2;
	}
	
	lua_pushboolean(L, 1);
	return 1;
}

static int l_delfd(lua_State *L)
{
	int fd = luaL_checkinteger(L, 1);
	int ret = luaL_delfd(L, fd);
	if (ret < 0) {
		lua_pushnil(L);
		lua_pushfstring(L, "delfd error:%d", -ret);
		return 2;
	}
	
	lua_pushboolean(L, 1);
	return 1;
}

static int suspend_thread(lua_State *L, lua_State *co)
{
	struct wait_ctx *ctx;
	struct epoll_event event = {0};
	
	if (luaCOCO_gettls(co, (unsigned long*)&ctx))
		luaL_argcheck(L, 0, 1, "alive c-coroutine expected");
	
	dprint("suspend_thread: co:%p, ctx:%p\n", co, ctx);
	
	if (ctx == NULL || ctx->suspended)
		return 0;
	
	ctx->suspended = 1;
	if (ctx->timer) {
		dprint("suspend_thread: remove timeout:%p\n", ctx->timer);
		del_timer(ctx->timer);
	}
	
	if (ctx->fd >= 0)
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ctx->fd, &event);
	
	return 1;
}

static int l_suspend_thread(lua_State *L)
{
	int ret;
	lua_State *co = lua_tothread(L, 1);
	luaL_argcheck(L, co, 1, "coroutine expected");
	ret = suspend_thread(L, co);
	lua_pushboolean(L, ret ? 1 : 0);
	return 1;
}

static int l_resume_thread(lua_State *L)
{
	struct wait_ctx *ctx;
	lua_State *co = lua_tothread(L, 1);
	luaL_argcheck(L, co, 1, "coroutine expected");
	if (luaCOCO_gettls(co, (unsigned long*)&ctx))
		luaL_argcheck(L, 0, 1, "alive c-coroutine expected");

	if (ctx == NULL || !ctx->suspended) {
		lua_pushboolean(L, 0);
		return 1;
	}

	ctx->suspended = 0;

	if (ctx->timer)
		add_timer(ctx->timer);

	if (ctx->event) {
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, ctx->fd, ctx->event)) {
			luaL_error(L, "epoll_clt error:%d", errno);
		}
	}
	
	lua_pushboolean(L, 1);
	return 1;
}

static int l_cancel_wait(lua_State *L)
{
	struct wait_ctx *ctx;
	lua_State *co = lua_tothread(L, 1);
	luaL_argcheck(L, co, 1, "coroutine expected");
	if (luaCOCO_gettls(co, (unsigned long*)&ctx))
		luaL_argcheck(L, 0, 1, "alive c-coroutine expected");

	if (ctx == NULL) {
		lua_pushboolean(L, 0);
		return 1;
	}

	suspend_thread(L, co);
	lua_pushinteger(L, -1);
	do_resume_thread(L, co, 1);

	lua_pushboolean(L, 1);
	return 1;
}

static int l_exit(lua_State *L)
{
	exit_scheduler = 1;
	return 0;
}

static const luaL_Reg lib[] = {
	{"wait",   l_wait},
	{"cancel_wait", l_cancel_wait},
	{"msleep", l_msleep},
	{"loop",   l_loop},
	{"run",    l_run},
	{"addfd",  l_addfd},
	{"delfd",  l_delfd},
	{"suspend_thread", l_suspend_thread},
	{"resume_thread",  l_resume_thread},
	{"exit", l_exit},
  {NULL, NULL}
};

/* }====================================================== */

LUALIB_API int luaopen_scheduler (lua_State *L)
{
	lua_setthis(L);
	exit_scheduler = 0;
	MS_PER_TICK = 1000UL / sysconf(_SC_CLK_TCK);
	init_timers((unsigned long)times(0));
	if (init_epoll())
		return 0;
	luaL_register(L, LUA_SCHEDULERLIBNAME, lib);
	return 1;
}
