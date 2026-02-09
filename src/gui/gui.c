#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "gui.h"
#include "../core/memory.h"
#include "../core/image.h"
#include "../core/fast_io.h"
#include "../operations/color_ops.h"
#include "../operations/edit_ops.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>



#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

/* Forward declarations for Double Buffering / 더블 버퍼링을 위한 전방 선언 */
static void mp_gui_request_repaint(mp_application* app);
static void mp_gui_render_to_backbuffer(mp_application* app, int w, int h);
static void mp_gui_draw_monster_bg(cairo_t* cr, int w, int h);
void mp_image_record_history(mp_image* img, mp_operation_type op_type, const char* description);


static Display* g_display = NULL;
static int g_screen = 0;
static char g_system_font[256] = "Sans"; /* Default fallback / 기본 대체 폰트 */

/* System File Dialog Helper / 시스템 파일 대화 상자 헬퍼 */
/* System File Dialog Helper / 시스템 파일 대화 상자 헬퍼 */
/* Non-blocking version using fork/exec/pipe / fork/exec/pipe를 사용하는 비차단 버전 */
static void mp_gui_open_file_dialog_system(mp_application* app) {
    if (app->dialog_pid != -1) {
        mp_fast_printf("[GUI] Dialog already open. / 대화 상자가 이미 열려 있습니다.\n");
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid == 0) {
        /* Child Process */
        close(pipefd[0]); /* Close read end */
        dup2(pipefd[1], STDOUT_FILENO); /* Redirect stdout to pipe */
        close(pipefd[1]);
        
        /* Close stderr or redirect to /dev/null to avoid noise */
        int nullfd = open("/dev/null", O_WRONLY);
        dup2(nullfd, STDERR_FILENO);
        close(nullfd);

        /* Try Zenity */
        execlp("zenity", "zenity", "--file-selection", "--title=Select Image / 이미지 선택", 
               "--file-filter=Images | *.jpg *.jpeg *.png *.bmp *.gif *.tiff *.webp", NULL);
        
        /* If Zenity fails, try KDialog */
        execlp("kdialog", "kdialog", "--getopenfilename", ".", 
               "Image Files (*.jpg *.jpeg *.png *.bmp *.gif *.tiff *.webp)", NULL);

        _exit(127); /* Command not found */
    } else {
        /* Parent Process */
        close(pipefd[1]); /* Close write end */
        app->dialog_fd = pipefd[0];
        app->dialog_pid = pid;
        
        /* Set pipe to non-blocking */
        int flags = fcntl(app->dialog_fd, F_GETFL, 0);
        fcntl(app->dialog_fd, F_SETFL, flags | O_NONBLOCK);
        
        mp_fast_printf("[GUI] Launching system file picker (async)... / [GUI] 시스템 파일 선택기 실행 중(비동기)...\n");
    }
}

static void mp_gui_check_dialog_result(mp_application* app) {
    if (app->dialog_fd == -1) return;

    char buffer[1024];
    ssize_t bytes = read(app->dialog_fd, buffer, sizeof(buffer) - 1);

    if (bytes > 0) {
        buffer[bytes] = '\0';
        /* Remove newline */
        char* newline = strrchr(buffer, '\n');
        if (newline) *newline = '\0';
        
        if (strlen(buffer) > 0) {
            mp_app_load_image(app, buffer);
            /* Force redraw using the new flicker-free method / 새로운 깜빡임 방지 방식을 사용하여 강제 다시 그리기 */
            mp_gui_request_repaint(app);
        }
    }

    if (bytes != -1 || (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        /* EOF or Error (pipe closed by child exit) */
        
        /* Wait for child to prevent zombie */
        int status;
        waitpid(app->dialog_pid, &status, WNOHANG);
        
        close(app->dialog_fd);
        app->dialog_fd = -1;
        app->dialog_pid = -1;
        // mp_fast_printf("[GUI] Dialog closed.\n");
    }
}

/* Dynamic Font Detection / 동적 폰트 감지 */
static void mp_get_system_font_name(void) {
    FILE* fp = popen("fc-match -f \"%{family}\" :lang=ko", "r");
    if (fp) {
        if (fgets(g_system_font, sizeof(g_system_font), fp)) {
            /* Remove newline / 개행 제거 */
            size_t len = strlen(g_system_font);
            if (len > 0 && g_system_font[len-1] == '\n') {
                g_system_font[len-1] = '\0';
            }
            /* If multiple families returned (comma separated), take first / 여러 패밀리가 반환된 경우 첫 번째 사용 */
            char* comma = strchr(g_system_font, ',');
            if (comma) *comma = '\0';
            
            mp_fast_printf("[GUI] Detected System Font: %s\n", g_system_font);
        }
        pclose(fp);
    } else {
        strcpy(g_system_font, "Sans"); /* "Sans" usually maps to a valid CJK font on Linux */
        mp_fast_printf("[GUI] Font detection failed. Using fallback: Sans\n");
    }
}

mp_result mp_gui_init(void) {
    g_display = XOpenDisplay(NULL);
    if (!g_display) {
        mp_fast_fprintf(2, "Failed to open X display / X 디스플레이 열기 실패\n");
        return MP_ERROR_IO;
    }
    g_screen = DefaultScreen(g_display);
    
    mp_get_system_font_name(); /* Detect font / 폰트 감지 */
    
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
    
    /* Create Back Buffer / 백 버퍼 생성 */
    window->back_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    window->back_context = cairo_create((cairo_surface_t*)window->back_surface);
    
    /* Disable Background Clearing to prevent flickering / 깜빡임 방지를 위해 배경 지우기 비활성화 */
    XSetWindowAttributes swa;
    swa.background_pixmap = None;
    XChangeWindowAttributes(g_display, window->x_window, CWBackPixmap, &swa);
    
    return window;
}

void mp_window_destroy(mp_window* window) {
    if (!window) return;
    
    if (window->back_context) cairo_destroy((cairo_t*)window->back_context);
    if (window->back_surface) cairo_surface_destroy((cairo_surface_t*)window->back_surface);
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


/* Button Definitions / 버튼 정의 */
typedef struct {
    const char* label_en;
    const char* label_kr;
    int y_offset;
    mp_operation_type op_type; /* or custom ID */
} mp_gui_button;

#define MP_BTN_OPEN_FILE  100
#define MP_BTN_LANG_TOGGLE 101

static const mp_gui_button g_buttons[] = {
    {"Open File", "파일 열기", 0, MP_BTN_OPEN_FILE},
    {"Language", "한/영", 50, MP_BTN_LANG_TOGGLE},
    {"Grayscale", "흑백 변환", 120, MP_OP_GRAYSCALE},
    {"Colorize", "컬러화", 170, MP_OP_COLORIZE},
    {"Invert", "색상 반전", 220, MP_OP_INVERT},
    {"Flip H", "좌우 반전", 270, MP_OP_FLIP_H},
    {"Flip V", "상하 반전", 320, MP_OP_FLIP_V}
};

static void mp_gui_draw_sidebar(cairo_t* cr, int h, mp_language_mode mode) {
    /* Glassmorphism Sidebar / 글래스모피즘 사이드바 */
    cairo_set_source_rgba(cr, 1, 1, 1, 0.1);
    cairo_rectangle(cr, 0, 0, 200, h);
    cairo_fill(cr);
    
    /* System Branding / 시스템 브랜딩 */
    cairo_set_source_rgb(cr, 0.4, 0.7, 1.0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 14);
    cairo_move_to(cr, 20, 40);
    cairo_show_text(cr, "CHRONOS-EXIF");
    cairo_move_to(cr, 20, 55);
    cairo_set_font_size(cr, 10);
    cairo_show_text(cr, "ARTIFACT ENGINE v2.2");
    
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_set_font_size(cr, 18);
    cairo_move_to(cr, 20, 40);
    
    /* Title Rendering / 타이틀 렌더링 */
    if (mode == MP_LANG_KR) {
        cairo_select_font_face(cr, g_system_font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_show_text(cr, "기능");
        cairo_select_font_face(cr, "Inter", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD); /* Reset */
    } else {
        cairo_show_text(cr, "Operations");
    }
    
    /* Subtle separators / 미묘한 구분선 */
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 20, 50);
    cairo_line_to(cr, 180, 50);
    cairo_stroke(cr);
    
    /* Draw Buttons / 버튼 그리기 */
    for (int i = 0; i < 7; i++) {
        int y = 70 + g_buttons[i].y_offset;
        
        /* Button Background / 버튼 배경 */
        cairo_set_source_rgba(cr, 1, 1, 1, 0.15);
        cairo_rectangle(cr, 20, y, 160, 40);
        cairo_fill(cr);
        
        /* Button Text / 버튼 텍스트 */
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_move_to(cr, 40, y + 25);
        
        if (mode == MP_LANG_EN) {
            cairo_select_font_face(cr, "Inter", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
            cairo_show_text(cr, g_buttons[i].label_en);
        } else if (mode == MP_LANG_KR) {
            cairo_select_font_face(cr, g_system_font, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD); /* Use detected font */
            cairo_show_text(cr, g_buttons[i].label_kr);
            cairo_select_font_face(cr, "Inter", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD); /* Reset */
        } else {
            /* Bilingual Mode / 이중 언어 모드 */
            /* Display EN (space) KR or just EN depending on space. Let's try EN for now to avoid crowding, user can toggle */
             cairo_show_text(cr, g_buttons[i].label_en);
        }
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

static void mp_gui_render_to_backbuffer(mp_application* app, int w, int h) {
    if (!app || !app->main_window || !app->main_window->back_context) return;
    cairo_t* cr = (cairo_t*)app->main_window->back_context;
    
    /* Reset state / 상태 초기화 */
    cairo_identity_matrix(cr);
    cairo_new_path(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    /* Draw to off-screen buffer / 오프스크린 버퍼에 그리기 */
    mp_gui_draw_monster_bg(cr, w, h);
    mp_gui_draw_sidebar(cr, h, app->language_mode);
    if (app->current_image) {
        mp_gui_draw_image(cr, app->current_image, w, h);
    }
}

static void mp_gui_request_repaint(mp_application* app) {
    if (!app || !app->main_window) return;
    XWindowAttributes wa;
    XGetWindowAttributes(g_display, app->main_window->x_window, &wa);
    mp_gui_render_to_backbuffer(app, wa.width, wa.height);
    
    /* Flush Cairo to backbuffer surface / Cairo를 백버퍼 서피스로 플러시 */
    cairo_surface_flush((cairo_surface_t*)app->main_window->back_surface);
    
    /* Trigger Expose without clearing (background is None) / 지우지 않고 Expose 트리거 */
    XClearArea(g_display, app->main_window->x_window, 0, 0, 0, 0, True);
    XFlush(g_display);
}

void mp_gui_run(mp_application* app) {
    if (!g_display || !app) return;
    
    mp_fast_printf("Starting GUI main loop... / GUI 메인 루프 시작 중...\n");
    XEvent ev;
    mp_bool quit = MP_FALSE;
    
    /* Allow mouse and resize events / 마우스 및 크기 조정 이벤트 허용 */
    XSelectInput(g_display, app->main_window->x_window, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask | SubstructureNotifyMask);

    /* Initial render */
    XWindowAttributes wa;
    XGetWindowAttributes(g_display, app->main_window->x_window, &wa);
    mp_gui_render_to_backbuffer(app, wa.width, wa.height);

    /* Init blocking state */
    app->dialog_fd = -1;
    app->dialog_pid = -1;

    while (!quit) {
        while (XPending(g_display) > 0) {
            XNextEvent(g_display, &ev);
        
        switch (ev.type) {
            case ConfigureNotify: {
                int nw = ev.xconfigure.width;
                int nh = ev.xconfigure.height;
                /* Resize main Cairo surface and backbuffer / 메인 Cairo 서피스 및 백버퍼 크기 조정 */
                if (app->main_window->cairo_surface) {
                    cairo_xlib_surface_set_size((cairo_surface_t*)app->main_window->cairo_surface, nw, nh);
                }
                if (app->main_window->back_surface) {
                    cairo_destroy((cairo_t*)app->main_window->back_context);
                    cairo_surface_destroy((cairo_surface_t*)app->main_window->back_surface);
                    app->main_window->back_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, nw, nh);
                    app->main_window->back_context = cairo_create((cairo_surface_t*)app->main_window->back_surface);
                    mp_gui_render_to_backbuffer(app, nw, nh);
                }
                break;
            }
            case Expose: {
                if (ev.xexpose.count > 0) break;
                
                if (app->main_window && app->main_window->cairo_context && app->main_window->back_surface) {
                    cairo_t* cr = (cairo_t*)app->main_window->cairo_context;
                    cairo_set_source_surface(cr, (cairo_surface_t*)app->main_window->back_surface, 0, 0);
                    cairo_paint(cr);
                }
                break;
            }
            case ButtonPress: {
                if (ev.xbutton.button == 1) { /* Left Click */
                    int x = ev.xbutton.x;
                    int y = ev.xbutton.y;
                    
                    /* Sidebar Hit Testing / 사이드바 히트 테스팅 */
                    if (x >= 20 && x <= 180) {
                        for (int i = 0; i < 7; i++) {
                            int btn_y = 70 + g_buttons[i].y_offset;
                            if (y >= btn_y && y <= btn_y + 40) {
                                /* Clicked button i */
                                if (g_buttons[i].op_type == MP_BTN_OPEN_FILE) {
                                    /* Invoke System File Dialog / 시스템 파일 대화 상자 호출 */
                                    mp_gui_open_file_dialog_system(app);
                                } else if (g_buttons[i].op_type == MP_BTN_LANG_TOGGLE) {
                                    app->language_mode = (app->language_mode + 1) % 3;
                                    mp_fast_printf("[GUI] Language switched to mode %d\n", app->language_mode);
                                    mp_gui_request_repaint(app);
                                } else {
                                    mp_app_apply_operation(app, g_buttons[i].op_type);
                                    mp_gui_request_repaint(app);
                                }
                            }
                        }
                    }
                }
                break;
            }
            case KeyPress: {
                XKeyEvent* xkey = &ev.xkey;
                KeySym sym = XLookupKeysym(xkey, 0);
                mp_bool ctrl = (xkey->state & ControlMask) != 0;
                mp_bool shift = (xkey->state & ShiftMask) != 0;

                if (ctrl) {
                    if (sym == XK_z || sym == XK_Z) {
                        if (shift) mp_app_redo(app);
                        else mp_app_undo(app);
                        mp_gui_request_repaint(app);
                    } else if (sym == XK_y || sym == XK_Y) {
                        mp_app_redo(app);
                        mp_gui_request_repaint(app);
                    } else if (sym == XK_o || sym == XK_O) {
                        mp_gui_open_file_dialog_system(app);
                    }
                } else {
                    if (sym == XK_Escape || sym == XK_q || sym == XK_Q) quit = MP_TRUE;
                }
                break;
            }
            case DestroyNotify:
                quit = MP_TRUE;
                break;
        }
        }
        
        mp_gui_check_dialog_result(app);
        usleep(10000); /* 10ms poll */
    }
}

mp_application* mp_app_create(void) {
    mp_application* app = (mp_application*)mp_calloc(1, sizeof(mp_application));
    if (!app) return NULL;
    app->main_window = mp_window_create("Many Pictures", 1024, 768);
    if (!app->main_window) { mp_free(app); return NULL; }
    app->zoom_level = 1.0f;
    app->language_mode = MP_LANG_EN_KR; /* Default to Bilingual */
    app->running = MP_TRUE;
    
    /* Undo/Redo Init */
    app->max_undo = 20;
    app->undo_stack = (mp_image_buffer**)mp_calloc(app->max_undo, sizeof(mp_image_buffer*));
    app->redo_stack = (mp_image_buffer**)mp_calloc(app->max_undo, sizeof(mp_image_buffer*));
    app->undo_count = 0;
    app->redo_count = 0;
    
    return app;
}

static void mp_app_clear_redo(mp_application* app) {
    for (int i = 0; i < app->redo_count; i++) {
        mp_image_buffer_destroy(app->redo_stack[i]);
    }
    app->redo_count = 0;
}

static void mp_app_push_undo(mp_application* app) {
    if (!app || !app->current_image) return;
    
    /* If full, shift */
    if (app->undo_count == app->max_undo) {
        mp_image_buffer_destroy(app->undo_stack[0]);
        for (int i = 0; i < app->max_undo - 1; i++) {
            app->undo_stack[i] = app->undo_stack[i+1];
        }
        app->undo_count--;
    }
    
    app->undo_stack[app->undo_count++] = mp_image_buffer_clone(app->current_image->buffer);
    mp_app_clear_redo(app);
}

void mp_app_undo(mp_application* app) {
    if (!app || !app->current_image || app->undo_count == 0) return;
    
    /* Push current to redo */
    if (app->redo_count < app->max_undo) {
        app->redo_stack[app->redo_count++] = app->current_image->buffer;
    } else {
        mp_image_buffer_destroy(app->current_image->buffer);
    }
    
    /* Restore from undo */
    app->current_image->buffer = app->undo_stack[--app->undo_count];
    mp_image_record_history(app->current_image, MP_OP_UNDO, "Chronos-EXIF Undo");
    mp_fast_printf("[GUI] Undo performed / 실행 취소됨 (%d left)\n", app->undo_count);
}

void mp_app_redo(mp_application* app) {
    if (!app || !app->current_image || app->redo_count == 0) return;
    
    /* Push current to undo */
    if (app->undo_count < app->max_undo) {
        app->undo_stack[app->undo_count++] = app->current_image->buffer;
    } else {
        mp_image_buffer_destroy(app->current_image->buffer);
    }
    
    /* Restore from redo */
    app->current_image->buffer = app->redo_stack[--app->redo_count];
    mp_image_record_history(app->current_image, MP_OP_REDO, "Chronos-EXIF Redo");
    mp_fast_printf("[GUI] Redo performed / 다시 실행됨 (%d left)\n", app->redo_count);
}

void mp_app_destroy(mp_application* app) {
    if (!app) return;
    if (app->current_image) mp_image_destroy(app->current_image);
    if (app->current_file) mp_free(app->current_file);
    if (app->main_window) mp_window_destroy(app->main_window);
    
    for (int i = 0; i < app->undo_count; i++) mp_image_buffer_destroy(app->undo_stack[i]);
    for (int i = 0; i < app->redo_count; i++) mp_image_buffer_destroy(app->redo_stack[i]);
    mp_free(app->undo_stack);
    mp_free(app->redo_stack);

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
    
    /* Clear stacks when loading new image / 새로운 이미지 로드 시 스택 초기화 */
    for (int i = 0; i < app->undo_count; i++) mp_image_buffer_destroy(app->undo_stack[i]);
    for (int i = 0; i < app->redo_count; i++) mp_image_buffer_destroy(app->redo_stack[i]);
    app->undo_count = 0;
    app->redo_count = 0;

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
    
    mp_image_record_history(image, MP_OP_LOAD, "Loaded Image Artifact");
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
    
    if (op_type != MP_BTN_LANG_TOGGLE && op_type != MP_BTN_OPEN_FILE) {
        mp_app_push_undo(app);
    }
    
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
