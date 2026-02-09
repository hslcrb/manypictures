#include "gui.h"
#include "../core/memory.h"
#include "../core/image.h"
#include "../core/fast_io.h"
#include "../operations/color_ops.h"
#include "../operations/edit_ops.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

static Display* g_display = NULL;
static int g_screen = 0;

mp_result mp_gui_init(void) {
    g_display = XOpenDisplay(NULL);
    if (!g_display) {
        mp_fast_fprintf(2, "Failed to open X display / X 디스플레이 열기 실패\n");
        return MP_ERROR_IO;
    }
    g_screen = DefaultScreen(g_display);
    mp_fast_printf("GUI initialized (X11 Connection Established) / GUI 초기화됨 (X11 연결 수립)\n");
    return MP_SUCCESS;
}

void mp_gui_shutdown(void) {
    if (g_display) {
        XCloseDisplay(g_display);
        g_display = NULL;
    }
    mp_fast_printf("GUI shutdown completed / GUI 종료 완료\n");
}

mp_window* mp_window_create(const char* title, u32 width, u32 height) {
    if (!g_display) return NULL;
    
    mp_window* window = (mp_window*)mp_calloc(1, sizeof(mp_window));
    if (!window) return NULL;
    
    window->base.type = MP_WIDGET_WINDOW;
    window->base.width = width;
    window->base.height = height;
    window->base.enabled = MP_TRUE;
    window->title = mp_strdup(title);
    
    /* Create X11 Window / X11 창 생성 */
    window->x_window = XCreateSimpleWindow(g_display, RootWindow(g_display, g_screen), 
                                          0, 0, width, height, 1,
                                          BlackPixel(g_display, g_screen), 
                                          WhitePixel(g_display, g_screen));
    
    XStoreName(g_display, window->x_window, title);
    XSelectInput(g_display, window->x_window, ExposureMask | KeyPressMask | StructureNotifyMask);
    
    /* Create Cairo Surface / Cairo 서피스 생성 */
    window->cairo_surface = cairo_xlib_surface_create(g_display, window->x_window,
                                                     DefaultVisual(g_display, g_screen),
                                                     width, height);
    window->cairo_context = cairo_create((cairo_surface_t*)window->cairo_surface);
    
    return window;
}

void mp_window_destroy(mp_window* window) {
    if (!window) return;
    
    if (window->cairo_context) cairo_destroy((cairo_t*)window->cairo_context);
    if (window->cairo_surface) cairo_surface_destroy((cairo_surface_t*)window->cairo_surface);
    if (window->x_window) XDestroyWindow(g_display, window->x_window);
    if (window->title) mp_free(window->title);
    mp_free(window);
}

void mp_window_show(mp_window* window) {
    if (window && g_display) {
        XMapWindow(g_display, window->x_window);
        XFlush(g_display);
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

static void mp_gui_draw_sidebar(cairo_t* cr, int h) {
    /* Glassmorphism Sidebar / 글래스모피즘 사이드바 */
    cairo_set_source_rgba(cr, 1, 1, 1, 0.1);
    cairo_rectangle(cr, 0, 0, 200, h);
    cairo_fill(cr);
    
    cairo_set_source_rgb(cr, 0.8, 0.8, 1.0);
    cairo_set_font_size(cr, 18);
    cairo_move_to(cr, 20, 40);
    cairo_show_text(cr, "Operations");
    
    /* Subtle separators / 미묘한 구분선 */
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 20, 50);
    cairo_line_to(cr, 180, 50);
    cairo_stroke(cr);
    
    /* Buttons (Visual only for now) / 버튼 (현재는 시각적 요소만) */
    const char* buttons[] = {"Grayscale", "Colorize", "Invert", "Flip H", "Flip V"};
    for (int i = 0; i < 5; i++) {
        cairo_set_source_rgba(cr, 1, 1, 1, 0.15);
        cairo_rectangle(cr, 20, 70 + i * 50, 160, 40);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_move_to(cr, 40, 95 + i * 50);
        cairo_show_text(cr, buttons[i]);
    }
}

static void mp_gui_draw_image(cairo_t* cr, mp_image* image, int w, int h) {
    if (!image || !image->buffer || !image->buffer->data) return;
    
    /* Cairo expects CAIRO_FORMAT_ARGB32 (B G R A in little endian) */
    /* Our internal format is RGB / 내부 형식은 RGB */
    /* For "Monster" performance, we should ideally use a conversion cache / "괴물급" 성능을 위해 변환 캐시 필요 */
    
    int img_w = image->buffer->width;
    int img_h = image->buffer->height;
    
    cairo_surface_t* img_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, img_w, img_h);
    unsigned char* dest = cairo_image_surface_get_data(img_surface);
    unsigned char* src = image->buffer->data;
    
    /* High performance pixel conversion / 고성능 픽셀 변환 */
    #pragma GCC unroll 4
    for (int i = 0; i < img_w * img_h; i++) {
        dest[i*4 + 0] = src[i*3 + 2]; /* B */
        dest[i*4 + 1] = src[i*3 + 1]; /* G */
        dest[i*4 + 2] = src[i*3 + 0]; /* R */
        dest[i*4 + 3] = 255;          /* A */
    }
    
    cairo_surface_mark_dirty(img_surface);
    
    /* Scale and draw / 크기 조절 및 그리기 */
    double scale_x = (double)(w - 240) / img_w;
    double scale_y = (double)(h - 80) / img_h;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;
    if (scale > 1.0) scale = 1.0;
    
    cairo_save(cr);
    cairo_translate(cr, 220, 60);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, img_surface, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
    
    cairo_surface_destroy(img_surface);
}

static void mp_gui_draw_monster_bg(cairo_t* cr, int w, int h) {
    /* Premium Gradient Background / 프리미엄 그라데이션 배경 */
    cairo_pattern_t* pat = cairo_pattern_create_linear(0, 0, w, h);
    cairo_pattern_add_color_stop_rgb(pat, 0, 0.05, 0.05, 0.1);  /* Deep dark / 짙은 어둠 */
    cairo_pattern_add_color_stop_rgb(pat, 1, 0.15, 0.1, 0.25); /* Elegant purple / 우아한 보라 */
    cairo_set_source(cr, pat);
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
    
    /* Glassmorphism Effect Overlay / 글래스모피즘 효과 오버레이 */
    cairo_set_source_rgba(cr, 1, 1, 1, 0.05); /* Subtle glass / 미묘한 유리 느낌 */
    cairo_rectangle(cr, w * 0.1, h * 0.1, w * 0.8, h * 0.8);
    cairo_fill(cr);
    
    /* Logo or text / 로고 또는 텍스트 */
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_select_font_face(cr, "Inter", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 32);
    cairo_move_to(cr, w * 0.5 - 100, h * 0.5);
    cairo_show_text(cr, "Many Pictures Monster");
}

void mp_gui_run(mp_application* app) {
    if (!g_display || !app) return;
    
    mp_fast_printf("Starting GUI main loop... / GUI 메인 루프 시작 중...\n");
    XEvent ev;
    mp_bool quit = MP_FALSE;
    
    while (!quit) {
        XNextEvent(g_display, &ev);
        
        switch (ev.type) {
            case Expose: {
                XWindowAttributes wa;
                XGetWindowAttributes(g_display, ev.xexpose.window, &wa);
                
                cairo_surface_t* surface = cairo_xlib_surface_create(g_display, ev.xexpose.window, 
                                                                    DefaultVisual(g_display, g_screen), 
                                                                    wa.width, wa.height);
                cairo_t* cr = cairo_create(surface);
                
                mp_gui_draw_monster_bg(cr, wa.width, wa.height);
                mp_gui_draw_sidebar(cr, wa.height);
                
                if (app->current_image) {
                    mp_gui_draw_image(cr, app->current_image, wa.width, wa.height);
                }
                
                cairo_destroy(cr);
                cairo_surface_destroy(surface);
                break;
            }
            case KeyPress: {
                KeySym sym = XLookupKeysym(&ev.xkey, 0);
                if (sym == XK_Escape || sym == XK_q || sym == XK_Q) quit = MP_TRUE;
                break;
            }
            case DestroyNotify:
                quit = MP_TRUE;
                break;
        }
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
    mp_fast_printf("Many Pictures: GUI Application is now active. / Many Pictures: GUI 애플리케이션이 활성화되었습니다.\n");
    
    /* Enter primary event loop */
    mp_gui_run(app);
    
    return MP_SUCCESS;
}

/* CLI-to-GUI Immediate Transition
 * Called from CLI when the user enters a '/' command.
 */
mp_result mp_app_transition_to_gui(mp_application* app) {
    mp_fast_printf("Transition signal received. Reactivating GUI... / 전환 신호 수신. GUI 재활성화 중...\n");
    if (!app->main_window) {
        app->main_window = mp_window_create("Many Pictures", 1024, 768);
    }
    return mp_app_run(app);
}

/* GUI-to-CLI Immediate Transition
 * Called when the user requests a terminal view.
 */
void mp_app_transition_to_cli(mp_application* app) {
    mp_fast_printf("Entering CLI shell mode. Enter 'exit' to return to GUI. / CLI 셸 모드 진입. GUI로 복귀하려면 'exit'를 입력하세요.\n");
    char cmd[256];
    while (1) {
        mp_fast_printf("mp> ");
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
    
    mp_fast_printf("Loading image / 이미지 로딩: %s\n", filepath);
    
    mp_image* image = mp_image_load(filepath);
    if (!image) {
        mp_fast_fprintf(2, "Failed to load image / 이미지 로드 실패: %s\n", filepath);
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
    
    mp_fast_printf("Image loaded / 이미지 로드됨: %ux%u\n", image->buffer->width, image->buffer->height);
    
    return MP_SUCCESS;
}

mp_result mp_app_save_image(mp_application* app, const char* filepath) {
    if (!app || !filepath || !app->current_image) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_fast_printf("Saving image / 이미지 저장 중: %s\n", filepath);
    
    mp_image_format format = mp_image_detect_format(filepath);
    if (format == MP_FORMAT_UNKNOWN) {
        format = MP_FORMAT_PNG;
    }
    
    mp_result result = mp_image_save(app->current_image, filepath, format);
    if (result != MP_SUCCESS) {
        mp_fast_fprintf(2, "Failed to save image / 이미지 저장 실패: %s\n", filepath);
        return result;
    }
    
    mp_fast_printf("Image saved successfully / 이미지 저장 완료\n");
    
    return MP_SUCCESS;
}

mp_result mp_app_apply_operation(mp_application* app, mp_operation_type op_type) {
    if (!app || !app->current_image) {
        return MP_ERROR_INVALID_PARAM;
    }
    
    mp_result result = MP_SUCCESS;
    
    switch (op_type) {
        case MP_OP_GRAYSCALE:
            mp_fast_printf("Applying grayscale conversion... / 그레이스케일 변환 적용 중...\n");
            result = mp_op_to_grayscale(app->current_image);
            break;
            
        case MP_OP_COLORIZE:
            mp_fast_printf("Applying colorization... / 컬러화 적용 중...\n");
            result = mp_op_to_color(app->current_image);
            break;
            
        case MP_OP_INVERT:
            mp_fast_printf("Applying color inversion... / 색상 반전 적용 중...\n");
            result = mp_op_invert(app->current_image);
            break;
            
        case MP_OP_INVERT_GRAYSCALE:
            mp_fast_printf("Applying invert + grayscale... / 반전 및 그레이스케일 적용 중...\n");
            result = mp_op_invert_grayscale(app->current_image);
            break;
            
        case MP_OP_FLIP_H:
            mp_fast_printf("Flipping horizontally... / 좌우 반전 중...\n");
            result = mp_op_flip_horizontal(app->current_image);
            break;
            
        case MP_OP_FLIP_V:
            mp_fast_printf("Flipping vertically... / 상하 반전 중...\n");
            result = mp_op_flip_vertical(app->current_image);
            break;
            
        default:
            return MP_ERROR_UNSUPPORTED;
    }
    
    if (result == MP_SUCCESS) {
        mp_fast_printf("Operation completed successfully / 작업 완료\n");
    } else {
        mp_fast_fprintf(2, "Operation failed / 작업 실패\n");
    }
    
    return result;
}
