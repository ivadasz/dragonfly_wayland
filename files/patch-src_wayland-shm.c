--- src/wayland-shm.c
+++ src/wayland-shm.c
@@ -140,6 +140,7 @@ shm_pool_create_buffer(struct wl_client *client, struct wl_resource *resource,
 		return;
 	}
 
+	fprintf(stderr, "%s: offset=%d, width=%d, height=%d, stride=%d, pool->size=%d\n", __func__, offset, width, height, stride, pool->size);
 	if (offset < 0 || width <= 0 || height <= 0 || stride < width ||
 	    INT32_MAX / stride <= height ||
 	    offset > pool->size - stride * height) {
@@ -205,8 +206,16 @@ shm_pool_resize(struct wl_client *client, struct wl_resource *resource,
 				       "shrinking pool invalid");
 		return;
 	}
+	if (size == pool->size)
+		return;
 
+#if defined(__DragonFly__)
+	fprintf(stderr, "%s: mremap from %d to %d attempted\n", __func__,
+	    pool->size, size);
+	data = MAP_FAILED;
+#else
 	data = mremap(pool->data, pool->size, size, MREMAP_MAYMOVE);
+#endif
 	if (data == MAP_FAILED) {
 		wl_resource_post_error(resource,
 				       WL_SHM_ERROR_INVALID_FD,
@@ -243,6 +252,9 @@ shm_create_pool(struct wl_client *client, struct wl_resource *resource,
 		goto err_free;
 	}
 
+	if (size < 0x8000)
+		size = 0x8000;
+
 	pool->refcount = 1;
 	pool->size = size;
 	pool->data = mmap(NULL, size,
@@ -449,6 +461,15 @@ sigbus_handler(int signum, siginfo_t *info, void *context)
 	sigbus_data->fallback_mapping_used = 1;
 
 	/* This should replace the previous mapping */
+#if defined(__DragonFly__)
+	if (mmap(pool->data, pool->size,
+		 PROT_READ | PROT_WRITE,
+		 MAP_PRIVATE | MAP_FIXED | MAP_ANON,
+		 0, 0) == (void *) -1) {
+		reraise_sigbus();
+		return;
+	}
+#else
 	if (mmap(pool->data, pool->size,
 		 PROT_READ | PROT_WRITE,
 		 MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
@@ -456,6 +477,7 @@ sigbus_handler(int signum, siginfo_t *info, void *context)
 		reraise_sigbus();
 		return;
 	}
+#endif
 }
 
 static void
