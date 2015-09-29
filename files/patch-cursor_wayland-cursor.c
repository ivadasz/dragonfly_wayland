--- cursor/wayland-cursor.c.orig	2015-06-13 00:51:38 +0200
+++ cursor/wayland-cursor.c
@@ -56,6 +56,13 @@
 	if (!pool)
 		return NULL;
 
+	/*
+	 * This makes sure that enough space is available to avoid resizing,
+	 * of the pool used for cursors.
+	 */
+	if (size < 0x8000)
+		size = 0x8000;
+
 	pool->fd = os_create_anonymous_file(size);
 	if (pool->fd < 0)
 		goto err_free;
@@ -91,6 +98,18 @@
 		return 0;
 #endif
 
+#if 0
+#if defined(__DragonFly__)
+	void *data;
+        data = mmap((void *)pool->data + pool->used, size - pool->size,
+                    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, pool->fd,
+                    pool->used);
+	if (data == (void *)-1) {
+		fprintf(stderr, "failed with MAP_FIXED\n");
+		return 0;
+	}
+#endif
+#endif
 	wl_shm_pool_resize(pool->pool, size);
 
 	munmap(pool->data, pool->size);
