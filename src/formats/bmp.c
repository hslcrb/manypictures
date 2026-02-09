#include "../core/types.h"
#include "../core/memory.h"
#include "../core/image.h"
#include <stdio.h>
#include <string.h>

/* BMP file structures */
#pragma pack(push, 1)
typedef struct {
    u16 signature;
    u32 file_size;
    u16 reserved1;
    u16 reserved2;
    u32 data_offset;
} bmp_file_header;

typedef struct {
    u32 header_size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 image_size;
    i32 x_pixels_per_meter;
    i32 y_pixels_per_meter;
    u32 colors_used;
    u32 colors_important;
} bmp_info_header;
#pragma pack(pop)

#define BMP_SIGNATURE 0x4D42
#define BMP_BI_RGB 0
#define BMP_BI_RLE8 1
#define BMP_BI_RLE4 2
#define BMP_BI_BITFIELDS 3

mp_image* mp_bmp_load(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return NULL;
    }
    
    bmp_file_header file_header;
    if (fread(&file_header, sizeof(bmp_file_header), 1, file) != 1) {
        fclose(file);
        return NULL;
    }
    
    if (file_header.signature != BMP_SIGNATURE) {
        fclose(file);
        return NULL;
    }
    
    bmp_info_header info_header;
    if (fread(&info_header, sizeof(bmp_info_header), 1, file) != 1) {
        fclose(file);
        return NULL;
    }
    
    /* Only support uncompressed RGB for now */
    if (info_header.compression != BMP_BI_RGB) {
        fclose(file);
        return NULL;
    }
    
    u32 width = info_header.width;
    u32 height = info_header.height < 0 ? -info_header.height : info_header.height;
    mp_bool top_down = info_header.height < 0;
    
    mp_color_format format = MP_COLOR_FORMAT_RGBA;
    if (info_header.bits_per_pixel == 24) {
        format = MP_COLOR_FORMAT_RGB;
    } else if (info_header.bits_per_pixel == 32) {
        format = MP_COLOR_FORMAT_RGBA;
    } else if (info_header.bits_per_pixel == 8) {
        format = MP_COLOR_FORMAT_GRAYSCALE;
    } else {
        fclose(file);
        return NULL;
    }
    
    mp_image* image = mp_image_create(width, height, format);
    if (!image) {
        fclose(file);
        return NULL;
    }
    
    /* Read palette if present */
    u8 palette[256][4];
    if (info_header.bits_per_pixel == 8) {
        u32 palette_size = info_header.colors_used ? info_header.colors_used : 256;
        fseek(file, sizeof(bmp_file_header) + info_header.header_size, SEEK_SET);
        fread(palette, 4, palette_size, file);
    }
    
    /* Seek to pixel data */
    fseek(file, file_header.data_offset, SEEK_SET);
    
    /* BMP rows are padded to 4-byte boundaries */
    u32 bytes_per_pixel = info_header.bits_per_pixel / 8;
    u32 row_size = ((width * bytes_per_pixel + 3) / 4) * 4;
    u8* row_buffer = (u8*)mp_malloc(row_size);
    
    if (!row_buffer) {
        mp_image_destroy(image);
        fclose(file);
        return NULL;
    }
    
    /* Read pixel data */
    for (u32 y = 0; y < height; y++) {
        u32 actual_y = top_down ? y : (height - 1 - y);
        
        if (fread(row_buffer, 1, row_size, file) != row_size) {
            mp_free(row_buffer);
            mp_image_destroy(image);
            fclose(file);
            return NULL;
        }
        
        for (u32 x = 0; x < width; x++) {
            mp_pixel pixel;
            
            if (info_header.bits_per_pixel == 8) {
                u8 index = row_buffer[x];
                pixel.b = palette[index][0];
                pixel.g = palette[index][1];
                pixel.r = palette[index][2];
                pixel.a = 255;
            } else if (info_header.bits_per_pixel == 24) {
                u32 offset = x * 3;
                pixel.b = row_buffer[offset];
                pixel.g = row_buffer[offset + 1];
                pixel.r = row_buffer[offset + 2];
                pixel.a = 255;
            } else if (info_header.bits_per_pixel == 32) {
                u32 offset = x * 4;
                pixel.b = row_buffer[offset];
                pixel.g = row_buffer[offset + 1];
                pixel.r = row_buffer[offset + 2];
                pixel.a = row_buffer[offset + 3];
            }
            
            mp_image_set_pixel(image->buffer, x, actual_y, pixel);
        }
    }
    
    mp_free(row_buffer);
    fclose(file);
    
    return image;
}

mp_result mp_bmp_save(mp_image* image, const char* filepath) {
    if (!image || !filepath) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(filepath, "wb");
    if (!file) {
        return MP_ERROR_IO;
    }
    
    u32 width = image->buffer->width;
    u32 height = image->buffer->height;
    u16 bits_per_pixel = 24;
    u32 bytes_per_pixel = 3;
    
    /* Calculate row size with padding */
    u32 row_size = ((width * bytes_per_pixel + 3) / 4) * 4;
    u32 image_size = row_size * height;
    
    /* Write file header */
    bmp_file_header file_header;
    file_header.signature = BMP_SIGNATURE;
    file_header.file_size = sizeof(bmp_file_header) + sizeof(bmp_info_header) + image_size;
    file_header.reserved1 = 0;
    file_header.reserved2 = 0;
    file_header.data_offset = sizeof(bmp_file_header) + sizeof(bmp_info_header);
    
    if (fwrite(&file_header, sizeof(bmp_file_header), 1, file) != 1) {
        fclose(file);
        return MP_ERROR_IO;
    }
    
    /* Write info header */
    bmp_info_header info_header;
    info_header.header_size = sizeof(bmp_info_header);
    info_header.width = width;
    info_header.height = height;
    info_header.planes = 1;
    info_header.bits_per_pixel = bits_per_pixel;
    info_header.compression = BMP_BI_RGB;
    info_header.image_size = image_size;
    info_header.x_pixels_per_meter = 2835;
    info_header.y_pixels_per_meter = 2835;
    info_header.colors_used = 0;
    info_header.colors_important = 0;
    
    if (fwrite(&info_header, sizeof(bmp_info_header), 1, file) != 1) {
        fclose(file);
        return MP_ERROR_IO;
    }
    
    /* Write pixel data */
    u8* row_buffer = (u8*)mp_calloc(1, row_size);
    if (!row_buffer) {
        fclose(file);
        return MP_ERROR_MEMORY;
    }
    
    for (u32 y = 0; y < height; y++) {
        u32 actual_y = height - 1 - y;
        
        for (u32 x = 0; x < width; x++) {
            mp_pixel pixel = mp_image_get_pixel(image->buffer, x, actual_y);
            u32 offset = x * 3;
            row_buffer[offset] = pixel.b;
            row_buffer[offset + 1] = pixel.g;
            row_buffer[offset + 2] = pixel.r;
        }
        
        if (fwrite(row_buffer, 1, row_size, file) != row_size) {
            mp_free(row_buffer);
            fclose(file);
            return MP_ERROR_IO;
        }
    }
    
    mp_free(row_buffer);
    fclose(file);
    
    return MP_SUCCESS;
}
