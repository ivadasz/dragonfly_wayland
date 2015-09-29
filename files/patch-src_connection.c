--- src/connection.c.orig	2015-06-13 00:31:24 +0200
+++ src/connection.c
@@ -293,7 +293,10 @@
 		msg.msg_namelen = 0;
 		msg.msg_iov = iov;
 		msg.msg_iovlen = count;
-		msg.msg_control = (clen > 0) ? cmsg : NULL;
+		if (clen == 0)
+			msg.msg_control = NULL;
+		else
+			msg.msg_control = cmsg;
 		msg.msg_controllen = clen;
 		msg.msg_flags = 0;
 
@@ -302,8 +305,10 @@
 				      MSG_NOSIGNAL | MSG_DONTWAIT);
 		} while (len == -1 && errno == EINTR);
 
-		if (len == -1)
+		if (len == -1) {
+			warn("%s: sendmsg", __func__);
 			return -1;
+		}
 
 		close_fds(&connection->fds_out, MAX_FDS_OUT);
 
@@ -361,8 +366,27 @@
 	if (connection->out.head - connection->out.tail +
 	    count > ARRAY_LENGTH(connection->out.data)) {
 		connection->want_flush = 1;
-		if (wl_connection_flush(connection) < 0)
+		/*
+		 * work around, wl_connection_flush can return a < 0 value
+		 * with errno == EAGAIN in regular usage.
+		 */
+		int count = 0;
+retry:
+		if (wl_connection_flush(connection) < 0) {
+			if (errno == EAGAIN) {
+				wl_log("%s: wl_connection_flush failed, "
+				    "retrying: %s\n", __func__,
+				    strerror(errno));
+				count++;
+				if (count > 10)
+					usleep(1);
+				goto retry;
+			} else {
+				wl_log("%s: wl_connection_flush failed: %s\n",
+				    __func__, strerror(errno));
+			}
 			return -1;
+		}
 	}
 
 	if (wl_buffer_put(&connection->out, data, count) < 0)
@@ -570,7 +594,7 @@
 			fd = args[i].h;
 			dup_fd = wl_os_dupfd_cloexec(fd, 0);
 			if (dup_fd < 0) {
-				wl_log("dup failed: %m");
+				wl_log("dup failed: %s\n", strerror(errno));
 				abort();
 			}
 			closure->args[i].h = dup_fd;
