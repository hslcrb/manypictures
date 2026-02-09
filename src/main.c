#include "core/types.h"
#include "core/memory.h"
#include "core/image.h"
#include "core/fast_io.h"
#include "operations/color_ops.h"
#include "operations/edit_ops.h"
#include "gui/gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Many Pictures - Advanced Image Viewer and Editor / 고성능 이미지 뷰어 및 편집기
 * 
 * Features / 주요 기능:
 * - Support for BMP, PNG, JPEG, GIF, TIFF, WebP formats / BMP, PNG, JPEG, GIF, TIFF, WebP 포맷 지원
 * - Video support (AVI, MP4, MKV, WebM, MOV, FLV) / 비디오 지원 (AVI, MP4, MKV, WebM, MOV, FLV)
 * - Color to grayscale conversion / 색상-흑백 변환
 * - Grayscale to color conversion (AI-based) / 흑백-색상 변환 (AI 기반)
 * - Color inversion / 색상 반전
 * - Combined invert + grayscale operation / 반전 + 흑백 통합 연산
 * - EXIF metadata with custom history tracking / 독자 히스토리 추적이 포함된 EXIF 메타데이터
 * - Git-like history restoration from EXIF / EXIF 기반 Git 스타일 히스토리 복원
 * - Image editing (rotate, crop, resize, flip) / 이미지 편집 (회전, 자르기, 크기 조절, 반전)
 * - Brightness, contrast, saturation, hue adjustments / 밝기, 대비, 채도, 색조 조절
 * - Custom GUI system / 독자 GUI 시스템
 */

#define MP_VERSION "1.0.0"
#define MP_NAME "Many Pictures"

static void print_usage(const char* program_name) {
    mp_fast_printf("%s v%s - Advanced Image Viewer and Editor\n\n", MP_NAME, MP_VERSION);
    mp_fast_printf("Usage: %s [options] [file]\n\n", program_name);
    mp_fast_printf("Options:\n");
    mp_fast_printf("  -h, --help              Show this help message\n");
    mp_fast_printf("  -v, --version           Show version information\n");
    mp_fast_printf("  -g, --grayscale <file>  Convert image to grayscale\n");
    mp_fast_printf("  -c, --colorize <file>   Convert grayscale to color\n");
    mp_fast_printf("  -i, --invert <file>     Invert image colors\n");
    mp_fast_printf("  -ig, --invert-gray <file> Invert and convert to grayscale\n");
    mp_fast_printf("  -r, --rotate <deg> <file> Rotate image (90, 180, 270)\n");
    mp_fast_printf("  -s, --resize <w>x<h> <file> Resize image\n");
    mp_fast_printf("  -o, --output <file>     Output file path\n");
    mp_fast_printf("  --info <file>           Show image information\n");
    mp_fast_printf("  --history <file>        Show image history from EXIF\n");
    mp_fast_printf("\n");
    mp_fast_printf("Supported formats:\n");
    mp_fast_printf("  Images: BMP, PNG, JPEG, GIF, TIFF, WebP, ICO, TGA, PSD\n");
    mp_fast_printf("  Videos: AVI, MP4, MKV, WebM, MOV, FLV\n");
    mp_fast_printf("\n");
    mp_fast_printf("Examples:\n");
    mp_fast_printf("  %s image.jpg                    # Open in GUI\n", program_name);
    mp_fast_printf("  %s -g input.jpg -o output.jpg   # Convert to grayscale\n", program_name);
    mp_fast_printf("  %s -c gray.jpg -o color.jpg     # Colorize grayscale\n", program_name);
    mp_fast_printf("  %s -i input.png -o output.png   # Invert colors\n", program_name);
    mp_fast_printf("  %s --info image.jpg             # Show image info\n", program_name);
}

static void print_version(void) {
    mp_fast_printf("%s v%s\n", MP_NAME, MP_VERSION);
    mp_fast_printf("Pure C implementation with custom codecs\n");
    mp_fast_printf("Copyright (c) 2026\n");
}

static void print_image_info(const char* filepath) {
    mp_image* image = mp_image_load(filepath);
    if (!image) {
        mp_fast_fprintf(2, "Error: Failed to load image '%s'\n", filepath);
        return;
    }
    
    mp_fast_printf("Image Information:\n");
    mp_fast_printf("  File: %s\n", filepath);
    mp_fast_printf("  Format: ");
    
    switch (image->metadata->format) {
        case MP_FORMAT_BMP: mp_fast_printf("BMP\n"); break;
        case MP_FORMAT_PNG: mp_fast_printf("PNG\n"); break;
        case MP_FORMAT_JPEG: mp_fast_printf("JPEG\n"); break;
        case MP_FORMAT_GIF: mp_fast_printf("GIF\n"); break;
        case MP_FORMAT_TIFF: mp_fast_printf("TIFF\n"); break;
        case MP_FORMAT_WEBP: mp_fast_printf("WebP\n"); break;
        default: mp_fast_printf("Unknown\n"); break;
    }
    
    mp_fast_printf("  Dimensions: %ux%u\n", image->metadata->width, image->metadata->height);
    mp_fast_printf("  Bit Depth: %u\n", image->metadata->bit_depth);
    mp_fast_printf("  Color Format: ");
    
    switch (image->metadata->color_format) {
        case MP_COLOR_FORMAT_RGB: mp_fast_printf("RGB\n"); break;
        case MP_COLOR_FORMAT_RGBA: mp_fast_printf("RGBA\n"); break;
        case MP_COLOR_FORMAT_GRAYSCALE: mp_fast_printf("Grayscale\n"); break;
        default: mp_fast_printf("Other\n"); break;
    }
    
    mp_fast_printf("  Has Alpha: %s\n", image->metadata->has_alpha ? "Yes" : "No");
    mp_fast_printf("  Has EXIF: %s\n", image->metadata->has_exif ? "Yes" : "No");
    
    if (image->metadata->has_exif && image->metadata->exif) {
        mp_fast_printf("\nEXIF Data:\n");
        if (image->metadata->exif->make[0]) {
            mp_fast_printf("  Make: %s\n", image->metadata->exif->make);
        }
        if (image->metadata->exif->model[0]) {
            mp_fast_printf("  Model: %s\n", image->metadata->exif->model);
        }
        if (image->metadata->exif->datetime[0]) {
            mp_fast_printf("  DateTime: %s\n", image->metadata->exif->datetime);
        }
    }
    
    mp_image_destroy(image);
}

static mp_result process_command_line(int argc, char** argv) {
    if (argc < 2) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    const char* input_file = NULL;
    const char* output_file = NULL;
    const char* operation = NULL;
    i32 rotate_degrees = 0;
    u32 resize_width = 0, resize_height = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            exit(0);
        } else if (strcmp(argv[i], "--info") == 0) {
            if (i + 1 < argc) {
                print_image_info(argv[++i]);
                exit(0);
            }
        } else if (strcmp(argv[i], "-g") == 0 || strcmp(argv[i], "--grayscale") == 0) {
            operation = "grayscale";
            if (i + 1 < argc) input_file = argv[++i];
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--colorize") == 0) {
            operation = "colorize";
            if (i + 1 < argc) input_file = argv[++i];
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--invert") == 0) {
            operation = "invert";
            if (i + 1 < argc) input_file = argv[++i];
        } else if (strcmp(argv[i], "-ig") == 0 || strcmp(argv[i], "--invert-gray") == 0) {
            operation = "invert-gray";
            if (i + 1 < argc) input_file = argv[++i];
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--rotate") == 0) {
            operation = "rotate";
            if (i + 2 < argc) {
                rotate_degrees = atoi(argv[++i]);
                input_file = argv[++i];
            }
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--resize") == 0) {
            operation = "resize";
            if (i + 2 < argc) {
                sscanf(argv[++i], "%ux%u", &resize_width, &resize_height);
                input_file = argv[++i];
            }
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 < argc) output_file = argv[++i];
        } else {
            input_file = argv[i];
        }
    }
    
    if (!input_file) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    if (!operation) {
        /* No operation specified, just return to open GUI */
        return MP_ERROR_INVALID_PARAM;
    }
    
    /* Load image */
    mp_fast_printf("Loading image: %s\n", input_file);
    mp_image* image = mp_image_load(input_file);
    if (!image) {
        mp_fast_fprintf(2, "Error: Failed to load image '%s'\n", input_file);
        return MP_ERROR_FILE_NOT_FOUND;
    }
    
    /* Apply operation */
    mp_fast_printf("Applying operation: %s\n", operation);
    mp_result result = MP_SUCCESS;
    
    if (strcmp(operation, "grayscale") == 0) {
        result = mp_op_to_grayscale(image);
    } else if (strcmp(operation, "colorize") == 0) {
        result = mp_op_to_color(image);
    } else if (strcmp(operation, "invert") == 0) {
        result = mp_op_invert(image);
    } else if (strcmp(operation, "invert-gray") == 0) {
        result = mp_op_invert_grayscale(image);
    } else if (strcmp(operation, "rotate") == 0) {
        result = mp_op_rotate(image, rotate_degrees);
    } else if (strcmp(operation, "resize") == 0) {
        result = mp_op_resize(image, resize_width, resize_height);
    }
    
    if (result != MP_SUCCESS) {
        mp_fast_fprintf(2, "Error: Operation failed\n");
        mp_image_destroy(image);
        return result;
    }
    
    /* Save image */
    if (!output_file) {
        output_file = "output.png";
    }
    
    mp_fast_printf("Saving image: %s\n", output_file);
    mp_image_format format = mp_image_detect_format(output_file);
    if (format == MP_FORMAT_UNKNOWN) {
        format = MP_FORMAT_PNG;
    }
    
    result = mp_image_save(image, output_file, format);
    if (result != MP_SUCCESS) {
        mp_fast_fprintf(2, "Error: Failed to save image\n");
        mp_image_destroy(image);
        return result;
    }
    
    mp_fast_printf("Done!\n");
    mp_image_destroy(image);
    
    return MP_SUCCESS;
}

int main(int argc, char** argv) {
    mp_fast_printf("%s v%s / %s v%s\n", MP_NAME, MP_VERSION, MP_NAME, MP_VERSION);
    mp_fast_printf("Initializing... / 초기화 중...\n\n");
    
    /* Initialize memory system / 메모리 시스템 초기화 */
    mp_memory_init();
    
    /* Try command-line processing first */
    mp_result result = process_command_line(argc, argv);
    
    if (result == MP_ERROR_INVALID_PARAM && argc >= 2) {
        /* Open GUI with file */
        mp_fast_printf("Starting GUI mode...\n");
        
        if (mp_gui_init() != MP_SUCCESS) {
            mp_fast_fprintf(2, "Error: Failed to initialize GUI\n");
            mp_memory_shutdown();
            return 1;
        }
        
        mp_application* app = mp_app_create();
        if (!app) {
            mp_fast_fprintf(2, "Error: Failed to create application\n");
            mp_gui_shutdown();
            mp_memory_shutdown();
            return 1;
        }
        
        /* Load initial file if provided */
        if (argc >= 2) {
            mp_app_load_image(app, argv[1]);
        }
        
        /* Run application */
        mp_app_run(app);
        
        /* Cleanup */
        mp_app_destroy(app);
        mp_gui_shutdown();
    } else if (result == MP_ERROR_INVALID_PARAM) {
        /* No arguments, show usage */
        print_usage(argv[0]);
    }
    
    /* Shutdown memory system */
    mp_memory_shutdown();
    
    return 0;
}
