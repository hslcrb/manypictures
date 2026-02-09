#ifndef MANYPICTURES_EXIF_H
#define MANYPICTURES_EXIF_H

#include "../core/types.h"

/* EXIF data reader/writer with custom history tracking */

/* EXIF tags */
#define EXIF_TAG_IMAGE_WIDTH 0x0100
#define EXIF_TAG_IMAGE_HEIGHT 0x0101
#define EXIF_TAG_MAKE 0x010F
#define EXIF_TAG_MODEL 0x0110
#define EXIF_TAG_ORIENTATION 0x0112
#define EXIF_TAG_SOFTWARE 0x0131
#define EXIF_TAG_DATETIME 0x0132
#define EXIF_TAG_EXIF_OFFSET 0x8769

/* Custom Many Pictures tags (using maker note area) */
#define EXIF_TAG_MP_HISTORY 0x9000
#define EXIF_TAG_MP_VERSION 0x9001
#define EXIF_TAG_MP_OPERATION_COUNT 0x9002

/* Read EXIF data from JPEG */
mp_exif_data* mp_exif_read_jpeg(const char* filepath);

/* Write EXIF data to JPEG */
mp_result mp_exif_write_jpeg(const char* filepath, const mp_exif_data* exif);

/* Read EXIF from buffer */
mp_exif_data* mp_exif_read_buffer(const u8* data, size_t size);

/* Write EXIF to buffer */
mp_result mp_exif_write_buffer(const mp_exif_data* exif, u8** out_data, size_t* out_size);

/* Create EXIF data structure */
mp_exif_data* mp_exif_create(void);

/* Destroy EXIF data */
void mp_exif_destroy(mp_exif_data* exif);

/* Add history entry to EXIF */
mp_result mp_exif_add_history(mp_exif_data* exif, mp_operation_type op_type,
                              const char* description, const u8* params, u32 param_size);

/* Get history from EXIF */
mp_history_chain* mp_exif_get_history(const mp_exif_data* exif);

/* Restore image to specific history point */
mp_result mp_exif_restore_history(mp_image* image, u32 history_index);

#endif /* MANYPICTURES_EXIF_H */
