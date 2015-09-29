--- src/wayland-shm.c.orig	2015-06-13 00:31:24 +0200
+++ src/wayland-shm.c
@@ -140,6 +140,8 @@
 		return;
 	}
 
+//	wl_log("%s: offset=%d, width=%d, height=%d, stride=%d, pool->size=%d\n",
+//	    __func__, offset, width, height, stride, pool->size);
 	if (offset < 0 || width <= 0 || height <= 0 || stride < width ||
 	    INT32_MAX / stride <= height ||
 	    offset > pool->size - stride * height) {
@@ -205,8 +207,16 @@
 				       "shrinking pool invalid");
 		return;
 	}
+	if (size == pool->size)
+		return;
 
+#if defined(__DragonFly__)
+	wl_log("%s: mremap from %d to %d attempted\n", __func__,
+	    pool->size, size);
+	data = MAP_FAILED;
+#else
 	data = mremap(pool->data, pool->size, size, MREMAP_MAYMOVE);
+#endif
 	if (data == MAP_FAILED) {
 		wl_resource_post_error(resource,
 				       WL_SHM_ERROR_INVALID_FD,
@@ -243,6 +253,10 @@
 		goto err_free;
 	}
 
+	/* Makes sure that cursor handling works without needing mremap */
+	if (size < 0x8000)
+		size = 0x8000;
+
 	pool->refcount = 1;
 	pool->size = size;
 	pool->data = mmap(NULL, size,
@@ -449,6 +463,15 @@
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
@@ -456,6 +479,7 @@
 		reraise_sigbus();
 		return;
 	}
+#endif
 }
 
 static void
