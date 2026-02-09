#ifndef MANYPICTURES_GUI_H
#define MANYPICTURES_GUI_H

#include "../core/types.h"

/* GUI system for Many Pictures */

/* Forward declarations */
typedef struct mp_window mp_window;
typedef struct mp_widget mp_widget;

/* Widget types */
typedef enum {
    MP_WIDGET_WINDOW,
    MP_WIDGET_BUTTON,
    MP_WIDGET_LABEL,
    MP_WIDGET_IMAGE_VIEW,
    MP_WIDGET_MENU,
    MP_WIDGET_MENU_ITEM,
    MP_WIDGET_TOOLBAR,
    MP_WIDGET_STATUSBAR,
    MP_WIDGET_SCROLLBAR,
    MP_WIDGET_PANEL
} mp_widget_type;

/* Language modes / 언어 모드 */
typedef enum {
    MP_LANG_EN_KR, /* English & Korean (Default) / 영문 & 국문 (기본) */
    MP_LANG_EN,    /* English Only / 영문 전용 */
    MP_LANG_KR     /* Korean Only / 국문 전용 */
} mp_language_mode;

/* Event types */
typedef enum {
    MP_EVENT_NONE,
    MP_EVENT_MOUSE_DOWN,
    MP_EVENT_MOUSE_UP,
    MP_EVENT_MOUSE_MOVE,
    MP_EVENT_KEY_DOWN,
    MP_EVENT_KEY_UP,
    MP_EVENT_PAINT,
    MP_EVENT_RESIZE,
    MP_EVENT_CLOSE
} mp_event_type;

/* Mouse buttons */
typedef enum {
    MP_MOUSE_LEFT = 1,
    MP_MOUSE_MIDDLE = 2,
    MP_MOUSE_RIGHT = 3
} mp_mouse_button;

/* Event structure */
typedef struct {
    mp_event_type type;
    i32 x, y;
    u32 button;
    u32 key_code;
    u32 modifiers;
    void* user_data;
} mp_event;

/* Event callback */
typedef void (*mp_event_callback)(mp_widget* widget, mp_event* event);

/* Widget structure */
struct mp_widget {
    mp_widget_type type;
    i32 x, y;
    u32 width, height;
    mp_bool visible;
    mp_bool enabled;
    void* native_handle;
    mp_event_callback on_event;
    void* user_data;
    mp_widget* parent;
    mp_widget* first_child;
    mp_widget* next_sibling;
};

/* Window structure */
struct mp_window {
    mp_widget base;
    char* title;
    mp_bool resizable;
    mp_bool maximized;
    
    /* Native handles / 네이티브 핸들 */
    void* x_display;
    unsigned long x_window;
    void* cairo_surface;
    void* cairo_context;
};

/* Application structure */
typedef struct {
    mp_window* main_window;
    mp_image* current_image;
    mp_widget* image_view;
    mp_widget* toolbar;
    mp_widget* statusbar;
    char* current_file;
    mp_bool running;
    f32 zoom_level;
    i32 scroll_x, scroll_y;
    mp_language_mode language_mode; /* Current language mode / 현재 언어 모드 */
} mp_application;

/* Initialize GUI system */
mp_result mp_gui_init(void);

/* Shutdown GUI system */
void mp_gui_shutdown(void);

/* Create window */
mp_window* mp_window_create(const char* title, u32 width, u32 height);

/* Destroy window */
void mp_window_destroy(mp_window* window);

/* Show window */
void mp_window_show(mp_window* window);

/* Hide window */
void mp_window_hide(mp_window* window);

/* Create widget */
mp_widget* mp_widget_create(mp_widget_type type, mp_widget* parent);

/* Destroy widget */
void mp_widget_destroy(mp_widget* widget);

/* Set widget position */
void mp_widget_set_position(mp_widget* widget, i32 x, i32 y);

/* Set widget size */
void mp_widget_set_size(mp_widget* widget, u32 width, u32 height);

/* Set widget event callback */
void mp_widget_set_callback(mp_widget* widget, mp_event_callback callback);

/* Run main event loop / 메인 이벤트 루프 실행 */
void mp_gui_run(mp_application* app);

/* Process events */
mp_bool mp_gui_process_events(void);

/* Create application */
mp_application* mp_app_create(void);

/* Destroy application */
void mp_app_destroy(mp_application* app);

/* Run application */
mp_result mp_app_run(mp_application* app);

/* Load image in application */
mp_result mp_app_load_image(mp_application* app, const char* filepath);

/* Save image in application */
mp_result mp_app_save_image(mp_application* app, const char* filepath);

/* Apply operation to current image */
mp_result mp_app_apply_operation(mp_application* app, mp_operation_type op_type);

#endif /* MANYPICTURES_GUI_H */
