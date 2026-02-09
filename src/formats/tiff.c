#include "../core/types.h"
#include "../core/memory.h"
#include "../core/image.h"

/* TIFF format handler - stub implementation */

mp_image* mp_tiff_load(const char* filepath) {
    /* TODO: Implement full TIFF decoder with IFD parsing */
    (void)filepath;
    return NULL;
}

mp_result mp_tiff_save(mp_image* image, const char* filepath) {
    /* TODO: Implement TIFF encoder */
    (void)image;
    (void)filepath;
    return MP_ERROR_UNSUPPORTED;
}
