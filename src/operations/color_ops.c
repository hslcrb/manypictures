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


mp_result mp_op_to_grayscale(mp_image* image) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    u8* restrict data = buffer->data;
    u32 width = buffer->width;
    u32 height = buffer->height;
    u32 bpp = buffer->bpp;
    u32 stride = buffer->stride;
    
    /* Extreme optimization: Fixed-point + 8x Unrolling / 극한 최적화: 고정 소수점 + 8배 언롤링 */
    for (u32 y = 0; y < height; y++) {
        u8* restrict p = data + y * stride;
        u32 x = 0;
        for (; x <= width - 8; x += 8) {
            #define GS_STEP { \
                u32 g = (p[0] * 77 + p[1] * 150 + p[2] * 29) >> 8; \
                p[0] = p[1] = p[2] = (u8)g; p += bpp; \
            }
            GS_STEP GS_STEP GS_STEP GS_STEP
            GS_STEP GS_STEP GS_STEP GS_STEP
            #undef GS_STEP
        }
        for (; x < width; x++) {
            u32 g = (p[0] * 77 + p[1] * 150 + p[2] * 29) >> 8;
            p[0] = p[1] = p[2] = (u8)g; p += bpp;
        }
    }
    
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_GRAYSCALE, "Converted to Grayscale (Monster Optimized)");
    return MP_SUCCESS;
}

mp_result mp_op_invert(mp_image* image) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_image_buffer* buffer = image->buffer;
    u8* restrict p = buffer->data;
    size_t count = (size_t)buffer->width * buffer->height;
    u32 bpp = buffer->bpp;
    
    /* Extreme optimization: Pointer arithmetic + 8x Unrolling / 극한 최적화: 포인터 연산 + 8배 언롤링 */
    size_t i = 0;
    for (; i <= count - 8; i += 8) {
        #define INV_STEP { p[0] = ~p[0]; p[1] = ~p[1]; p[2] = ~p[2]; p += bpp; }
        INV_STEP INV_STEP INV_STEP INV_STEP
        INV_STEP INV_STEP INV_STEP INV_STEP
        #undef INV_STEP
    }
    for (; i < count; i++) {
        p[0] = ~p[0]; p[1] = ~p[1]; p[2] = ~p[2]; p += bpp;
    }
    
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_INVERT, "Inverted Colors (Monster Optimized)");
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
    
    /* Precompute LUT / LUT 사전 계산 */
    u8 lut[256];
    for (int i = 0; i < 256; i++) {
        i32 v = i + value;
        lut[i] = (u8)(v < 0 ? 0 : (v > 255 ? 255 : v));
    }
    
    mp_image_buffer* buffer = image->buffer;
    u8* restrict p = buffer->data;
    size_t count = (size_t)buffer->width * buffer->height;
    u32 bpp = buffer->bpp;
    
    /* Extreme optimization: LUT + 16x Unrolling / 극한 최적화: LUT + 16배 언롤링 */
    size_t i = 0;
    for (; i <= count - 16; i += 16) {
        #define BR_LUT_STEP { p[0] = lut[p[0]]; p[1] = lut[p[1]]; p[2] = lut[p[2]]; p += bpp; }
        BR_LUT_STEP BR_LUT_STEP BR_LUT_STEP BR_LUT_STEP
        BR_LUT_STEP BR_LUT_STEP BR_LUT_STEP BR_LUT_STEP
        BR_LUT_STEP BR_LUT_STEP BR_LUT_STEP BR_LUT_STEP
        BR_LUT_STEP BR_LUT_STEP BR_LUT_STEP BR_LUT_STEP
        #undef BR_LUT_STEP
    }
    for (; i < count; i++) {
        p[0] = lut[p[0]]; p[1] = lut[p[1]]; p[2] = lut[p[2]]; p += bpp;
    }
    
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_BRIGHTNESS, "Adjusted Brightness (Monster LUT Optimized)");
    return MP_SUCCESS;
}

mp_result mp_op_contrast(mp_image* image, f32 value) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    f32 factor = (259.0f * (value * 255.0f + 255.0f)) / (255.0f * (259.0f - value * 255.0f));
    
    /* Precompute LUT / LUT 사전 계산 */
    u8 lut[256];
    for (int i = 0; i < 256; i++) {
        i32 v = (i32)(factor * (i - 128) + 128);
        lut[i] = (u8)(v < 0 ? 0 : (v > 255 ? 255 : v));
    }
    
    mp_image_buffer* buffer = image->buffer;
    u8* restrict p = buffer->data;
    size_t count = (size_t)buffer->width * buffer->height;
    u32 bpp = buffer->bpp;
    
    /* Extreme optimization: LUT + 16x Unrolling / 극한 최적화: LUT + 16배 언롤링 */
    size_t i = 0;
    for (; i <= count - 16; i += 16) {
        #define CT_LUT_STEP { p[0] = lut[p[0]]; p[1] = lut[p[1]]; p[2] = lut[p[2]]; p += bpp; }
        CT_LUT_STEP CT_LUT_STEP CT_LUT_STEP CT_LUT_STEP
        CT_LUT_STEP CT_LUT_STEP CT_LUT_STEP CT_LUT_STEP
        CT_LUT_STEP CT_LUT_STEP CT_LUT_STEP CT_LUT_STEP
        CT_LUT_STEP CT_LUT_STEP CT_LUT_STEP CT_LUT_STEP
        #undef CT_LUT_STEP
    }
    for (; i < count; i++) {
        p[0] = lut[p[0]]; p[1] = lut[p[1]]; p[2] = lut[p[2]]; p += bpp;
    }
    
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_CONTRAST, "Adjusted Contrast (Monster LUT Optimized)");
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
    mp_image_record_history(image, MP_OP_SATURATION, "Adjusted Saturation");
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
    mp_image_record_history(image, MP_OP_HUE, "Adjusted Hue");
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
    
    mp_image_buffer* buffer = image->buffer;
    u32 w = buffer->width, h = buffer->height;
    u32 bpp = buffer->bpp;
    u32 stride = buffer->stride;
    u8* restrict data = buffer->data;
    
    mp_fast_printf("Starting Extreme Spectral Colorization (Monster v2.5)... / 극한 스펙트럼 컬러화 시작 (Monster v2.5)...\n");
    
    for (u32 y = 0; y < h; y++) {
        u8* restrict row = data + y * stride;
        for (u32 x = 0; x < w; x++) {
            u8 gray = (u8)((row[0] * 77 + row[1] * 150 + row[2] * 29) >> 8);
            u8 ctx[8];
            
            /* Neighborhood sampling via pointer offsets / 포인터 오프셋을 통한 주변부 샘플링 */
            int ci = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    i32 nx = (i32)x + dx, ny = (i32)y + dy;
                    if (__builtin_expect(nx >= 0 && nx < (i32)w && ny >= 0 && ny < (i32)h, 1)) {
                        u8* np = data + ny * stride + nx * bpp;
                        ctx[ci++] = (u8)((np[0] * 77 + np[1] * 150 + np[2] * 29) >> 8);
                    } else {
                        ctx[ci++] = gray;
                    }
                }
            }
            
            u8 r, g, b;
            mp_colorization_predict(gray, ctx, &r, &g, &b);
            row[0] = r; row[1] = g; row[2] = b;
            row += bpp;
        }
    }
    
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_COLORIZE, "Applied Spectral Colorization (Extreme Optimized)");
    return MP_SUCCESS;
}


mp_result mp_op_invert_grayscale(mp_image* image) {
    if (!image || !image->buffer) return MP_ERROR_INVALID_PARAM;
    
    mp_image_buffer* buffer = image->buffer;
    u8* restrict p = buffer->data;
    size_t count = (size_t)buffer->width * buffer->height;
    u32 bpp = buffer->bpp;
    
    /* Extreme optimization: Fixed-point + Unrolling + Direct manipulation / 극한 최적화: 고정 소수점 + 언롤링 + 직접 조작 */
    size_t i = 0;
    for (; i <= count - 8; i += 8) {
        #define IGS_STEP { \
            u8 ir = 255 - p[0], ig = 255 - p[1], ib = 255 - p[2]; \
            u32 gray = (ir * 77 + ig * 150 + ib * 29) >> 8; \
            p[0] = p[1] = p[2] = (u8)gray; p += bpp; \
        }
        IGS_STEP IGS_STEP IGS_STEP IGS_STEP
        IGS_STEP IGS_STEP IGS_STEP IGS_STEP
        #undef IGS_STEP
    }
    for (; i < count; i++) {
        u8 ir = 255 - p[0], ig = 255 - p[1], ib = 255 - p[2];
        u32 gray = (ir * 77 + ig * 150 + ib * 29) >> 8;
        p[0] = p[1] = p[2] = (u8)gray; p += bpp;
    }
    
    image->modified = MP_TRUE;
    mp_image_record_history(image, MP_OP_INVERT_GRAYSCALE, "Inverted and Grayscaled (Monster Optimized)");
    return MP_SUCCESS;
}

