#ifndef MANYPICTURES_JPEG_H
#define MANYPICTURES_JPEG_H

#include "../core/types.h"

/* JPEG codec implementation */

/* JPEG markers */
#define JPEG_MARKER_SOI  0xFFD8  /* Start of Image */
#define JPEG_MARKER_EOI  0xFFD9  /* End of Image */
#define JPEG_MARKER_SOF0 0xFFC0  /* Start of Frame (Baseline DCT) */
#define JPEG_MARKER_SOF2 0xFFC2  /* Start of Frame (Progressive DCT) */
#define JPEG_MARKER_DHT  0xFFC4  /* Define Huffman Table */
#define JPEG_MARKER_DQT  0xFFDB  /* Define Quantization Table */
#define JPEG_MARKER_DRI  0xFFDD  /* Define Restart Interval */
#define JPEG_MARKER_SOS  0xFFDA  /* Start of Scan */
#define JPEG_MARKER_APP0 0xFFE0  /* JFIF marker */
#define JPEG_MARKER_APP1 0xFFE1  /* EXIF marker */
#define JPEG_MARKER_COM  0xFFFE  /* Comment */

/* JPEG component */
typedef struct {
    u8 id;
    u8 h_sampling;
    u8 v_sampling;
    u8 quant_table_id;
    u8 dc_table_id;
    u8 ac_table_id;
    i16 dc_predictor;
} jpeg_component;

/* JPEG decoder context */
typedef struct {
    u8* data;
    size_t size;
    size_t pos;
    u32 bit_buffer;
    u32 bit_count;
    
    u16 width;
    u16 height;
    u8 num_components;
    jpeg_component components[4];
    
    u8 quant_tables[4][64];
    mp_bool quant_table_defined[4];
    
    void* huffman_dc_tables[4];
    void* huffman_ac_tables[4];
    
    u16 restart_interval;
    u32 restart_count;
} jpeg_decoder;

/* JPEG encoder context */
typedef struct {
    u8* output;
    size_t output_size;
    size_t output_capacity;
    u32 bit_buffer;
    u32 bit_count;
    
    u8 quality;
    u8 quant_tables[4][64];
} jpeg_encoder;

/* Initialize JPEG decoder */
jpeg_decoder* mp_jpeg_decoder_create(const u8* data, size_t size);

/* Destroy JPEG decoder */
void mp_jpeg_decoder_destroy(jpeg_decoder* decoder);

/* Decode JPEG image */
mp_result mp_jpeg_decode(jpeg_decoder* decoder, mp_image_buffer** out_buffer);

/* Initialize JPEG encoder */
jpeg_encoder* mp_jpeg_encoder_create(u8 quality);

/* Destroy JPEG encoder */
void mp_jpeg_encoder_destroy(jpeg_encoder* encoder);

/* Encode image to JPEG */
mp_result mp_jpeg_encode(jpeg_encoder* encoder, const mp_image_buffer* buffer,
                         u8** out_data, size_t* out_size);

/* DCT and IDCT */
void mp_jpeg_fdct(const i16 input[64], i16 output[64]);
void mp_jpeg_idct(const i16 input[64], i16 output[64]);

#endif /* MANYPICTURES_JPEG_H */
