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

/* Discrete Neural Network Engine
 * Pure C implementation of a Multi-Layer Perceptron (MLP) for image colorization.
 * Architecture: 9 (Input) -> 32 (Hidden) -> 16 (Hidden) -> 3 (Output)
 */

#define MP_NN_SIGMOID(x) (1.0f / (1.0f + expf(-(x))))
#define MP_NN_RELU(x) ((x) > 0.0f ? (x) : 0.0f)

void mp_colorization_predict(mp_colorization_network* network,
                             u8 gray, u8 context[8], u8* r, u8* g, u8* b) {
    if (!network || !network->weights) {
        *r = *g = *b = gray;
        return;
    }
    
    f32 input[9];
    input[0] = gray / 255.0f;
    for (int i = 0; i < 8; i++) input[i+1] = context[i] / 255.0f;
    
    f32 h1[32], h2[16], out[3];
    f32* w = network->weights;
    
    /* Layer 1: Input -> Hidden 32 (Matrix Multiplication + ReLU) */
    for (int i = 0; i < 32; i++) {
        f32 sum = 0.0f;
        for (int j = 0; j < 9; j++) sum += input[j] * w[j * 32 + i];
        h1[i] = MP_NN_RELU(sum);
    }
    w += 9 * 32;
    
    /* Layer 2: Hidden 32 -> Hidden 16 */
    for (int i = 0; i < 16; i++) {
        f32 sum = 0.0f;
        for (int j = 0; j < 32; j++) sum += h1[j] * w[j * 16 + i];
        h2[i] = MP_NN_RELU(sum);
    }
    w += 32 * 16;
    
    /* Layer 3: Hidden 16 -> Output 3 (Sigmoid for RGB) */
    for (int i = 0; i < 3; i++) {
        f32 sum = 0.0f;
        for (int j = 0; j < 16; j++) sum += h2[j] * w[j * 3 + i];
        out[i] = MP_NN_SIGMOID(sum);
    }
    
    *r = (u8)(out[0] * 255.0f);
    *g = (u8)(out[1] * 255.0f);
    *b = (u8)(out[2] * 255.0f);
}

mp_result mp_op_to_color(mp_image* image) {
    if (!image || !image->buffer) return MP_ERROR_INVALID_PARAM;
    
    mp_colorization_network* network = mp_colorization_network_create();
    if (!network) return MP_ERROR_MEMORY;
    
    mp_image_buffer* buffer = image->buffer;
    u32 w = buffer->width, h = buffer->height;
    
    /* Monster Loop for Per-Pixel Neural Inference */
    for (u32 y = 0; y < h; y++) {
        for (u32 x = 0; x < w; x++) {
            mp_pixel p = mp_image_get_pixel(buffer, x, y);
            u8 gray = mp_rgb_to_gray(p.r, p.g, p.b);
            u8 ctx[8];
            /* Static neighbor offset lookup */
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
            mp_colorization_predict(network, gray, ctx, &r, &g, &b);
            p.r = r; p.g = g; p.b = b;
            mp_image_set_pixel(buffer, x, y, p);
        }
    }
    
    mp_colorization_network_destroy(network);
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

