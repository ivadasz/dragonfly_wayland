--- src/wayland-os.c
+++ src/wayland-os.c
@@ -30,7 +30,7 @@
 #include <unistd.h>
 #include <fcntl.h>
 #include <errno.h>
-#include <sys/epoll.h>
+//#include <sys/epoll.h>
 
 #include "../config.h"
 #include "wayland-os.h"
@@ -62,11 +62,13 @@ wl_os_socket_cloexec(int domain, int type, int protocol)
 {
 	int fd;
 
+#ifdef SOCK_CLOEXEC
 	fd = socket(domain, type | SOCK_CLOEXEC, protocol);
 	if (fd >= 0)
 		return fd;
 	if (errno != EINVAL)
 		return -1;
+#endif
 
 	fd = socket(domain, type, protocol);
 	return set_cloexec_or_close(fd);
@@ -121,6 +123,7 @@ recvmsg_cloexec_fallback(int sockfd, struct msghdr *msg, int flags)
 ssize_t
 wl_os_recvmsg_cloexec(int sockfd, struct msghdr *msg, int flags)
 {
+#ifdef MSG_CMSG_CLOEXEC
 	ssize_t len;
 
 	len = recvmsg(sockfd, msg, flags | MSG_CMSG_CLOEXEC);
@@ -128,10 +131,12 @@ wl_os_recvmsg_cloexec(int sockfd, struct msghdr *msg, int flags)
 		return len;
 	if (errno != EINVAL)
 		return -1;
+#endif
 
 	return recvmsg_cloexec_fallback(sockfd, msg, flags);
 }
 
+#if 0
 int
 wl_os_epoll_create_cloexec(void)
 {
@@ -148,6 +153,7 @@ wl_os_epoll_create_cloexec(void)
 	fd = epoll_create(1);
 	return set_cloexec_or_close(fd);
 }
+#endif
 
 int
 wl_os_accept_cloexec(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
