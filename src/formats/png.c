#include "../core/types.h"
#include "../core/memory.h"
#include "../core/image.h"
#include "../codecs/deflate.h"
#include <stdio.h>
#include <string.h>

/* PNG chunk types */
#define PNG_CHUNK_IHDR 0x49484452
#define PNG_CHUNK_PLTE 0x504C5445
#define PNG_CHUNK_IDAT 0x49444154
#define PNG_CHUNK_IEND 0x49454E44
#define PNG_CHUNK_tRNS 0x74524E53
#define PNG_CHUNK_gAMA 0x67414D41
#define PNG_CHUNK_cHRM 0x6348524D
#define PNG_CHUNK_sRGB 0x73524742
#define PNG_CHUNK_iCCP 0x69434350
#define PNG_CHUNK_tEXt 0x74455874
#define PNG_CHUNK_zTXt 0x7A545874
#define PNG_CHUNK_iTXt 0x69545874

/* PNG color types */
#define PNG_COLOR_GRAYSCALE 0
#define PNG_COLOR_RGB 2
#define PNG_COLOR_PALETTE 3
#define PNG_COLOR_GRAYSCALE_ALPHA 4
#define PNG_COLOR_RGBA 6

/* PNG filter types */
#define PNG_FILTER_NONE 0
#define PNG_FILTER_SUB 1
#define PNG_FILTER_UP 2
#define PNG_FILTER_AVERAGE 3
#define PNG_FILTER_PAETH 4

typedef struct {
    u32 width;
    u32 height;
    u8 bit_depth;
    u8 color_type;
    u8 compression;
    u8 filter;
    u8 interlace;
} png_ihdr;

static u32 mp_read_u32_be(const u8* data) {
    return ((u32)data[0] << 24) | ((u32)data[1] << 16) | 
           ((u32)data[2] << 8) | (u32)data[3];
}

static void mp_write_u32_be(u8* data, u32 value) {
    data[0] = (value >> 24) & 0xFF;
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;
}

static u8 mp_paeth_predictor(u8 a, u8 b, u8 c) {
    i32 p = a + b - c;
    i32 pa = p > a ? p - a : a - p;
    i32 pb = p > b ? p - b : b - p;
    i32 pc = p > c ? p - c : c - p;
    
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

static void mp_png_unfilter_scanline(u8* scanline, const u8* prev_scanline, 
                                     u32 width, u32 bytes_per_pixel, u8 filter_type) {
    switch (filter_type) {
        case PNG_FILTER_NONE:
            break;
            
        case PNG_FILTER_SUB:
            for (u32 i = bytes_per_pixel; i < width; i++) {
                scanline[i] = (scanline[i] + scanline[i - bytes_per_pixel]) & 0xFF;
            }
            break;
            
        case PNG_FILTER_UP:
            if (prev_scanline) {
                for (u32 i = 0; i < width; i++) {
                    scanline[i] = (scanline[i] + prev_scanline[i]) & 0xFF;
                }
            }
            break;
            
        case PNG_FILTER_AVERAGE:
            for (u32 i = 0; i < width; i++) {
                u8 a = (i >= bytes_per_pixel) ? scanline[i - bytes_per_pixel] : 0;
                u8 b = prev_scanline ? prev_scanline[i] : 0;
                scanline[i] = (scanline[i] + ((a + b) / 2)) & 0xFF;
            }
            break;
            
        case PNG_FILTER_PAETH:
            for (u32 i = 0; i < width; i++) {
                u8 a = (i >= bytes_per_pixel) ? scanline[i - bytes_per_pixel] : 0;
                u8 b = prev_scanline ? prev_scanline[i] : 0;
                u8 c = (prev_scanline && i >= bytes_per_pixel) ? prev_scanline[i - bytes_per_pixel] : 0;
                scanline[i] = (scanline[i] + mp_paeth_predictor(a, b, c)) & 0xFF;
            }
            break;
    }
}

mp_image* mp_png_load(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return NULL;
    }
    
    /* Check PNG signature */
    u8 signature[8];
    if (fread(signature, 1, 8, file) != 8) {
        fclose(file);
        return NULL;
    }
    
    const u8 png_sig[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    if (memcmp(signature, png_sig, 8) != 0) {
        fclose(file);
        return NULL;
    }
    
    png_ihdr ihdr;
    u8* idat_data = NULL;
    size_t idat_size = 0;
    size_t idat_capacity = 0;
    
    /* Read chunks */
    while (1) {
        u8 chunk_header[8];
        if (fread(chunk_header, 1, 8, file) != 8) {
            break;
        }
        
        u32 chunk_length = mp_read_u32_be(chunk_header);
        u32 chunk_type = mp_read_u32_be(chunk_header + 4);
        
        if (chunk_type == PNG_CHUNK_IHDR) {
            u8 ihdr_data[13];
            if (fread(ihdr_data, 1, 13, file) != 13) {
                fclose(file);
                return NULL;
            }
            
            ihdr.width = mp_read_u32_be(ihdr_data);
            ihdr.height = mp_read_u32_be(ihdr_data + 4);
            ihdr.bit_depth = ihdr_data[8];
            ihdr.color_type = ihdr_data[9];
            ihdr.compression = ihdr_data[10];
            ihdr.filter = ihdr_data[11];
            ihdr.interlace = ihdr_data[12];
            
            fseek(file, 4, SEEK_CUR); /* Skip CRC */
        } else if (chunk_type == PNG_CHUNK_IDAT) {
            if (idat_size + chunk_length > idat_capacity) {
                idat_capacity = (idat_size + chunk_length) * 2;
                u8* new_data = (u8*)mp_realloc(idat_data, idat_capacity);
                if (!new_data) {
                    mp_free(idat_data);
                    fclose(file);
                    return NULL;
                }
                idat_data = new_data;
            }
            
            if (fread(idat_data + idat_size, 1, chunk_length, file) != chunk_length) {
                mp_free(idat_data);
                fclose(file);
                return NULL;
            }
            
            idat_size += chunk_length;
            fseek(file, 4, SEEK_CUR); /* Skip CRC */
        } else if (chunk_type == PNG_CHUNK_IEND) {
            fseek(file, 4, SEEK_CUR); /* Skip CRC */
            break;
        } else {
            /* Skip unknown chunk */
            fseek(file, chunk_length + 4, SEEK_CUR);
        }
    }
    
    fclose(file);
    
    if (!idat_data) {
        return NULL;
    }
    
    /* Determine color format */
    mp_color_format format;
    u32 bytes_per_pixel;
    
    switch (ihdr.color_type) {
        case PNG_COLOR_GRAYSCALE:
            format = MP_COLOR_FORMAT_GRAYSCALE;
            bytes_per_pixel = 1;
            break;
        case PNG_COLOR_RGB:
            format = MP_COLOR_FORMAT_RGB;
            bytes_per_pixel = 3;
            break;
        case PNG_COLOR_RGBA:
            format = MP_COLOR_FORMAT_RGBA;
            bytes_per_pixel = 4;
            break;
        case PNG_COLOR_GRAYSCALE_ALPHA:
            format = MP_COLOR_FORMAT_GRAYSCALE_ALPHA;
            bytes_per_pixel = 2;
            break;
        default:
            mp_free(idat_data);
            return NULL;
    }
    
    /* Decompress IDAT */
    size_t scanline_size = ihdr.width * bytes_per_pixel + 1; /* +1 for filter byte */
    size_t raw_size = scanline_size * ihdr.height;
    u8* raw_data = (u8*)mp_malloc(raw_size);
    
    if (!raw_data) {
        mp_free(idat_data);
        return NULL;
    }
    
    /* Skip ZLIB header (2 bytes) */
    mp_deflate_stream stream;
    mp_deflate_init(&stream, idat_data + 2, idat_size - 2, raw_data, raw_size);
    
    mp_result result = mp_deflate_decompress(&stream);
    mp_free(idat_data);
    
    if (result != MP_SUCCESS) {
        mp_free(raw_data);
        return NULL;
    }
    
    /* Create image */
    mp_image* image = mp_image_create(ihdr.width, ihdr.height, format);
    if (!image) {
        mp_free(raw_data);
        return NULL;
    }
    
    /* Unfilter and copy scanlines */
    u8* prev_scanline = NULL;
    for (u32 y = 0; y < ihdr.height; y++) {
        u8* scanline = raw_data + y * scanline_size;
        u8 filter_type = scanline[0];
        u8* pixel_data = scanline + 1;
        
        mp_png_unfilter_scanline(pixel_data, prev_scanline, 
                                 ihdr.width * bytes_per_pixel, bytes_per_pixel, filter_type);
        
        /* Copy to image buffer */
        memcpy(image->buffer->data + y * image->buffer->stride, pixel_data, 
               ihdr.width * bytes_per_pixel);
        
        prev_scanline = pixel_data;
    }
    
    mp_free(raw_data);
    
    return image;
}

mp_result mp_png_save(mp_image* image, const char* filepath) {
    if (!image || !filepath) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(filepath, "wb");
    if (!file) {
        return MP_ERROR_IO;
    }
    
    /* Write PNG signature */
    const u8 png_sig[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    fwrite(png_sig, 1, 8, file);
    
    /* Determine PNG color type */
    u8 color_type;
    u32 bytes_per_pixel;
    
    switch (image->buffer->format) {
        case MP_COLOR_FORMAT_GRAYSCALE:
            color_type = PNG_COLOR_GRAYSCALE;
            bytes_per_pixel = 1;
            break;
        case MP_COLOR_FORMAT_RGB:
        case MP_COLOR_FORMAT_BGR:
            color_type = PNG_COLOR_RGB;
            bytes_per_pixel = 3;
            break;
        case MP_COLOR_FORMAT_RGBA:
        case MP_COLOR_FORMAT_BGRA:
            color_type = PNG_COLOR_RGBA;
            bytes_per_pixel = 4;
            break;
        default:
            fclose(file);
            return MP_ERROR_UNSUPPORTED;
    }
    
    /* Write IHDR chunk */
    u8 ihdr_data[25];
    mp_write_u32_be(ihdr_data, 13); /* Length */
    mp_write_u32_be(ihdr_data + 4, PNG_CHUNK_IHDR);
    mp_write_u32_be(ihdr_data + 8, image->buffer->width);
    mp_write_u32_be(ihdr_data + 12, image->buffer->height);
    ihdr_data[16] = 8; /* Bit depth */
    ihdr_data[17] = color_type;
    ihdr_data[18] = 0; /* Compression */
    ihdr_data[19] = 0; /* Filter */
    ihdr_data[20] = 0; /* Interlace */
    u32 ihdr_crc = mp_crc32(ihdr_data + 4, 17);
    mp_write_u32_be(ihdr_data + 21, ihdr_crc);
    fwrite(ihdr_data, 1, 25, file);
    
    /* Prepare image data with filter bytes */
    size_t scanline_size = image->buffer->width * bytes_per_pixel + 1;
    size_t raw_size = scanline_size * image->buffer->height;
    u8* raw_data = (u8*)mp_malloc(raw_size);
    
    if (!raw_data) {
        fclose(file);
        return MP_ERROR_MEMORY;
    }
    
    for (u32 y = 0; y < image->buffer->height; y++) {
        raw_data[y * scanline_size] = PNG_FILTER_NONE;
        memcpy(raw_data + y * scanline_size + 1,
               image->buffer->data + y * image->buffer->stride,
               image->buffer->width * bytes_per_pixel);
    }
    
    /* Compress data */
    u8* compressed_data;
    size_t compressed_size;
    
    mp_result result = mp_deflate_compress(raw_data, raw_size, &compressed_data, &compressed_size);
    mp_free(raw_data);
    
    if (result != MP_SUCCESS) {
        fclose(file);
        return result;
    }
    
    /* Write IDAT chunk */
    u8 idat_header[8];
    mp_write_u32_be(idat_header, compressed_size);
    mp_write_u32_be(idat_header + 4, PNG_CHUNK_IDAT);
    fwrite(idat_header, 1, 8, file);
    fwrite(compressed_data, 1, compressed_size, file);
    
    u32 idat_crc = mp_crc32_update(0xFFFFFFFF, idat_header + 4, 4);
    idat_crc = mp_crc32_update(idat_crc, compressed_data, compressed_size) ^ 0xFFFFFFFF;
    
    u8 crc_bytes[4];
    mp_write_u32_be(crc_bytes, idat_crc);
    fwrite(crc_bytes, 1, 4, file);
    
    mp_free(compressed_data);
    
    /* Write IEND chunk */
    u8 iend_data[12];
    mp_write_u32_be(iend_data, 0); /* Length */
    mp_write_u32_be(iend_data + 4, PNG_CHUNK_IEND);
    u32 iend_crc = mp_crc32(iend_data + 4, 4);
    mp_write_u32_be(iend_data + 8, iend_crc);
    fwrite(iend_data, 1, 12, file);
    
    fclose(file);
    
    return MP_SUCCESS;
}
