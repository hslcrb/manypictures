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
    
    u32 bpp = old_buffer->bpp;
    u32 old_w = old_buffer->width;
    u32 old_h = old_buffer->height;
    u8* restrict src = old_buffer->data;
    u8* restrict dst = new_buffer->data;

    /* Extreme optimization: Dedicated loops for specific rotations / 극한 최적화: 특정 회전을 위한 전용 루프 */
    if (degrees == 90) {
        for (u32 y = 0; y < old_h; y++) {
            u8* src_row = src + y * old_buffer->stride;
            for (u32 x = 0; x < old_w; x++) {
                u32 new_x = old_h - 1 - y;
                u32 new_y = x;
                u8* dst_px = dst + new_y * new_buffer->stride + new_x * bpp;
                memcpy(dst_px, src_row + x * bpp, bpp);
            }
        }
    } else if (degrees == 180) {
        for (u32 y = 0; y < old_h; y++) {
            u8* src_row = src + y * old_buffer->stride;
            u8* dst_row = dst + (old_h - 1 - y) * new_buffer->stride;
            for (u32 x = 0; x < old_w; x++) {
                u32 new_x = old_w - 1 - x;
                memcpy(dst_row + new_x * bpp, src_row + x * bpp, bpp);
            }
        }
    } else if (degrees == 270) {
        for (u32 y = 0; y < old_h; y++) {
            u8* src_row = src + y * old_buffer->stride;
            for (u32 x = 0; x < old_w; x++) {
                u32 new_x = y;
                u32 new_y = old_w - 1 - x;
                u8* dst_px = dst + new_y * new_buffer->stride + new_x * bpp;
                memcpy(dst_px, src_row + x * bpp, bpp);
            }
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
    u32 bpp = buffer->bpp;
    u32 width = buffer->width;
    u32 height = buffer->height;
    u32 stride = buffer->stride;
    u8* data = buffer->data;
    
    /* Extreme optimization: Targeted pointer reversal / 극한 최적화: 정밀 포인터 반전 */
    for (u32 y = 0; y < height; y++) {
        u8* left = data + y * stride;
        u8* right = left + (width - 1) * bpp;
        while (left < right) {
            /* Swap pixels based on bpp / bpp에 따른 픽셀 스왑 */
            if (bpp == 3) {
                u8 t0 = left[0], t1 = left[1], t2 = left[2];
                left[0] = right[0]; left[1] = right[1]; left[2] = right[2];
                right[0] = t0; right[1] = t1; right[2] = t2;
            } else if (bpp == 4) {
                u32* lp = (u32*)left;
                u32* rp = (u32*)right;
                u32 tmp = *lp; *lp = *rp; *rp = tmp;
            } else {
                for (u32 k = 0; k < bpp; k++) {
                    u8 tmp = left[k]; left[k] = right[k]; right[k] = tmp;
                }
            }
            left += bpp;
            right -= bpp;
        }
    }
    
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_FLIP_H, "Flipped Horizontally (Monster Optimized)");
    return MP_SUCCESS;
}

mp_result mp_op_flip_vertical(mp_image* image) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    u32 stride = buffer->stride;
    u8* temp_row = (u8*)mp_malloc(stride);
    if (!temp_row) return MP_ERROR_MEMORY;
    
    u8* data = buffer->data;
    u32 h = buffer->height;
    
    /* Extreme optimization: Row-level swapping with memcpy / 극한 최적화: memcpy를 사용한 행 단위 스왑 */
    for (u32 y = 0; y < h / 2; y++) {
        u8* top = data + y * stride;
        u8* bottom = data + (h - 1 - y) * stride;
        memcpy(temp_row, top, stride);
        memcpy(top, bottom, stride);
        memcpy(bottom, temp_row, stride);
    }
    
    mp_free(temp_row);
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_FLIP_V, "Flipped Vertically (Monster Row-Swap)");
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
    
    u32 bpp = old_buffer->bpp;
    u32 old_stride = old_buffer->stride;
    u32 new_stride = new_buffer->stride;
    u8* src = old_buffer->data + y * old_stride + x * bpp;
    u8* dst = new_buffer->data;
    
    /* Extreme optimization: Row-level copy via memcpy / 극한 최적화: memcpy를 이용한 행 단위 복사 */
    for (u32 ny = 0; ny < height; ny++) {
        memcpy(dst, src, width * bpp);
        src += old_stride;
        dst += new_stride;
    }
    
    mp_image_buffer_destroy(old_buffer);
    image->buffer = new_buffer;
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_CROP, "Cropped Image (Monster Row-Copy)");
    
    return MP_SUCCESS;
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
    u32 bpp = old_buffer->bpp;
    u8* restrict src_data = old_buffer->data;
    u8* restrict dst_data = new_buffer->data;
    
    /* Extreme optimization: Direct pointer access in resize loop / 극한 최적화: 크기 조정 루프 내 직접 포인터 액세스 */
    for (u32 y = 0; y < new_height; y++) {
        u8* restrict dst_row = dst_data + y * new_buffer->stride;
        f32 src_y = y * y_ratio;
        u32 sy = (u32)src_y;
        if (sy >= old_buffer->height) sy = old_buffer->height - 1;
        u8* restrict src_row = src_data + sy * old_buffer->stride;

        for (u32 x = 0; x < new_width; x++) {
            f32 src_x = x * x_ratio;
            u32 sx = (u32)src_x;
            if (sx >= old_buffer->width) sx = old_buffer->width - 1;

            if (algorithm == MP_RESIZE_NEAREST) {
                u8* sp = src_row + sx * bpp;
                if (bpp == 3) {
                    dst_row[0] = sp[0]; dst_row[1] = sp[1]; dst_row[2] = sp[2];
                } else if (bpp == 4) {
                    *((u32*)dst_row) = *((u32*)sp);
                } else {
                    memcpy(dst_row, sp, bpp);
                }
            } else {
                /* Bi-linear sampling fallback for complexity / 복잡성을 고려한 바이리니어 샘플링 폴백 */
                mp_pixel pixel = mp_sample_bilinear(old_buffer, src_x, src_y);
                if (bpp == 3) {
                    dst_row[0] = pixel.r; dst_row[1] = pixel.g; dst_row[2] = pixel.b;
                } else {
                    dst_row[0] = pixel.r; dst_row[1] = pixel.g; dst_row[2] = pixel.b; dst_row[3] = pixel.a;
                }
            }
            dst_row += bpp;
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
