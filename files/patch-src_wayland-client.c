--- src/wayland-client.c
+++ src/wayland-client.c
@@ -598,7 +598,7 @@ wl_proxy_marshal_array_constructor(struct wl_proxy *proxy,
 
 	closure = wl_closure_marshal(&proxy->object, opcode, args, message);
 	if (closure == NULL) {
-		wl_log("Error marshalling request: %m\n");
+		wl_log("Error marshalling request: %s\n", strerror(errno));
 		abort();
 	}
 
@@ -606,7 +606,7 @@ wl_proxy_marshal_array_constructor(struct wl_proxy *proxy,
 		wl_closure_print(closure, &proxy->object, true);
 
 	if (wl_closure_send(closure, proxy->display->connection)) {
-		wl_log("Error sending request: %m\n");
+		wl_log("Error sending request: %s\n", strerror(errno));
 		abort();
 	}
 
@@ -1547,6 +1547,7 @@ wl_display_dispatch_queue(struct wl_display *display,
 	pthread_mutex_lock(&display->mutex);
 
 	ret = dispatch_queue(display, queue);
+//	fprintf(stderr, "dispatch_queue returns %d\n", ret);
 	if (ret == -1)
 		goto err_unlock;
 	if (ret > 0) {
@@ -1559,6 +1560,7 @@ wl_display_dispatch_queue(struct wl_display *display,
 	 * will close the socket, and if we bail out here we don't get
 	 * a chance to process the error. */
 	ret = wl_connection_flush(display->connection);
+//	fprintf(stderr, "%s: wl_connection_flush returns %d\n", __func__, ret);
 	if (ret < 0 && errno != EAGAIN && errno != EPIPE) {
 		display_fatal_error(display, errno);
 		goto err_unlock;
@@ -1573,6 +1575,7 @@ wl_display_dispatch_queue(struct wl_display *display,
 	do {
 		ret = poll(pfd, 1, -1);
 	} while (ret == -1 && errno == EINTR);
+//	fprintf(stderr, "poll returns %d\n", ret);
 
 	if (ret == -1) {
 		wl_display_cancel_read(display);
@@ -1585,6 +1588,7 @@ wl_display_dispatch_queue(struct wl_display *display,
 		goto err_unlock;
 
 	ret = dispatch_queue(display, queue);
+//	fprintf(stderr, "dispatch_queue returns %d\n", ret);
 	if (ret == -1)
 		goto err_unlock;
 
