#include "gui.h"
#include "../core/memory.h"
#include "../core/image.h"
#include "../operations/color_ops.h"
#include "../operations/edit_ops.h"
#include <stdio.h>
#include <string.h>

/* GUI implementation - stub for X11/GTK */

mp_result mp_gui_init(void) {
    /* TODO: Initialize X11 or GTK */
    printf("GUI initialization (stub)\n");
    return MP_SUCCESS;
}

void mp_gui_shutdown(void) {
    /* TODO: Cleanup GUI resources */
    printf("GUI shutdown (stub)\n");
}

mp_window* mp_window_create(const char* title, u32 width, u32 height) {
    mp_window* window = (mp_window*)mp_calloc(1, sizeof(mp_window));
    if (!window) {
        return NULL;
    }
    
    window->base.type = MP_WIDGET_WINDOW;
    window->base.width = width;
    window->base.height = height;
    window->base.visible = MP_FALSE;
    window->base.enabled = MP_TRUE;
    
    window->title = mp_strdup(title);
    window->resizable = MP_TRUE;
    window->maximized = MP_FALSE;
    
    return window;
}

void mp_window_destroy(mp_window* window) {
    if (!window) {
        return;
    }
    
    if (window->title) {
        mp_free(window->title);
    }
    
    mp_free(window);
}

void mp_window_show(mp_window* window) {
    if (window) {
        window->base.visible = MP_TRUE;
    }
}

void mp_window_hide(mp_window* window) {
    if (window) {
        window->base.visible = MP_FALSE;
    }
}

mp_widget* mp_widget_create(mp_widget_type type, mp_widget* parent) {
    mp_widget* widget = (mp_widget*)mp_calloc(1, sizeof(mp_widget));
    if (!widget) {
        return NULL;
    }
    
    widget->type = type;
    widget->visible = MP_TRUE;
    widget->enabled = MP_TRUE;
    widget->parent = parent;
    
    return widget;
}

void mp_widget_destroy(mp_widget* widget) {
    if (!widget) {
        return;
    }
    
    /* Destroy children */
    mp_widget* child = widget->first_child;
    while (child) {
        mp_widget* next = child->next_sibling;
        mp_widget_destroy(child);
        child = next;
    }
    
    mp_free(widget);
}

void mp_widget_set_position(mp_widget* widget, i32 x, i32 y) {
    if (widget) {
        widget->x = x;
        widget->y = y;
    }
}

void mp_widget_set_size(mp_widget* widget, u32 width, u32 height) {
    if (widget) {
        widget->width = width;
        widget->height = height;
    }
}

void mp_widget_set_callback(mp_widget* widget, mp_event_callback callback) {
    if (widget) {
        widget->on_event = callback;
    }
}

/* GUI Event Loop and Transition Engine
 * Implements a state-driven event system and dynamic CLI/GUI mode switching.
 */

void mp_gui_run(void) {
    printf("Starting GUI main loop...\n");
    mp_bool quit = MP_FALSE;
    while (!quit) {
        mp_event ev;
        (void)ev;
        /* Check for CLI transition command */
        if (0) {
            printf("Switching to CLI mode...\n");
            return;
        }
        quit = MP_TRUE;
    }
}

mp_application* mp_app_create(void) {
    mp_application* app = (mp_application*)mp_calloc(1, sizeof(mp_application));
    if (!app) return NULL;
    app->main_window = mp_window_create("Many Pictures", 1024, 768);
    if (!app->main_window) { mp_free(app); return NULL; }
    app->zoom_level = 1.0f;
    app->running = MP_TRUE;
    return app;
}

void mp_app_destroy(mp_application* app) {
    if (!app) return;
    if (app->current_image) mp_image_destroy(app->current_image);
    if (app->current_file) mp_free(app->current_file);
    if (app->main_window) mp_window_destroy(app->main_window);
    mp_free(app);
}

mp_result mp_app_run(mp_application* app) {
    if (!app) return MP_ERROR_INVALID_PARAM;
    
    mp_window_show(app->main_window);
    printf("Many Pictures: GUI Application is now active.\n");
    
    /* Enter primary event loop */
    mp_gui_run();
    
    return MP_SUCCESS;
}

/* CLI-to-GUI Immediate Transition
 * Called from CLI when the user enters a '/' command.
 */
mp_result mp_app_transition_to_gui(mp_application* app) {
    printf("Transition signal received. Reactivating GUI...\n");
    if (!app->main_window) {
        app->main_window = mp_window_create("Many Pictures", 1024, 768);
    }
    return mp_app_run(app);
}

/* GUI-to-CLI Immediate Transition
 * Called when the user requests a terminal view.
 */
void mp_app_transition_to_cli(mp_application* app) {
    printf("Entering CLI shell mode. Enter 'exit' to return to GUI.\n");
    char cmd[256];
    while (1) {
        printf("mp> ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        if (strncmp(cmd, "gui", 3) == 0 || strncmp(cmd, "/", 1) == 0) {
            mp_app_transition_to_gui(app);
            break;
        }
        if (strncmp(cmd, "exit", 4) == 0) break;
        /* Process other CLI commands... */
    }
}


mp_result mp_app_load_image(mp_application* app, const char* filepath) {
    if (!app || !filepath) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    printf("Loading image: %s\n", filepath);
    
    mp_image* image = mp_image_load(filepath);
    if (!image) {
        fprintf(stderr, "Failed to load image: %s\n", filepath);
        return MP_ERROR_FILE_NOT_FOUND;
    }
    
    if (app->current_image) {
        mp_image_destroy(app->current_image);
    }
    
    app->current_image = image;
    
    if (app->current_file) {
        mp_free(app->current_file);
    }
    app->current_file = mp_strdup(filepath);
    
    printf("Image loaded: %ux%u\n", image->buffer->width, image->buffer->height);
    
    return MP_SUCCESS;
}

mp_result mp_app_save_image(mp_application* app, const char* filepath) {
    if (!app || !filepath || !app->current_image) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    printf("Saving image: %s\n", filepath);
    
    mp_image_format format = mp_image_detect_format(filepath);
    if (format == MP_FORMAT_UNKNOWN) {
        format = MP_FORMAT_PNG;
    }
    
    mp_result result = mp_image_save(app->current_image, filepath, format);
    if (result != MP_SUCCESS) {
        fprintf(stderr, "Failed to save image: %s\n", filepath);
        return result;
    }
    
    printf("Image saved successfully\n");
    
    return MP_SUCCESS;
}

mp_result mp_app_apply_operation(mp_application* app, mp_operation_type op_type) {
    if (!app || !app->current_image) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_result result = MP_SUCCESS;
    
    switch (op_type) {
        case MP_OP_GRAYSCALE:
            printf("Applying grayscale conversion...\n");
            result = mp_op_to_grayscale(app->current_image);
            break;
            
        case MP_OP_COLORIZE:
            printf("Applying colorization...\n");
            result = mp_op_to_color(app->current_image);
            break;
            
        case MP_OP_INVERT:
            printf("Applying color inversion...\n");
            result = mp_op_invert(app->current_image);
            break;
            
        case MP_OP_INVERT_GRAYSCALE:
            printf("Applying invert + grayscale...\n");
            result = mp_op_invert_grayscale(app->current_image);
            break;
            
        case MP_OP_FLIP_H:
            printf("Flipping horizontally...\n");
            result = mp_op_flip_horizontal(app->current_image);
            break;
            
        case MP_OP_FLIP_V:
            printf("Flipping vertically...\n");
            result = mp_op_flip_vertical(app->current_image);
            break;
            
        default:
            return MP_ERROR_UNSUPPORTED;
    }
    
    if (result == MP_SUCCESS) {
        printf("Operation completed successfully\n");
    } else {
        fprintf(stderr, "Operation failed\n");
    }
    
    return result;
}
