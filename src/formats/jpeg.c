#include "../core/types.h"
#include "../core/memory.h"
#include "../core/image.h"
#include "../codecs/jpeg.h"
#include <stdio.h>

/* JPEG format handler - uses custom JPEG codec */

mp_image* mp_jpeg_load(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return NULL;
    }
    
    /* Get file size */
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    /* Read entire file */
    u8* data = (u8*)mp_malloc(size);
    if (!data) {
        fclose(file);
        return NULL;
    }
    
    if (fread(data, 1, size, file) != size) {
        mp_free(data);
        fclose(file);
        return NULL;
    }
    
    fclose(file);
    
    /* Create decoder */
    jpeg_decoder* decoder = mp_jpeg_decoder_create(data, size);
    if (!decoder) {
        mp_free(data);
        return NULL;
    }
    
    /* Decode image */
    mp_image_buffer* buffer = NULL;
    mp_result result = mp_jpeg_decode(decoder, &buffer);
    
    mp_jpeg_decoder_destroy(decoder);
    mp_free(data);
    
    if (result != MP_SUCCESS || !buffer) {
        return NULL;
    }
    
    /* Create image structure */
    mp_image* image = (mp_image*)mp_malloc(sizeof(mp_image));
    if (!image) {
        mp_image_buffer_destroy(buffer);
        return NULL;
    }
    
    image->buffer = buffer;
    image->metadata = (mp_image_metadata*)mp_calloc(1, sizeof(mp_image_metadata));
    image->metadata->format = MP_FORMAT_JPEG;
    image->metadata->width = buffer->width;
    image->metadata->height = buffer->height;
    image->metadata->color_format = buffer->format;
    image->filepath = NULL;
    image->modified = MP_FALSE;
    image->history = NULL;
    
    return image;
}

mp_result mp_jpeg_save(mp_image* image, const char* filepath) {
    if (!image || !filepath) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    /* Create encoder with quality 90 */
    jpeg_encoder* encoder = mp_jpeg_encoder_create(90);
    if (!encoder) {
        return MP_ERROR_MEMORY;
    }
    
    /* Encode image */
    u8* data = NULL;
    size_t size = 0;
    mp_result result = mp_jpeg_encode(encoder, image->buffer, &data, &size);
    
    mp_jpeg_encoder_destroy(encoder);
    
    if (result != MP_SUCCESS) {
        return result;
    }
    
    /* Write to file */
    FILE* file = fopen(filepath, "wb");
    if (!file) {
        mp_free(data);
        return MP_ERROR_IO;
    }
    
    if (fwrite(data, 1, size, file) != size) {
        mp_free(data);
        fclose(file);
        return MP_ERROR_IO;
    }
    
    mp_free(data);
    fclose(file);
    
    return MP_SUCCESS;
}
