--- src/event-loop.c
+++ src/event-loop.c
@@ -32,17 +32,17 @@
 #include <fcntl.h>
 #include <sys/socket.h>
 #include <sys/un.h>
-#include <sys/epoll.h>
-#include <sys/signalfd.h>
-#include <sys/timerfd.h>
 #include <unistd.h>
 #include <assert.h>
+#include <event2/event.h>
+#include <event2/thread.h>
 #include "wayland-private.h"
 #include "wayland-server.h"
 #include "wayland-os.h"
 
 struct wl_event_loop {
-	int epoll_fd;
+	struct event_base *evbase;
+//	int epoll_fd;
 	struct wl_list check_list;
 	struct wl_list idle_list;
 	struct wl_list destroy_list;
@@ -51,8 +51,10 @@ struct wl_event_loop {
 };
 
 struct wl_event_source_interface {
-	int (*dispatch)(struct wl_event_source *source,
-			struct epoll_event *ep);
+//	int (*dispatch)(struct wl_event_source *source,
+//			struct epoll_event *ep);
+	int (*dispatch)(evutil_socket_t fd, short what,
+			void *arg);
 };
 
 struct wl_event_source {
@@ -60,6 +62,7 @@ struct wl_event_source {
 	struct wl_event_loop *loop;
 	struct wl_list link;
 	void *data;
+	struct event *ev;
 	int fd;
 };
 
@@ -69,6 +72,15 @@ struct wl_event_source_fd {
 	int fd;
 };
 
+static void
+wl_event_source_libevent_dispatch(evutil_socket_t fd, short what, void *arg)
+{
+	struct wl_event_source *source = (struct wl_event_source *) arg;
+
+	(void)source->interface->dispatch(fd, what, arg);
+}
+
+#if 0
 static int
 wl_event_source_fd_dispatch(struct wl_event_source *source,
 			    struct epoll_event *ep)
@@ -88,11 +100,38 @@ wl_event_source_fd_dispatch(struct wl_event_source *source,
 
 	return fd_source->func(fd_source->fd, mask, source->data);
 }
+#endif
+
+static int
+wl_event_source_fd_dispatch(evutil_socket_t fd, short what, void *arg)
+{
+	struct wl_event_source *source = (struct wl_event_source *) arg;
+	struct wl_event_source_fd *fd_source = (struct wl_event_source_fd *) source;
+	uint32_t mask;
+
+//	wl_log("fd_dispatch\n");
+
+	mask = 0;
+	if (what & EV_READ)
+		mask |= WL_EVENT_READABLE;
+	if (what & EV_WRITE)
+		mask |= WL_EVENT_WRITABLE;
+//	wl_log("%s: what=%d, mask=%u\n", __func__, what, mask);
+#if 0
+	if (what & EPOLLHUP)
+		mask |= WL_EVENT_HANGUP;
+	if (what & EPOLLERR)
+		mask |= WL_EVENT_ERROR;
+#endif
+
+	return fd_source->func(fd_source->fd, mask, source->data);
+}
 
 struct wl_event_source_interface fd_source_interface = {
 	wl_event_source_fd_dispatch,
 };
 
+#if 0
 static struct wl_event_source *
 add_source(struct wl_event_loop *loop,
 	   struct wl_event_source *source, uint32_t mask, void *data)
@@ -123,6 +162,18 @@ add_source(struct wl_event_loop *loop,
 
 	return source;
 }
+#endif
+
+static struct wl_event_source *
+add_source(struct wl_event_loop *loop,
+	   struct wl_event_source *source, void *data)
+{
+	source->loop = loop;
+	source->data = data;
+	wl_list_init(&source->link);
+
+	return source;
+}
 
 WL_EXPORT struct wl_event_source *
 wl_event_loop_add_fd(struct wl_event_loop *loop,
@@ -131,19 +182,34 @@ wl_event_loop_add_fd(struct wl_event_loop *loop,
 		     void *data)
 {
 	struct wl_event_source_fd *source;
+	short what;
 
 	source = malloc(sizeof *source);
 	if (source == NULL)
 		return NULL;
 
+//	wl_log("%s called\n", __func__);
+
+	what = EV_PERSIST;
+	if (mask & WL_EVENT_READABLE)
+		what |= EV_READ;
+	if (mask & WL_EVENT_WRITABLE)
+		what |= EV_WRITE;
+
+	add_source(loop, &source->base, data);
 	source->base.interface = &fd_source_interface;
-	source->base.fd = wl_os_dupfd_cloexec(fd, 0);
+	source->base.fd = fcntl(fd, F_DUPFD_CLOEXEC, 0);
+	fcntl(source->base.fd, O_NONBLOCK);
 	source->func = func;
 	source->fd = fd;
+	source->base.ev = event_new(loop->evbase, source->base.fd, what,
+	    wl_event_source_libevent_dispatch, source);
+	event_add(source->base.ev, NULL);
 
-	return add_source(loop, &source->base, mask, data);
+	return &source->base;
 }
 
+#if 0
 WL_EXPORT int
 wl_event_source_fd_update(struct wl_event_source *source, uint32_t mask)
 {
@@ -159,12 +225,36 @@ wl_event_source_fd_update(struct wl_event_source *source, uint32_t mask)
 
 	return epoll_ctl(loop->epoll_fd, EPOLL_CTL_MOD, source->fd, &ep);
 }
+#endif
+
+WL_EXPORT int
+wl_event_source_fd_update(struct wl_event_source *source, uint32_t mask)
+{
+	struct wl_event_loop *loop = source->loop;
+	short what;
+
+//	wl_log("%s called\n", __func__);
+
+	what = EV_PERSIST;
+	if (mask & WL_EVENT_READABLE)
+		what |= EV_READ;
+	if (mask & WL_EVENT_WRITABLE)
+		what |= EV_WRITE;
+
+	event_free(source->ev);
+	source->ev = event_new(loop->evbase, source->fd, what,
+	    wl_event_source_libevent_dispatch, source);
+	event_add(source->ev, NULL);
+
+	return 0;
+}
 
 struct wl_event_source_timer {
 	struct wl_event_source base;
 	wl_event_loop_timer_func_t func;
 };
 
+#if 0
 static int
 wl_event_source_timer_dispatch(struct wl_event_source *source,
 			       struct epoll_event *ep)
@@ -181,6 +271,19 @@ wl_event_source_timer_dispatch(struct wl_event_source *source,
 
 	return timer_source->func(timer_source->base.data);
 }
+#endif
+
+static int
+wl_event_source_timer_dispatch(evutil_socket_t fd, short what, void *arg)
+{
+	struct wl_event_source *source = (struct wl_event_source *) arg;
+	struct wl_event_source_timer *timer_source =
+		(struct wl_event_source_timer *) source;
+
+//	wl_log("timer_dispatch\n");
+
+	return timer_source->func(timer_source->base.data);
+}
 
 struct wl_event_source_interface timer_source_interface = {
 	wl_event_source_timer_dispatch,
@@ -197,25 +300,31 @@ wl_event_loop_add_timer(struct wl_event_loop *loop,
 	if (source == NULL)
 		return NULL;
 
+//	wl_log("%s called\n", __func__);
+
+	add_source(loop, &source->base, data);
 	source->base.interface = &timer_source_interface;
-	source->base.fd = timerfd_create(CLOCK_MONOTONIC,
-					 TFD_CLOEXEC | TFD_NONBLOCK);
+	source->base.fd = -1;
 	source->func = func;
+	source->base.ev = event_new(loop->evbase, -1, 0,
+	    wl_event_source_libevent_dispatch, source);
 
-	return add_source(loop, &source->base, WL_EVENT_READABLE, data);
+	return &source->base;
 }
 
 WL_EXPORT int
 wl_event_source_timer_update(struct wl_event_source *source, int ms_delay)
 {
-	struct itimerspec its;
+	struct timeval tv;
 
-	its.it_interval.tv_sec = 0;
-	its.it_interval.tv_nsec = 0;
-	its.it_value.tv_sec = ms_delay / 1000;
-	its.it_value.tv_nsec = (ms_delay % 1000) * 1000 * 1000;
-	if (timerfd_settime(source->fd, 0, &its, NULL) < 0)
-		return -1;
+//	wl_log("updating timer to ms_delay=%d\n", ms_delay);
+
+	tv.tv_sec = ms_delay / 1000;
+	tv.tv_usec = (ms_delay % 1000) * 1000;
+	if (ms_delay == 0)
+		event_del(source->ev);
+	else
+		event_add(source->ev, &tv);
 
 	return 0;
 }
@@ -226,6 +335,7 @@ struct wl_event_source_signal {
 	wl_event_loop_signal_func_t func;
 };
 
+#if 0
 static int
 wl_event_source_signal_dispatch(struct wl_event_source *source,
 			       struct epoll_event *ep)
@@ -243,6 +353,20 @@ wl_event_source_signal_dispatch(struct wl_event_source *source,
 	return signal_source->func(signal_source->signal_number,
 				   signal_source->base.data);
 }
+#endif
+
+static int
+wl_event_source_signal_dispatch(evutil_socket_t fd, short what, void *arg)
+{
+	struct wl_event_source *source = (struct wl_event_source *) arg;
+	struct wl_event_source_signal *signal_source =
+		(struct wl_event_source_signal *) source;
+
+//	wl_log("signal_dispatch\n");
+
+	return signal_source->func(signal_source->signal_number,
+				   signal_source->base.data);
+}
 
 struct wl_event_source_interface signal_source_interface = {
 	wl_event_source_signal_dispatch,
@@ -255,23 +379,24 @@ wl_event_loop_add_signal(struct wl_event_loop *loop,
 			void *data)
 {
 	struct wl_event_source_signal *source;
-	sigset_t mask;
 
 	source = malloc(sizeof *source);
 	if (source == NULL)
 		return NULL;
 
+//	wl_log("%s called\n", __func__);
+
+	add_source(loop, &source->base, data);
 	source->base.interface = &signal_source_interface;
+	source->base.fd = -1;
+	source->func = func;
 	source->signal_number = signal_number;
 
-	sigemptyset(&mask);
-	sigaddset(&mask, signal_number);
-	source->base.fd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
-	sigprocmask(SIG_BLOCK, &mask, NULL);
+	source->base.ev = event_new(loop->evbase, signal_number,
+	    EV_SIGNAL|EV_PERSIST, wl_event_source_libevent_dispatch, source);
+	event_add(source->base.ev, NULL);
 
-	source->func = func;
-
-	return add_source(loop, &source->base, WL_EVENT_READABLE, data);
+	return &source->base;
 }
 
 struct wl_event_source_idle {
@@ -297,6 +422,7 @@ wl_event_loop_add_idle(struct wl_event_loop *loop,
 	source->base.interface = &idle_source_interface;
 	source->base.loop = loop;
 	source->base.fd = -1;
+	source->base.ev = NULL;
 
 	source->func = func;
 	source->base.data = data;
@@ -312,17 +438,26 @@ wl_event_source_check(struct wl_event_source *source)
 	wl_list_insert(source->loop->check_list.prev, &source->link);
 }
 
+WL_EXPORT void
+wl_event_source_activate(struct wl_event_source *source)
+{
+	if (source->ev != NULL) {
+		if (!event_pending(source->ev, EV_TIMEOUT|EV_READ|EV_WRITE|EV_SIGNAL, NULL)) {
+			event_add(source->ev, NULL);
+		}
+		event_active(source->ev, EV_TIMEOUT, 0);
+	}
+}
+
 WL_EXPORT int
 wl_event_source_remove(struct wl_event_source *source)
 {
 	struct wl_event_loop *loop = source->loop;
 
-	/* We need to explicitly remove the fd, since closing the fd
-	 * isn't enough in case we've dup'ed the fd. */
-	if (source->fd >= 0) {
-		epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, source->fd, NULL);
-		close(source->fd);
-		source->fd = -1;
+//	wl_log("%s called\n", __func__);
+	if (source->ev != NULL) {
+	event_free(source->ev);
+	source->ev = NULL;
 	}
 
 	wl_list_remove(&source->link);
@@ -336,9 +471,26 @@ wl_event_loop_process_destroy_list(struct wl_event_loop *loop)
 {
 	struct wl_event_source *source, *next;
 
-	wl_list_for_each_safe(source, next, &loop->destroy_list, link)
+	wl_list_for_each_safe(source, next, &loop->destroy_list, link){
+//		fprintf(stderr, "%s: processing entry\n", __func__);
+		if (source->ev != NULL) {
+			event_free(source->ev);
+			source->ev = NULL;
+		}
+//		fprintf(stderr, "%s: freed event\n", __func__);
+
+		/* We need to explicitly remove the fd, since closing the fd
+		 * isn't enough in case we've dup'ed the fd. */
+		if (source->fd >= 0) {
+//			fprintf(stderr, "%s: closing fd\n", __func__);
+			close(source->fd);
+			source->fd = -1;
+		}
+//		fprintf(stderr, "%s: freeing\n", __func__);
 		free(source);
+	}
 
+//	fprintf(stderr, "%s: wl_list_init\n", __func__);
 	wl_list_init(&loop->destroy_list);
 }
 
@@ -351,11 +503,17 @@ wl_event_loop_create(void)
 	if (loop == NULL)
 		return NULL;
 
-	loop->epoll_fd = wl_os_epoll_create_cloexec();
-	if (loop->epoll_fd < 0) {
+	evthread_use_pthreads();
+	loop->evbase = event_base_new();
+	if (loop->evbase == NULL) {
 		free(loop);
 		return NULL;
 	}
+//	loop->epoll_fd = wl_os_epoll_create_cloexec();
+//	if (loop->epoll_fd < 0) {
+//		free(loop);
+//		return NULL;
+//	}
 	wl_list_init(&loop->check_list);
 	wl_list_init(&loop->idle_list);
 	wl_list_init(&loop->destroy_list);
@@ -371,21 +529,22 @@ wl_event_loop_destroy(struct wl_event_loop *loop)
 	wl_signal_emit(&loop->destroy_signal, loop);
 
 	wl_event_loop_process_destroy_list(loop);
-	close(loop->epoll_fd);
+	event_base_free(loop->evbase);
+//	close(loop->epoll_fd);
 	free(loop);
 }
 
 static int
 post_dispatch_check(struct wl_event_loop *loop)
 {
-	struct epoll_event ep;
 	struct wl_event_source *source, *next;
 	int n;
 
-	ep.events = 0;
+//	wl_log("%s called\n", __func__);
+
 	n = 0;
 	wl_list_for_each_safe(source, next, &loop->check_list, link)
-		n += source->interface->dispatch(source, &ep);
+		n += source->interface->dispatch(source->fd, 0, source);
 
 	return n;
 }
@@ -395,49 +554,56 @@ wl_event_loop_dispatch_idle(struct wl_event_loop *loop)
 {
 	struct wl_event_source_idle *source;
 
+//	fprintf(stderr, "%s: running\n", __func__);
+
 	while (!wl_list_empty(&loop->idle_list)) {
 		source = container_of(loop->idle_list.next,
 				      struct wl_event_source_idle, base.link);
 		source->func(source->base.data);
 		wl_event_source_remove(&source->base);
 	}
+//	fprintf(stderr, "%s: finished\n", __func__);
 }
 
 WL_EXPORT int
 wl_event_loop_dispatch(struct wl_event_loop *loop, int timeout)
 {
-	struct epoll_event ep[32];
-	struct wl_event_source *source;
-	int i, count, n;
+	int val, n;
+
+//	fprintf(stderr, "%s: called\n", __func__);
 
 	wl_event_loop_dispatch_idle(loop);
 
-	count = epoll_wait(loop->epoll_fd, ep, ARRAY_LENGTH(ep), timeout);
-	if (count < 0)
-		return -1;
+//	wl_log("%s: called\n", __func__);
+//	fprintf(stderr, "%s: calling event_base_loop\n", __func__);
 
-	for (i = 0; i < count; i++) {
-		source = ep[i].data.ptr;
-		if (source->fd != -1)
-			source->interface->dispatch(source, &ep[i]);
+	val = event_base_loop(loop->evbase, EVLOOP_ONCE);
+	if (val < 0) {
+		fprintf(stderr, "%s: event_base_loop returned %d\n", __func__, val);
+		return -1;
 	}
 
+//	fprintf(stderr, "%s: calling wl_event_loop_process_destroy_list\n", __func__);
 	wl_event_loop_process_destroy_list(loop);
+//	fprintf(stderr, "%s: calling post_dispatch_check\n", __func__);
 
 	wl_event_loop_dispatch_idle(loop);
 
 	do {
 		n = post_dispatch_check(loop);
+//		fprintf(stderr, "%s: post_dispatch_check returned %d\n", __func__, n);
 	} while (n > 0);
 
 	return 0;
 }
 
+#if 0
 WL_EXPORT int
 wl_event_loop_get_fd(struct wl_event_loop *loop)
 {
 	return loop->epoll_fd;
 }
+#endif
 
 WL_EXPORT void
 wl_event_loop_add_destroy_listener(struct wl_event_loop *loop,
