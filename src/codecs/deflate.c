#include "deflate.h"
#include "../core/memory.h"
#include <string.h>

/* Huffman tree node */
typedef struct mp_huffman_node {
    i16 symbol;
    struct mp_huffman_node* left;
    struct mp_huffman_node* right;
} mp_huffman_node;

/* Fixed Huffman tables for DEFLATE */
static const u16 g_length_base[29] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};

static const u8 g_length_extra[29] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

static const u16 g_dist_base[30] = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
    257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

static const u8 g_dist_extra[30] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

/* CRC32 table */
static u32 g_crc32_table[256];
static mp_bool g_crc32_initialized = MP_FALSE;

static void mp_crc32_init_table(void) {
    if (g_crc32_initialized) {
        return;
    }
    
    for (u32 i = 0; i < 256; i++) {
        u32 crc = i;
        for (u32 j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        g_crc32_table[i] = crc;
    }
    
    g_crc32_initialized = MP_TRUE;
}

u32 mp_crc32(const u8* data, size_t size) {
    return mp_crc32_update(0xFFFFFFFF, data, size) ^ 0xFFFFFFFF;
}

u32 mp_crc32_update(u32 crc, const u8* data, size_t size) {
    mp_crc32_init_table();
    
    for (size_t i = 0; i < size; i++) {
        crc = g_crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc;
}

u32 mp_adler32(const u8* data, size_t size) {
    u32 s1 = 1;
    u32 s2 = 0;
    
    for (size_t i = 0; i < size; i++) {
        s1 = (s1 + data[i]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    
    return (s2 << 16) | s1;
}

void mp_deflate_init(mp_deflate_stream* stream, const u8* input, size_t input_size,
                     u8* output, size_t output_size) {
    stream->input = input;
    stream->input_size = input_size;
    stream->input_pos = 0;
    stream->output = output;
    stream->output_size = output_size;
    stream->output_pos = 0;
    stream->bit_buffer = 0;
    stream->bit_count = 0;
}

static u32 mp_deflate_read_bits(mp_deflate_stream* stream, u32 count) {
    while (stream->bit_count < count) {
        if (stream->input_pos >= stream->input_size) {
            return 0;
        }
        stream->bit_buffer |= ((u32)stream->input[stream->input_pos++]) << stream->bit_count;
        stream->bit_count += 8;
    }
    
    u32 result = stream->bit_buffer & ((1 << count) - 1);
    stream->bit_buffer >>= count;
    stream->bit_count -= count;
    
    return result;
}

static void mp_deflate_write_byte(mp_deflate_stream* stream, u8 byte) {
    if (stream->output_pos < stream->output_size) {
        stream->output[stream->output_pos++] = byte;
    }
}

static mp_result mp_deflate_decompress_uncompressed(mp_deflate_stream* stream) {
    /* Align to byte boundary */
    stream->bit_count = 0;
    stream->bit_buffer = 0;
    
    if (stream->input_pos + 4 > stream->input_size) {
        return MP_ERROR_CORRUPTED;
    }
    
    u16 len = stream->input[stream->input_pos] | (stream->input[stream->input_pos + 1] << 8);
    u16 nlen = stream->input[stream->input_pos + 2] | (stream->input[stream->input_pos + 3] << 8);
    stream->input_pos += 4;
    
    if (len != (u16)~nlen) {
        return MP_ERROR_CORRUPTED;
    }
    
    if (stream->input_pos + len > stream->input_size) {
        return MP_ERROR_CORRUPTED;
    }
    
    for (u16 i = 0; i < len; i++) {
        mp_deflate_write_byte(stream, stream->input[stream->input_pos++]);
    }
    
    return MP_SUCCESS;
}

static mp_result mp_deflate_decompress_fixed(mp_deflate_stream* stream) {
    /* Simplified fixed Huffman implementation */
    /* In a full implementation, this would build proper Huffman trees */
    
    while (1) {
        u32 code = mp_deflate_read_bits(stream, 7);
        
        if (code <= 23) {
            /* Literal 256-279 */
            u32 symbol = code + 256;
            if (symbol == 256) {
                break; /* End of block */
            }
            mp_deflate_write_byte(stream, (u8)(symbol - 256));
        } else {
            /* This is a simplified version - full implementation needed */
            code = (code << 1) | mp_deflate_read_bits(stream, 1);
            
            if (code < 256) {
                mp_deflate_write_byte(stream, (u8)code);
            } else if (code == 256) {
                break;
            } else {
                /* Length/distance pair */
                u32 length_code = code - 257;
                if (length_code >= 29) {
                    return MP_ERROR_CORRUPTED;
                }
                
                u32 length = g_length_base[length_code];
                if (g_length_extra[length_code] > 0) {
                    length += mp_deflate_read_bits(stream, g_length_extra[length_code]);
                }
                
                u32 dist_code = mp_deflate_read_bits(stream, 5);
                if (dist_code >= 30) {
                    return MP_ERROR_CORRUPTED;
                }
                
                u32 distance = g_dist_base[dist_code];
                if (g_dist_extra[dist_code] > 0) {
                    distance += mp_deflate_read_bits(stream, g_dist_extra[dist_code]);
                }
                
                /* Copy from history */
                for (u32 i = 0; i < length; i++) {
                    if (distance > stream->output_pos) {
                        return MP_ERROR_CORRUPTED;
                    }
                    u8 byte = stream->output[stream->output_pos - distance];
                    mp_deflate_write_byte(stream, byte);
                }
            }
        }
    }
    
    return MP_SUCCESS;
}

mp_result mp_deflate_decompress(mp_deflate_stream* stream) {
    mp_bool is_final = MP_FALSE;
    
    while (!is_final) {
        is_final = mp_deflate_read_bits(stream, 1);
        u32 block_type = mp_deflate_read_bits(stream, 2);
        
        mp_result result;
        switch (block_type) {
            case 0: /* Uncompressed */
                result = mp_deflate_decompress_uncompressed(stream);
                break;
            case 1: /* Fixed Huffman */
                result = mp_deflate_decompress_fixed(stream);
                break;
            case 2: /* Dynamic Huffman */
                /* Full dynamic Huffman implementation would go here */
                return MP_ERROR_UNSUPPORTED;
            default:
                return MP_ERROR_CORRUPTED;
        }
        
        if (result != MP_SUCCESS) {
            return result;
        }
    }
    
    return MP_SUCCESS;
}

mp_result mp_deflate_compress(const u8* input, size_t input_size,
                              u8** output, size_t* output_size) {
    /* Simplified compression - just use uncompressed blocks */
    size_t max_block_size = 65535;
    size_t num_blocks = (input_size + max_block_size - 1) / max_block_size;
    size_t compressed_size = input_size + num_blocks * 5 + 6;
    
    u8* compressed = (u8*)mp_malloc(compressed_size);
    if (!compressed) {
        return MP_ERROR_MEMORY;
    }
    
    size_t out_pos = 0;
    size_t in_pos = 0;
    
    /* ZLIB header */
    compressed[out_pos++] = 0x78;
    compressed[out_pos++] = 0x01;
    
    for (size_t block = 0; block < num_blocks; block++) {
        size_t block_size = input_size - in_pos;
        if (block_size > max_block_size) {
            block_size = max_block_size;
        }
        
        mp_bool is_final = (block == num_blocks - 1);
        
        /* Block header */
        compressed[out_pos++] = is_final ? 0x01 : 0x00;
        
        /* Length and complement */
        compressed[out_pos++] = block_size & 0xFF;
        compressed[out_pos++] = (block_size >> 8) & 0xFF;
        compressed[out_pos++] = (~block_size) & 0xFF;
        compressed[out_pos++] = ((~block_size) >> 8) & 0xFF;
        
        /* Copy data */
        memcpy(compressed + out_pos, input + in_pos, block_size);
        out_pos += block_size;
        in_pos += block_size;
    }
    
    /* Adler32 checksum */
    u32 adler = mp_adler32(input, input_size);
    compressed[out_pos++] = (adler >> 24) & 0xFF;
    compressed[out_pos++] = (adler >> 16) & 0xFF;
    compressed[out_pos++] = (adler >> 8) & 0xFF;
    compressed[out_pos++] = adler & 0xFF;
    
    *output = compressed;
    *output_size = out_pos;
    
    return MP_SUCCESS;
}
