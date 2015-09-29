--- src/wayland-client.c.orig	2015-08-25 05:38:19 +0200
+++ src/wayland-client.c
@@ -598,7 +598,7 @@
 
 	closure = wl_closure_marshal(&proxy->object, opcode, args, message);
 	if (closure == NULL) {
-		wl_log("Error marshalling request: %m\n");
+		wl_log("Error marshalling request: %s\n", strerror(errno));
 		abort();
 	}
 
@@ -606,7 +606,7 @@
 		wl_closure_print(closure, &proxy->object, true);
 
 	if (wl_closure_send(closure, proxy->display->connection)) {
-		wl_log("Error sending request: %m\n");
+		wl_log("Error sending request: %s\n", strerror(errno));
 		abort();
 	}
 
