#include "core/types.h"
#include "core/memory.h"
#include "core/image.h"
#include "operations/color_ops.h"
#include "operations/edit_ops.h"
#include "gui/gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Many Pictures - Advanced Image Viewer and Editor
 * 
 * Features:
 * - Support for BMP, PNG, JPEG, GIF, TIFF, WebP formats
 * - Video support (AVI, MP4, MKV, WebM, MOV, FLV)
 * - Color to grayscale conversion
 * - Grayscale to color conversion (AI-based)
 * - Color inversion
 * - Combined invert + grayscale operation
 * - EXIF metadata with custom history tracking
 * - Git-like history restoration from EXIF
 * - Image editing (rotate, crop, resize, flip)
 * - Brightness, contrast, saturation, hue adjustments
 * - Custom GUI system
 */

#define MP_VERSION "1.0.0"
#define MP_NAME "Many Pictures"

static void print_usage(const char* program_name) {
    printf("%s v%s - Advanced Image Viewer and Editor\n\n", MP_NAME, MP_VERSION);
    printf("Usage: %s [options] [file]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help              Show this help message\n");
    printf("  -v, --version           Show version information\n");
    printf("  -g, --grayscale <file>  Convert image to grayscale\n");
    printf("  -c, --colorize <file>   Convert grayscale to color\n");
    printf("  -i, --invert <file>     Invert image colors\n");
    printf("  -ig, --invert-gray <file> Invert and convert to grayscale\n");
    printf("  -r, --rotate <deg> <file> Rotate image (90, 180, 270)\n");
    printf("  -s, --resize <w>x<h> <file> Resize image\n");
    printf("  -o, --output <file>     Output file path\n");
    printf("  --info <file>           Show image information\n");
    printf("  --history <file>        Show image history from EXIF\n");
    printf("\n");
    printf("Supported formats:\n");
    printf("  Images: BMP, PNG, JPEG, GIF, TIFF, WebP, ICO, TGA, PSD\n");
    printf("  Videos: AVI, MP4, MKV, WebM, MOV, FLV\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s image.jpg                    # Open in GUI\n", program_name);
    printf("  %s -g input.jpg -o output.jpg   # Convert to grayscale\n", program_name);
    printf("  %s -c gray.jpg -o color.jpg     # Colorize grayscale\n", program_name);
    printf("  %s -i input.png -o output.png   # Invert colors\n", program_name);
    printf("  %s --info image.jpg             # Show image info\n", program_name);
}

static void print_version(void) {
    printf("%s v%s\n", MP_NAME, MP_VERSION);
    printf("Pure C implementation with custom codecs\n");
    printf("Copyright (c) 2026\n");
}

static void print_image_info(const char* filepath) {
    mp_image* image = mp_image_load(filepath);
    if (!image) {
        fprintf(stderr, "Error: Failed to load image '%s'\n", filepath);
        return;
    }
    
    printf("Image Information:\n");
    printf("  File: %s\n", filepath);
    printf("  Format: ");
    
    switch (image->metadata->format) {
        case MP_FORMAT_BMP: printf("BMP\n"); break;
        case MP_FORMAT_PNG: printf("PNG\n"); break;
        case MP_FORMAT_JPEG: printf("JPEG\n"); break;
        case MP_FORMAT_GIF: printf("GIF\n"); break;
        case MP_FORMAT_TIFF: printf("TIFF\n"); break;
        case MP_FORMAT_WEBP: printf("WebP\n"); break;
        default: printf("Unknown\n"); break;
    }
    
    printf("  Dimensions: %ux%u\n", image->metadata->width, image->metadata->height);
    printf("  Bit Depth: %u\n", image->metadata->bit_depth);
    printf("  Color Format: ");
    
    switch (image->metadata->color_format) {
        case MP_COLOR_FORMAT_RGB: printf("RGB\n"); break;
        case MP_COLOR_FORMAT_RGBA: printf("RGBA\n"); break;
        case MP_COLOR_FORMAT_GRAYSCALE: printf("Grayscale\n"); break;
        default: printf("Other\n"); break;
    }
    
    printf("  Has Alpha: %s\n", image->metadata->has_alpha ? "Yes" : "No");
    printf("  Has EXIF: %s\n", image->metadata->has_exif ? "Yes" : "No");
    
    if (image->metadata->has_exif && image->metadata->exif) {
        printf("\nEXIF Data:\n");
        if (image->metadata->exif->make[0]) {
            printf("  Make: %s\n", image->metadata->exif->make);
        }
        if (image->metadata->exif->model[0]) {
            printf("  Model: %s\n", image->metadata->exif->model);
        }
        if (image->metadata->exif->datetime[0]) {
            printf("  DateTime: %s\n", image->metadata->exif->datetime);
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
    printf("Loading image: %s\n", input_file);
    mp_image* image = mp_image_load(input_file);
    if (!image) {
        fprintf(stderr, "Error: Failed to load image '%s'\n", input_file);
        return MP_ERROR_FILE_NOT_FOUND;
    }
    
    /* Apply operation */
    printf("Applying operation: %s\n", operation);
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
        fprintf(stderr, "Error: Operation failed\n");
        mp_image_destroy(image);
        return result;
    }
    
    /* Save image */
    if (!output_file) {
        output_file = "output.png";
    }
    
    printf("Saving image: %s\n", output_file);
    mp_image_format format = mp_image_detect_format(output_file);
    if (format == MP_FORMAT_UNKNOWN) {
        format = MP_FORMAT_PNG;
    }
    
    result = mp_image_save(image, output_file, format);
    if (result != MP_SUCCESS) {
        fprintf(stderr, "Error: Failed to save image\n");
        mp_image_destroy(image);
        return result;
    }
    
    printf("Done!\n");
    mp_image_destroy(image);
    
    return MP_SUCCESS;
}

int main(int argc, char** argv) {
    printf("%s v%s\n", MP_NAME, MP_VERSION);
    printf("Initializing...\n\n");
    
    /* Initialize memory system */
    mp_memory_init();
    
    /* Try command-line processing first */
    mp_result result = process_command_line(argc, argv);
    
    if (result == MP_ERROR_INVALID_PARAM && argc >= 2) {
        /* Open GUI with file */
        printf("Starting GUI mode...\n");
        
        if (mp_gui_init() != MP_SUCCESS) {
            fprintf(stderr, "Error: Failed to initialize GUI\n");
            mp_memory_shutdown();
            return 1;
        }
        
        mp_application* app = mp_app_create();
        if (!app) {
            fprintf(stderr, "Error: Failed to create application\n");
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
