--- src/wayland-server.c
+++ src/wayland-server.c
@@ -44,6 +44,13 @@
 #include <sys/stat.h>
 #include <ffi.h>
 
+#include "../config.h"
+
+#ifdef HAVE_SYS_UCRED_H
+#include <sys/types.h>
+#include <sys/ucred.h>
+#endif
+
 #include "wayland-private.h"
 #include "wayland-server.h"
 #include "wayland-server-protocol.h"
@@ -80,7 +87,11 @@ struct wl_client {
 	struct wl_list link;
 	struct wl_map objects;
 	struct wl_signal destroy_signal;
+#ifdef HAVE_SYS_UCRED_H
+	struct xucred xucred;
+#else
 	struct ucred ucred;
+#endif
 	int error;
 };
 
@@ -251,7 +262,9 @@ wl_client_connection_data(int fd, uint32_t mask, void *data)
 	}
 
 	if (mask & WL_EVENT_WRITABLE) {
+//		wl_log("%s: WL_EVENT_WRITABLE\n", __func__);
 		len = wl_connection_flush(connection);
+//		wl_log("%s: len=%d\n", __func__, len);
 		if (len < 0 && errno != EAGAIN) {
 			wl_client_destroy(client);
 			return 1;
@@ -263,8 +276,13 @@ wl_client_connection_data(int fd, uint32_t mask, void *data)
 
 	len = 0;
 	if (mask & WL_EVENT_READABLE) {
+//		wl_log("%s: WL_EVENT_READABLE\n", __func__);
 		len = wl_connection_read(connection);
+#if 1
 		if (len == 0 || (len < 0 && errno != EAGAIN)) {
+#else
+		if (len < 0 && errno != EAGAIN) {
+#endif
 			wl_client_destroy(client);
 			return 1;
 		}
@@ -349,8 +367,12 @@ wl_client_connection_data(int fd, uint32_t mask, void *data)
 			break;
 	}
 
-	if (client->error)
+	if (client->error) {
+		fprintf(stderr, "%s: calling wl_client_destroy\n", __func__);
 		wl_client_destroy(client);
+	}
+
+//	fprintf(stderr, "%s: returning 1\n", __func__);
 
 	return 1;
 }
@@ -414,7 +436,10 @@ WL_EXPORT struct wl_client *
 wl_client_create(struct wl_display *display, int fd)
 {
 	struct wl_client *client;
+#ifdef HAVE_SYS_UCRED_H
+#else
 	socklen_t len;
+#endif
 
 	client = malloc(sizeof *client);
 	if (client == NULL)
@@ -429,10 +454,13 @@ wl_client_create(struct wl_display *display, int fd)
 	if (!client->source)
 		goto err_client;
 
+#ifdef HAVE_SYS_UCRED_H
+#else
 	len = sizeof client->ucred;
 	if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED,
 		       &client->ucred, &len) < 0)
 		goto err_source;
+#endif
 
 	client->connection = wl_connection_create(fd);
 	if (client->connection == NULL)
@@ -484,12 +512,21 @@ WL_EXPORT void
 wl_client_get_credentials(struct wl_client *client,
 			  pid_t *pid, uid_t *uid, gid_t *gid)
 {
+#ifdef HAVE_SYS_UCRED_H
+	if (pid)
+		*pid = 0;
+	if (uid)
+		*uid = client->xucred.cr_uid;
+	if (gid)
+		*gid = client->xucred.cr_gid;
+#else
 	if (pid)
 		*pid = client->ucred.pid;
 	if (uid)
 		*uid = client->ucred.uid;
 	if (gid)
 		*gid = client->ucred.gid;
+#endif
 }
 
 /** Look up an object in the client name space
@@ -669,6 +706,8 @@ wl_client_destroy(struct wl_client *client)
 {
 	uint32_t serial = 0;
 
+	wl_log("%s called\n", __func__);
+
 	wl_signal_emit(&client->destroy_signal, client);
 
 	wl_client_flush(client);
@@ -911,7 +950,7 @@ wl_global_create(struct wl_display *display,
 
 	if (interface->version < version) {
 		wl_log("wl_global_create: implemented version higher "
-		       "than interface version%m\n");
+		       "than interface version%s\n", strerror(errno));
 		return NULL;
 	}
 
@@ -1035,11 +1074,13 @@ socket_data(int fd, uint32_t mask, void *data)
 	client_fd = wl_os_accept_cloexec(fd, (struct sockaddr *) &name,
 					 &length);
 	if (client_fd < 0)
-		wl_log("failed to accept: %m\n");
+		wl_log("failed to accept: %s\n", strerror(errno));
 	else
 		if (!wl_client_create(display, client_fd))
 			close(client_fd);
 
+	wl_log("%s: accepted connection\n", __func__);
+
 	return 1;
 }
 
@@ -1140,12 +1181,12 @@ _wl_display_add_socket(struct wl_display *display, struct wl_socket *s)
 
 	size = offsetof (struct sockaddr_un, sun_path) + strlen(s->addr.sun_path);
 	if (bind(s->fd, (struct sockaddr *) &s->addr, size) < 0) {
-		wl_log("bind() failed with error: %m\n");
+		wl_log("bind() failed with error: %s\n", strerror(errno));
 		return -1;
 	}
 
 	if (listen(s->fd, 128) < 0) {
-		wl_log("listen() failed with error: %m\n");
+		wl_log("listen() failed with error: %s\n", strerror(errno));
 		return -1;
 	}
 
