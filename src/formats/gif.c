#include "../core/types.h"
#include "../core/memory.h"
#include "../core/image.h"

/* GIF format handler - stub implementation */

mp_image* mp_gif_load(const char* filepath) {
    /* TODO: Implement full GIF decoder with LZW decompression */
    (void)filepath;
    return NULL;
}

mp_result mp_gif_save(mp_image* image, const char* filepath) {
    /* TODO: Implement GIF encoder */
    (void)image;
    (void)filepath;
    return MP_ERROR_UNSUPPORTED;
}
