#include "../core/types.h"
#include "../core/memory.h"
#include "../core/image.h"

/* WebP format handler - stub implementation */

mp_image* mp_webp_load(const char* filepath) {
    /* TODO: Implement full WebP decoder with VP8/VP8L */
    (void)filepath;
    return NULL;
}

mp_result mp_webp_save(mp_image* image, const char* filepath) {
    /* TODO: Implement WebP encoder */
    (void)image;
    (void)filepath;
    return MP_ERROR_UNSUPPORTED;
}
