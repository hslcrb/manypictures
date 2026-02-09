#include "edit_ops.h"
#include "../core/memory.h"
#include "../core/image.h"
#include <string.h>
#include <math.h>

mp_result mp_op_rotate(mp_image* image, i32 degrees) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    degrees = degrees % 360;
    if (degrees < 0) degrees += 360;
    
    if (degrees == 0) {
        return MP_SUCCESS;
    }
    
    mp_image_buffer* old_buffer = image->buffer;
    mp_image_buffer* new_buffer;
    
    if (degrees == 90 || degrees == 270) {
        /* Swap width and height */
        new_buffer = mp_image_buffer_create(old_buffer->height, old_buffer->width, old_buffer->format);
    } else if (degrees == 180) {
        new_buffer = mp_image_buffer_create(old_buffer->width, old_buffer->height, old_buffer->format);
    } else {
        return MP_ERROR_UNSUPPORTED;
    }
    
    if (!new_buffer) {
        return MP_ERROR_MEMORY;
    }
    
    for (u32 y = 0; y < old_buffer->height; y++) {
        for (u32 x = 0; x < old_buffer->width; x++) {
            mp_pixel pixel = mp_image_get_pixel(old_buffer, x, y);
            u32 new_x, new_y;
            
            if (degrees == 90) {
                new_x = old_buffer->height - 1 - y;
                new_y = x;
            } else if (degrees == 180) {
                new_x = old_buffer->width - 1 - x;
                new_y = old_buffer->height - 1 - y;
            } else { /* 270 */
                new_x = y;
                new_y = old_buffer->width - 1 - x;
            }
            
            mp_image_set_pixel(new_buffer, new_x, new_y, pixel);
        }
    }
    
    mp_image_buffer_destroy(old_buffer);
    image->buffer = new_buffer;
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_ROTATE, "Rotated Image");
    
    return MP_SUCCESS;
}

mp_result mp_op_flip_horizontal(mp_image* image) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    
    for (u32 y = 0; y < buffer->height; y++) {
        for (u32 x = 0; x < buffer->width / 2; x++) {
            u32 mirror_x = buffer->width - 1 - x;
            mp_pixel left = mp_image_get_pixel(buffer, x, y);
            mp_pixel right = mp_image_get_pixel(buffer, mirror_x, y);
            mp_image_set_pixel(buffer, x, y, right);
            mp_image_set_pixel(buffer, mirror_x, y, left);
        }
    }
    
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_FLIP_H, "Flipped Horizontally");
    return MP_SUCCESS;
}

mp_result mp_op_flip_vertical(mp_image* image) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    
    for (u32 y = 0; y < buffer->height / 2; y++) {
        u32 mirror_y = buffer->height - 1 - y;
        for (u32 x = 0; x < buffer->width; x++) {
            mp_pixel top = mp_image_get_pixel(buffer, x, y);
            mp_pixel bottom = mp_image_get_pixel(buffer, x, mirror_y);
            mp_image_set_pixel(buffer, x, y, bottom);
            mp_image_set_pixel(buffer, x, mirror_y, top);
        }
    }
    
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_FLIP_V, "Flipped Vertically");
    return MP_SUCCESS;
}

mp_result mp_op_crop(mp_image* image, u32 x, u32 y, u32 width, u32 height) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* old_buffer = image->buffer;
    
    if (x + width > old_buffer->width || y + height > old_buffer->height) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* new_buffer = mp_image_buffer_create(width, height, old_buffer->format);
    if (!new_buffer) {
        return MP_ERROR_MEMORY;
    }
    
    for (u32 ny = 0; ny < height; ny++) {
        for (u32 nx = 0; nx < width; nx++) {
            mp_pixel pixel = mp_image_get_pixel(old_buffer, x + nx, y + ny);
            mp_image_set_pixel(new_buffer, nx, ny, pixel);
        }
    }
    
    mp_image_buffer_destroy(old_buffer);
    image->buffer = new_buffer;
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_CROP, "Cropped Image");
    
    return MP_SUCCESS;
}

static mp_pixel mp_sample_nearest(const mp_image_buffer* buffer, f32 x, f32 y) {
    u32 ix = (u32)(x + 0.5f);
    u32 iy = (u32)(y + 0.5f);
    
    if (ix >= buffer->width) ix = buffer->width - 1;
    if (iy >= buffer->height) iy = buffer->height - 1;
    
    return mp_image_get_pixel(buffer, ix, iy);
}

static mp_pixel mp_sample_bilinear(const mp_image_buffer* buffer, f32 x, f32 y) {
    u32 x0 = (u32)x;
    u32 y0 = (u32)y;
    u32 x1 = x0 + 1;
    u32 y1 = y0 + 1;
    
    if (x1 >= buffer->width) x1 = buffer->width - 1;
    if (y1 >= buffer->height) y1 = buffer->height - 1;
    
    f32 fx = x - x0;
    f32 fy = y - y0;
    
    mp_pixel p00 = mp_image_get_pixel(buffer, x0, y0);
    mp_pixel p10 = mp_image_get_pixel(buffer, x1, y0);
    mp_pixel p01 = mp_image_get_pixel(buffer, x0, y1);
    mp_pixel p11 = mp_image_get_pixel(buffer, x1, y1);
    
    mp_pixel result;
    result.r = (u8)((1 - fx) * (1 - fy) * p00.r + fx * (1 - fy) * p10.r +
                    (1 - fx) * fy * p01.r + fx * fy * p11.r);
    result.g = (u8)((1 - fx) * (1 - fy) * p00.g + fx * (1 - fy) * p10.g +
                    (1 - fx) * fy * p01.g + fx * fy * p11.g);
    result.b = (u8)((1 - fx) * (1 - fy) * p00.b + fx * (1 - fy) * p10.b +
                    (1 - fx) * fy * p01.b + fx * fy * p11.b);
    result.a = (u8)((1 - fx) * (1 - fy) * p00.a + fx * (1 - fy) * p10.a +
                    (1 - fx) * fy * p01.a + fx * fy * p11.a);
    
    return result;
}

mp_result mp_op_resize_ex(mp_image* image, u32 new_width, u32 new_height,
                          mp_resize_algorithm algorithm) {
    if (!image || !image->buffer || new_width == 0 || new_height == 0) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* old_buffer = image->buffer;
    mp_image_buffer* new_buffer = mp_image_buffer_create(new_width, new_height, old_buffer->format);
    
    if (!new_buffer) {
        return MP_ERROR_MEMORY;
    }
    
    f32 x_ratio = (f32)old_buffer->width / new_width;
    f32 y_ratio = (f32)old_buffer->height / new_height;
    
    for (u32 y = 0; y < new_height; y++) {
        for (u32 x = 0; x < new_width; x++) {
            f32 src_x = x * x_ratio;
            f32 src_y = y * y_ratio;
            
            mp_pixel pixel;
            
            switch (algorithm) {
                case MP_RESIZE_NEAREST:
                    pixel = mp_sample_nearest(old_buffer, src_x, src_y);
                    break;
                case MP_RESIZE_BILINEAR:
                    pixel = mp_sample_bilinear(old_buffer, src_x, src_y);
                    break;
                default:
                    pixel = mp_sample_bilinear(old_buffer, src_x, src_y);
                    break;
            }
            
            mp_image_set_pixel(new_buffer, x, y, pixel);
        }
    }
    
    mp_image_buffer_destroy(old_buffer);
    image->buffer = new_buffer;
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_RESIZE, "Resized Image");
    
    return MP_SUCCESS;
}

mp_result mp_op_resize(mp_image* image, u32 new_width, u32 new_height) {
    return mp_op_resize_ex(image, new_width, new_height, MP_RESIZE_BILINEAR);
}
