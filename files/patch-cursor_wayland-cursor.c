--- cursor/wayland-cursor.c
+++ cursor/wayland-cursor.c
@@ -56,6 +56,9 @@ shm_pool_create(struct wl_shm *shm, int size)
 	if (!pool)
 		return NULL;
 
+	if (size < 0x8000)
+		size = 0x8000;
+
 	pool->fd = os_create_anonymous_file(size);
 	if (pool->fd < 0)
 		goto err_free;
@@ -91,6 +94,18 @@ shm_pool_resize(struct shm_pool *pool, int size)
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
@@ -162,6 +177,7 @@ wl_cursor_image_get_buffer(struct wl_cursor_image *_img)
 	struct cursor_image *image = (struct cursor_image *) _img;
 	struct wl_cursor_theme *theme = image->theme;
 
+//	fprintf(stderr, "%s running\n", __func__);
 	if (!image->buffer) {
 		image->buffer =
 			wl_shm_pool_create_buffer(theme->pool->pool,
