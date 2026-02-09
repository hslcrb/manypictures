#include "color_ops.h"
#include "../core/memory.h"
#include "../core/image.h"
#include <math.h>
#include <string.h>

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

mp_result mp_op_invert_grayscale(mp_image* image) {
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
            u8 gray = mp_rgb_to_gray(pixel.r, pixel.g, pixel.b);
            pixel.r = pixel.g = pixel.b = gray;
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

/* Simplified colorization network */
mp_colorization_network* mp_colorization_network_create(void) {
    mp_colorization_network* network = (mp_colorization_network*)mp_malloc(sizeof(mp_colorization_network));
    if (!network) {
        return NULL;
    }
    
    /* Simple 3-layer network: input(9) -> hidden(32) -> hidden(16) -> output(3) */
    network->num_layers = 4;
    network->layer_sizes[0] = 9;  /* gray + 8 context pixels */
    network->layer_sizes[1] = 32;
    network->layer_sizes[2] = 16;
    network->layer_sizes[3] = 3;  /* RGB output */
    
    /* Allocate weights (simplified - would need proper initialization) */
    u32 total_weights = (9 * 32) + (32 * 16) + (16 * 3);
    network->weights = (f32*)mp_malloc(total_weights * sizeof(f32));
    
    if (!network->weights) {
        mp_free(network);
        return NULL;
    }
    
    /* Initialize with simple heuristic weights */
    for (u32 i = 0; i < total_weights; i++) {
        network->weights[i] = ((f32)(i % 100) - 50.0f) / 100.0f;
    }
    
    return network;
}

void mp_colorization_network_destroy(mp_colorization_network* network) {
    if (!network) {
        return;
    }
    
    if (network->weights) {
        mp_free(network->weights);
    }
    
    mp_free(network);
}

void mp_colorization_predict(mp_colorization_network* network,
                             u8 gray, u8 context[8], u8* r, u8* g, u8* b) {
    if (!network) {
        *r = *g = *b = gray;
        return;
    }
    
    /* Simplified prediction - in real implementation would do full forward pass */
    /* For now, use heuristic based on gray value and context */
    
    f32 gray_f = gray / 255.0f;
    f32 context_avg = 0.0f;
    for (u32 i = 0; i < 8; i++) {
        context_avg += context[i] / 255.0f;
    }
    context_avg /= 8.0f;
    
    /* Simple heuristic colorization */
    if (gray_f > 0.7f) {
        /* Bright areas - slight warm tint */
        *r = gray + (u8)((255 - gray) * 0.1f);
        *g = gray;
        *b = gray - (u8)(gray * 0.05f);
    } else if (gray_f < 0.3f) {
        /* Dark areas - slight cool tint */
        *r = gray - (u8)(gray * 0.05f);
        *g = gray;
        *b = gray + (u8)((255 - gray) * 0.1f);
    } else {
        /* Mid-tones - neutral */
        *r = *g = *b = gray;
    }
}

mp_result mp_op_to_color(mp_image* image) {
    if (!image || !image->buffer) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_colorization_network* network = mp_colorization_network_create();
    if (!network) {
        return MP_ERROR_MEMORY;
    }
    
    mp_image_buffer* buffer = image->buffer;
    
    for (u32 y = 0; y < buffer->height; y++) {
        for (u32 x = 0; x < buffer->width; x++) {
            mp_pixel pixel = mp_image_get_pixel(buffer, x, y);
            u8 gray = mp_rgb_to_gray(pixel.r, pixel.g, pixel.b);
            
            /* Get context pixels */
            u8 context[8];
            i32 dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
            i32 dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
            
            for (u32 i = 0; i < 8; i++) {
                i32 nx = x + dx[i];
                i32 ny = y + dy[i];
                
                if (nx >= 0 && nx < (i32)buffer->width && ny >= 0 && ny < (i32)buffer->height) {
                    mp_pixel ctx_pixel = mp_image_get_pixel(buffer, nx, ny);
                    context[i] = mp_rgb_to_gray(ctx_pixel.r, ctx_pixel.g, ctx_pixel.b);
                } else {
                    context[i] = gray;
                }
            }
            
            u8 r, g, b;
            mp_colorization_predict(network, gray, context, &r, &g, &b);
            
            pixel.r = r;
            pixel.g = g;
            pixel.b = b;
            mp_image_set_pixel(buffer, x, y, pixel);
        }
    }
    
    mp_colorization_network_destroy(network);
    
    image->modified = MP_TRUE;
    return MP_SUCCESS;
}
