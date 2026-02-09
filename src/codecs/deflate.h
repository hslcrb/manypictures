#ifndef MANYPICTURES_DEFLATE_H
#define MANYPICTURES_DEFLATE_H

#include "../core/types.h"

/* Custom DEFLATE decompression implementation */

typedef struct {
    const u8* input;
    size_t input_size;
    size_t input_pos;
    u8* output;
    size_t output_size;
    size_t output_pos;
    u32 bit_buffer;
    u32 bit_count;
} mp_deflate_stream;

/* Initialize deflate stream / 디플레이트 스트림 초기화 */
void mp_deflate_init(mp_deflate_stream* stream, const u8* input, size_t input_size, 
                     u8* output, size_t output_size);

/* Decompress DEFLATE data / DEFLATE 데이터 압축 해제 */
mp_result mp_deflate_decompress(mp_deflate_stream* stream);

/* Compress data using DEFLATE / DEFLATE를 사용하여 데이터 압축 */
mp_result mp_deflate_compress(const u8* input, size_t input_size, 
                              u8** output, size_t* output_size);

/* CRC32 calculation / CRC32 계산 */
u32 mp_crc32(const u8* data, size_t size);
u32 mp_crc32_update(u32 crc, const u8* data, size_t size);

/* Adler32 checksum / Adler32 체크섬 */
u32 mp_adler32(const u8* data, size_t size);

#endif /* MANYPICTURES_DEFLATE_H */
