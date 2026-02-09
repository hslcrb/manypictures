#ifndef MANYPICTURES_COLOR_OPS_H
#define MANYPICTURES_COLOR_OPS_H

#include "../core/types.h"

/* Color manipulation operations */

/* Convert image to grayscale */
mp_result mp_op_to_grayscale(mp_image* image);

/* Convert grayscale to color (AI-based approximation) */
mp_result mp_op_to_color(mp_image* image);

/* Invert colors */
mp_result mp_op_invert(mp_image* image);

/* Invert and convert to grayscale in one operation */
mp_result mp_op_invert_grayscale(mp_image* image);

/* Adjust brightness (-255 to 255) */
mp_result mp_op_brightness(mp_image* image, i32 value);

/* Adjust contrast (0.0 to 2.0) */
mp_result mp_op_contrast(mp_image* image, f32 value);

/* Adjust saturation (0.0 to 2.0) */
mp_result mp_op_saturation(mp_image* image, f32 value);

/* Adjust hue (-180 to 180 degrees) */
mp_result mp_op_hue(mp_image* image, i32 degrees);

/* Convert RGB to HSV */
void mp_rgb_to_hsv(u8 r, u8 g, u8 b, f32* h, f32* s, f32* v);

/* Convert HSV to RGB */
void mp_hsv_to_rgb(f32 h, f32 s, f32 v, u8* r, u8* g, u8* b);

/* Predict color for grayscale pixel (Spectral Projection) */
void mp_colorization_predict(u8 gray, u8 context[8], u8* r, u8* g, u8* b);

#endif /* MANYPICTURES_COLOR_OPS_H */
