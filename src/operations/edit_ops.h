#ifndef MANYPICTURES_EDIT_OPS_H
#define MANYPICTURES_EDIT_OPS_H

#include "../core/types.h"

/* Image editing operations */

/* Rotate image by degrees (90, 180, 270) */
mp_result mp_op_rotate(mp_image* image, i32 degrees);

/* Flip image horizontally */
mp_result mp_op_flip_horizontal(mp_image* image);

/* Flip image vertically */
mp_result mp_op_flip_vertical(mp_image* image);

/* Crop image */
mp_result mp_op_crop(mp_image* image, u32 x, u32 y, u32 width, u32 height);

/* Resize image */
mp_result mp_op_resize(mp_image* image, u32 new_width, u32 new_height);

/* Resize with specific algorithm */
typedef enum {
    MP_RESIZE_NEAREST,
    MP_RESIZE_BILINEAR,
    MP_RESIZE_BICUBIC,
    MP_RESIZE_LANCZOS
} mp_resize_algorithm;

mp_result mp_op_resize_ex(mp_image* image, u32 new_width, u32 new_height, 
                          mp_resize_algorithm algorithm);

#endif /* MANYPICTURES_EDIT_OPS_H */
