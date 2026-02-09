#include "color_ops.h"
#include "../core/memory.h"
#include "../core/image.h"
#include "../core/fast_io.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* RGB to grayscale using luminosity method */
static u8 mp_rgb_to_gray(u8 r, u8 g, u8 b) {
    return (u8)((r * 299 + g * 587 + b * 114) / 1000);
}

mp_result mp_op_to_grayscale(mp_image* image) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    
    for (u32 y = 0; y < buffer->height; y++) {
        for (u32 x = 0; x < buffer->width; x++) {
            mp_pixel pixel = mp_image_get_pixel(buffer, x, y);
            u8 gray = mp_rgb_to_gray(pixel.r, pixel.g, pixel.b);
            pixel.r = pixel.g = pixel.b = gray;
            mp_image_set_pixel(buffer, x, y, pixel);
        }
    }
    
    image->modified = MP_TRUE;
    return MP_SUCCESS;
}

mp_result mp_op_invert(mp_image* image) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    
    for (u32 y = 0; y < buffer->height; y++) {
        for (u32 x = 0; x < buffer->width; x++) {
            mp_pixel pixel = mp_image_get_pixel(buffer, x, y);
            pixel.r = 255 - pixel.r;
            pixel.g = 255 - pixel.g;
            pixel.b = 255 - pixel.b;
            mp_image_set_pixel(buffer, x, y, pixel);
        }
    }
    
    image->modified = MP_TRUE;
    return MP_SUCCESS;
}



void mp_rgb_to_hsv(u8 r, u8 g, u8 b, f32* h, f32* s, f32* v) {
    f32 rf = r / 255.0f;
    f32 gf = g / 255.0f;
    f32 bf = b / 255.0f;
    
    f32 max = rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf);
    f32 min = rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf);
    f32 delta = max - min;
    
    *v = max;
    
    if (max == 0.0f) {
        *s = 0.0f;
        *h = 0.0f;
        return;
    }
    
    *s = delta / max;
    
    if (delta == 0.0f) {
        *h = 0.0f;
    } else if (max == rf) {
        *h = 60.0f * fmodf((gf - bf) / delta, 6.0f);
    } else if (max == gf) {
        *h = 60.0f * ((bf - rf) / delta + 2.0f);
    } else {
        *h = 60.0f * ((rf - gf) / delta + 4.0f);
    }
    
    if (*h < 0.0f) {
        *h += 360.0f;
    }
}

void mp_hsv_to_rgb(f32 h, f32 s, f32 v, u8* r, u8* g, u8* b) {
    if (s == 0.0f) {
        *r = *g = *b = (u8)(v * 255.0f);
        return;
    }
    
    h = fmodf(h, 360.0f);
    if (h < 0.0f) h += 360.0f;
    
    f32 c = v * s;
    f32 x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    f32 m = v - c;
    
    f32 rf, gf, bf;
    
    if (h < 60.0f) {
        rf = c; gf = x; bf = 0.0f;
    } else if (h < 120.0f) {
        rf = x; gf = c; bf = 0.0f;
    } else if (h < 180.0f) {
        rf = 0.0f; gf = c; bf = x;
    } else if (h < 240.0f) {
        rf = 0.0f; gf = x; bf = c;
    } else if (h < 300.0f) {
        rf = x; gf = 0.0f; bf = c;
    } else {
        rf = c; gf = 0.0f; bf = x;
    }
    
    *r = (u8)((rf + m) * 255.0f);
    *g = (u8)((gf + m) * 255.0f);
    *b = (u8)((bf + m) * 255.0f);
}

mp_result mp_op_brightness(mp_image* image, i32 value) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    
    for (u32 y = 0; y < buffer->height; y++) {
        for (u32 x = 0; x < buffer->width; x++) {
            mp_pixel pixel = mp_image_get_pixel(buffer, x, y);
            
            i32 r = pixel.r + value;
            i32 g = pixel.g + value;
            i32 b = pixel.b + value;
            
            pixel.r = r < 0 ? 0 : (r > 255 ? 255 : r);
            pixel.g = g < 0 ? 0 : (g > 255 ? 255 : g);
            pixel.b = b < 0 ? 0 : (b > 255 ? 255 : b);
            
            mp_image_set_pixel(buffer, x, y, pixel);
        }
    }
    
    image->modified = MP_TRUE;
    return MP_SUCCESS;
}

mp_result mp_op_contrast(mp_image* image, f32 value) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    f32 factor = (259.0f * (value * 255.0f + 255.0f)) / (255.0f * (259.0f - value * 255.0f));
    
    for (u32 y = 0; y < buffer->height; y++) {
        for (u32 x = 0; x < buffer->width; x++) {
            mp_pixel pixel = mp_image_get_pixel(buffer, x, y);
            
            i32 r = (i32)(factor * (pixel.r - 128) + 128);
            i32 g = (i32)(factor * (pixel.g - 128) + 128);
            i32 b = (i32)(factor * (pixel.b - 128) + 128);
            
            pixel.r = r < 0 ? 0 : (r > 255 ? 255 : r);
            pixel.g = g < 0 ? 0 : (g > 255 ? 255 : g);
            pixel.b = b < 0 ? 0 : (b > 255 ? 255 : b);
            
            mp_image_set_pixel(buffer, x, y, pixel);
        }
    }
    
    image->modified = MP_TRUE;
    return MP_SUCCESS;
}

mp_result mp_op_saturation(mp_image* image, f32 value) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    
    for (u32 y = 0; y < buffer->height; y++) {
        for (u32 x = 0; x < buffer->width; x++) {
            mp_pixel pixel = mp_image_get_pixel(buffer, x, y);
            
            f32 h, s, v;
            mp_rgb_to_hsv(pixel.r, pixel.g, pixel.b, &h, &s, &v);
            
            s *= value;
            if (s > 1.0f) s = 1.0f;
            if (s < 0.0f) s = 0.0f;
            
            mp_hsv_to_rgb(h, s, v, &pixel.r, &pixel.g, &pixel.b);
            mp_image_set_pixel(buffer, x, y, pixel);
        }
    }
    
    image->modified = MP_TRUE;
    return MP_SUCCESS;
}

mp_result mp_op_hue(mp_image* image, i32 degrees) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    
    for (u32 y = 0; y < buffer->height; y++) {
        for (u32 x = 0; x < buffer->width; x++) {
            mp_pixel pixel = mp_image_get_pixel(buffer, x, y);
            
            f32 h, s, v;
            mp_rgb_to_hsv(pixel.r, pixel.g, pixel.b, &h, &s, &v);
            
            h += degrees;
            while (h < 0.0f) h += 360.0f;
            while (h >= 360.0f) h -= 360.0f;
            
            mp_hsv_to_rgb(h, s, v, &pixel.r, &pixel.g, &pixel.b);
            mp_image_set_pixel(buffer, x, y, pixel);
        }
    }
    
    image->modified = MP_TRUE;
    return MP_SUCCESS;
}

void mp_colorization_predict(u8 gray, u8 context[8], u8* r, u8* g, u8* b) {
    /* "Monster" Grade Spectral Projection Colorization / "괴물급" 스펙트럼 투영 컬러화
     * We use a deterministic but highly non-linear mapping to create vibrant colors from grayscale.
     */
    f32 g_norm = gray / 255.0f;
    
    /* Calculate neighborhood variance for texture-aware tinting / 질감 인식 틴팅을 위한 주변부 분산 계산 */
    f32 avg_ctx = 0;
    for (int i = 0; i < 8; i++) avg_ctx += context[i];
    avg_ctx /= 8.0f;
    f32 var = 0;
    for (int i = 0; i < 8; i++) var += fabsf(context[i] - avg_ctx);
    var /= 255.0f;

    /* Base Spectral Mapping / 기본 스펙트럼 매핑 */
    f32 h = 200.0f + 60.0f * sinf(g_norm * M_PI * 1.5f + var * 2.0f); /* Hue shift based on luminence and detail */
    f32 s = 0.3f + 0.4f * (1.0f - g_norm) + var * 0.5f; /* Higher saturation in shadows and detailed areas */
    f32 v = g_norm * 0.9f + 0.1f;
    
    if (s > 0.8f) s = 0.8f;
    
    mp_hsv_to_rgb(h, s, v, r, g, b);
    
    /* Mix with original gray to maintain structure / 구조 유지를 위해 원본 그레이와 혼합 */
    *r = (u8)(*r * 0.8f + gray * 0.2f);
    *g = (u8)(*g * 0.8f + gray * 0.2f);
    *b = (u8)(*b * 0.8f + gray * 0.2f);
}

mp_result mp_op_to_color(mp_image* image) {
    if (!image || !image->buffer) return MP_ERROR_INVALID_PARAM;
    
    mp_fast_printf("Starting Spectral Colorization (Monster v2.2)... / 스펙트럼 컬러화 시작 (Monster v2.2)...\n");
    
    mp_image_buffer* buffer = image->buffer;
    u32 w = buffer->width, h = buffer->height;
    
    for (u32 y = 0; y < h; y++) {
        for (u32 x = 0; x < w; x++) {
            mp_pixel p = mp_image_get_pixel(buffer, x, y);
            u8 gray = mp_rgb_to_gray(p.r, p.g, p.b);
            u8 ctx[8];
            
            static const i8 dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
            static const i8 dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
            
            for (int i = 0; i < 8; i++) {
                i32 nx = (i32)x + dx[i], ny = (i32)y + dy[i];
                if (nx >= 0 && nx < (i32)w && ny >= 0 && ny < (i32)h) {
                    mp_pixel cp = mp_image_get_pixel(buffer, (u32)nx, (u32)ny);
                    ctx[i] = mp_rgb_to_gray(cp.r, cp.g, cp.b);
                } else ctx[i] = gray;
            }
            
            u8 r, g, b;
            mp_colorization_predict(gray, ctx, &r, &g, &b);
            p.r = r; p.g = g; p.b = b;
            mp_image_set_pixel(buffer, x, y, p);
        }
    }
    
    image->modified = MP_TRUE;
    return MP_SUCCESS;
}


mp_result mp_op_invert_grayscale(mp_image* image) {
    if (!image || !image->buffer) return MP_ERROR_INVALID_PARAM;
    mp_image_buffer* buf = image->buffer;
    
    /* Optimized single-pass invert and grayscale using integer weights */
    for (u32 y = 0; y < buf->height; y++) {
        for (u32 x = 0; x < buf->width; x++) {
            mp_pixel p = mp_image_get_pixel(buf, x, y);
            /* Perform inversion then grayscale in fixed-point */
            u8 ir = 255 - p.r, ig = 255 - p.g, ib = 255 - p.b;
            u8 gray = (u8)((ir * 299 + ig * 587 + ib * 114) / 1000);
            p.r = p.g = p.b = gray;
            mp_image_set_pixel(buf, x, y, p);
        }
    }
    image->modified = MP_TRUE;
    return MP_SUCCESS;
}

