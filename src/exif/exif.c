#include "exif.h"
#include "../core/memory.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

/* EXIF data handler with custom history tracking */

mp_exif_data* mp_exif_create(void) {
    mp_exif_data* exif = (mp_exif_data*)mp_calloc(1, sizeof(mp_exif_data));
    if (!exif) {
        return NULL;
    }
    
    /* Set default software tag */
    strncpy(exif->software, "Many Pictures v1.0", sizeof(exif->software) - 1);
    
    /* Get current time */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(exif->datetime, sizeof(exif->datetime), "%Y:%m:%d %H:%M:%S", tm_info);
    
    return exif;
}

void mp_exif_destroy(mp_exif_data* exif) {
    if (!exif) {
        return;
    }
    
    if (exif->history) {
        mp_history_entry* entry = exif->history->head;
        while (entry) {
            mp_history_entry* next = entry->next;
            mp_free(entry);
            entry = next;
        }
        mp_free(exif->history);
    }
    
    if (exif->raw_data) {
        mp_free(exif->raw_data);
    }
    
    mp_free(exif);
}

mp_result mp_exif_add_history(mp_exif_data* exif, mp_operation_type op_type,
                              const char* description, const u8* params, u32 param_size) {
    if (!exif) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    /* Create history chain if it doesn't exist */
    if (!exif->history) {
        exif->history = (mp_history_chain*)mp_calloc(1, sizeof(mp_history_chain));
        if (!exif->history) {
            return MP_ERROR_MEMORY;
        }
        exif->history->max_entries = 100;
    }
    
    /* Create new history entry */
    mp_history_entry* entry = (mp_history_entry*)mp_calloc(1, sizeof(mp_history_entry));
    if (!entry) {
        return MP_ERROR_MEMORY;
    }
    
    entry->op_type = op_type;
    entry->timestamp = (u64)time(NULL);
    
    if (description) {
        strncpy(entry->description, description, sizeof(entry->description) - 1);
    }
    
    if (params && param_size > 0) {
        if (param_size > sizeof(entry->params)) {
            param_size = sizeof(entry->params);
        }
        memcpy(entry->params, params, param_size);
        entry->param_size = param_size;
    }
    
    /* Calculate checksum (simplified - would use SHA-256 in full implementation) */
    u32 checksum = 0;
    for (u32 i = 0; i < param_size; i++) {
        checksum = (checksum * 31) + params[i];
    }
    memcpy(entry->checksum, &checksum, sizeof(u32));
    
    /* Add to chain */
    if (!exif->history->head) {
        exif->history->head = entry;
        exif->history->tail = entry;
    } else {
        exif->history->tail->next = entry;
        entry->prev = exif->history->tail;
        exif->history->tail = entry;
    }
    
    exif->history->count++;
    
    /* Remove old entries if exceeding max */
    while (exif->history->count > exif->history->max_entries) {
        mp_history_entry* old = exif->history->head;
        exif->history->head = old->next;
        if (exif->history->head) {
            exif->history->head->prev = NULL;
        }
        mp_free(old);
        exif->history->count--;
    }
    
    return MP_SUCCESS;
}

mp_history_chain* mp_exif_get_history(const mp_exif_data* exif) {
    if (!exif) {
        return NULL;
    }
    return exif->history;
}

mp_result mp_exif_restore_history(mp_image* image, u32 history_index) {
    if (!image || !image->metadata || !image->metadata->exif) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_history_chain* history = image->metadata->exif->history;
    if (!history || history_index >= history->count) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    /* Find the history entry */
    mp_history_entry* entry = history->head;
    for (u32 i = 0; i < history_index && entry; i++) {
        entry = entry->next;
    }
    
    if (!entry) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    /* TODO: Implement actual restoration by replaying operations */
    /* This would involve:
     * 1. Loading original image
     * 2. Replaying all operations up to history_index
     * 3. Updating current image buffer
     */
    
    return MP_ERROR_UNSUPPORTED;
}

mp_exif_data* mp_exif_read_jpeg(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        return NULL;
    }
    
    /* Read JPEG markers looking for APP1 (EXIF) */
    u8 marker[2];
    while (fread(marker, 1, 2, file) == 2) {
        if (marker[0] != 0xFF) {
            break;
        }
        
        if (marker[1] == 0xE1) { /* APP1 - EXIF */
            u8 size_bytes[2];
            if (fread(size_bytes, 1, 2, file) != 2) {
                break;
            }
            
            u16 size = (size_bytes[0] << 8) | size_bytes[1];
            
            /* Read EXIF data */
            u8* exif_data = (u8*)mp_malloc(size);
            if (!exif_data) {
                fclose(file);
                return NULL;
            }
            
            if (fread(exif_data, 1, size - 2, file) != size - 2) {
                mp_free(exif_data);
                fclose(file);
                return NULL;
            }
            
            /* Parse EXIF data */
            mp_exif_data* exif = mp_exif_read_buffer(exif_data, size - 2);
            mp_free(exif_data);
            fclose(file);
            
            return exif;
        } else if (marker[1] == 0xDA) { /* SOS - Start of Scan */
            break;
        } else {
            /* Skip this marker */
            u8 size_bytes[2];
            if (fread(size_bytes, 1, 2, file) != 2) {
                break;
            }
            u16 size = (size_bytes[0] << 8) | size_bytes[1];
            fseek(file, size - 2, SEEK_CUR);
        }
    }
    
    fclose(file);
    return NULL;
}

mp_exif_data* mp_exif_read_buffer(const u8* data, size_t size) {
    if (!data || size < 6) {
        return NULL;
    }
    
    /* Check for "Exif\0\0" header */
    if (memcmp(data, "Exif\0\0", 6) != 0) {
        return NULL;
    }
    
    mp_exif_data* exif = mp_exif_create();
    if (!exif) {
        return NULL;
    }
    
    /* Store raw EXIF data */
    exif->raw_data = (u8*)mp_malloc(size);
    if (exif->raw_data) {
        memcpy(exif->raw_data, data, size);
        exif->raw_data_size = size;
    }
    
    /* TODO: Parse EXIF IFD structure */
    /* This would involve:
     * 1. Reading TIFF header (byte order, IFD offset)
     * 2. Parsing IFD entries
     * 3. Extracting standard tags
     * 4. Looking for custom Many Pictures tags
     * 5. Reconstructing history chain
     */
    
    return exif;
}

mp_result mp_exif_write_jpeg(const char* filepath, const mp_exif_data* exif) {
    if (!filepath || !exif) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    /* TODO: Implement EXIF writing */
    /* This would involve:
     * 1. Reading original JPEG
     * 2. Removing old APP1 marker
     * 3. Creating new EXIF data with history
     * 4. Inserting APP1 marker after SOI
     * 5. Writing modified JPEG
     */
    
    return MP_ERROR_UNSUPPORTED;
}

mp_result mp_exif_write_buffer(const mp_exif_data* exif, u8** out_data, size_t* out_size) {
    if (!exif || !out_data || !out_size) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    /* TODO: Implement EXIF buffer writing */
    /* This would create a complete EXIF APP1 segment including:
     * 1. APP1 marker (0xFFE1)
     * 2. Size field
     * 3. "Exif\0\0" identifier
     * 4. TIFF header
     * 5. IFD0 with standard tags
     * 6. Custom IFD with Many Pictures history
     */
    
    return MP_ERROR_UNSUPPORTED;
}
