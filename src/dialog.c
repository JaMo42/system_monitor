#include "dialog.h"
#include "input.h"
#include "sm.h"
#include "util.h"

typedef struct {
    int lines, cols;
} Dimensions;

static Dimensions
WrappedTextDimensions(const char *text, int max_width) {
    int lines = 1, cols = 0;
    int i, l;
    for (i = 0, l = 0; text[i]; ++i, ++l) {
        if (text[i] == '\n' || l > max_width) {
            ++lines;
            l = 0;
        } else if (l > cols) {
            cols = l;
        }
    }
    ++cols;
    return (Dimensions){.lines = lines, .cols = cols};
}

static void
PrintLine(WINDOW *win, const char **str, int n) {
    int i;
    char c;
    for (i = 0; i < n; ++i) {
        c = (*str)[i];
        if (c == '\0' || c == '\n') {
            break;
        }
        waddch(win, (*str)[i]);
    }
    *str += i + ((*str)[i] == '\n');
}

static void
PlainMessageBoxDraw(void *win) {
    redrawwin((WINDOW *)win);
    wrefresh((WINDOW *)win);
}

void
ShowPlainMessageBox(const char *title, const char *message) {
    const Dimensions dim
        = WrappedTextDimensions(message, (int)(0.9 * COLS) - 2);
    const int width = Max(dim.cols, title ? (int)strlen(title) + 8 : 3);
    const int height = dim.lines;
    int line;
    bool quit_key_was_resize;
    if (height + 4 > LINES) {
        return;
    }
    WINDOW *win = newwin(
        height + 2,
        width + 2,
        (LINES - (height + 2)) / 2,
        (COLS - (width + 2)) / 2
    );
    if (title) {
        DrawWindow(win, title);
    } else {
        wattron(win, COLOR_PAIR(theme->border));
        Border(win);
        wattroff(win, COLOR_PAIR(theme->border));
    }
    for (line = 1; *message; ++line) {
        wmove(win, line, 1);
        PrintLine(win, &message, width);
    }

    pthread_mutex_lock(&draw_mutex);
    PlainMessageBoxDraw(win);
    refresh();
    overlay_data = win;
    DrawOverlay = PlainMessageBoxDraw;
    pthread_mutex_unlock(&draw_mutex);

    flushinp();
    quit_key_was_resize = GetChar() == KEY_RESIZE;

    pthread_mutex_lock(&draw_mutex);
    DrawOverlay = NULL;
    overlay_data = NULL;
    pthread_mutex_unlock(&draw_mutex);

    delwin(win);
    flushinp();

    if (quit_key_was_resize) {
        ungetch(KEY_RESIZE);
    } else {
        ungetch(KEY_REFRESH);
    }
}

typedef struct {
    WINDOW *win;
    const char *str;
    bool secret;
} InputBoxState;

static void
InputBoxDraw(void *pstate) {
    InputBoxState *state = pstate;
    redrawwin(state->win);
    wrefresh(state->win);
}

static void
InputBoxChange(Input_String *_string, bool text_changed) {
    (void)_string;
    if (!text_changed) {
        return;
    }
    // InputBoxState *state = overlay_data;
    GetLineDraw();
    InputBoxDraw(overlay_data);
}

static void
InputBoxFinish(Input_String *string) {
    InputBoxState *state = overlay_data;
    state->str = string->data;
}

const char *
ShowInputBox(const char *title, const char *label, bool secret) {
    const Dimensions dim = WrappedTextDimensions(label, (int)(0.9 * COLS) - 2);
    int width = Max(Max(dim.cols, title ? (int)strlen(title) + 8 : 3), 20);
    const int height = dim.lines + 3;
    int entryx = 2;
    const int entryy = 1 + dim.lines + 1;

    WINDOW *win = newwin(
        height + 2,
        width + 2,
        (LINES - (height + 2)) / 2,
        (COLS - (width + 2)) / 2
    );

    InputBoxState state = {
        .win = win,
        .str = NULL,
        .secret = secret,
    };

    if (title) {
        DrawWindow(win, title);
    } else {
        wattron(win, COLOR_PAIR(theme->border));
        Border(win);
        wattroff(win, COLOR_PAIR(theme->border));
    }
    for (int line = 1; *label; ++line) {
        wmove(win, line, 1);
        PrintLine(win, &label, width);
    }
    DrawRect(win, entryx - 1, entryy - 1, width, 3);
    wmove(win, entryy, entryx);
    if (secret) {
        waddstr(win, "🔐");
        ++entryx;
        --width;
    }

    pthread_mutex_lock(&draw_mutex);
    InputBoxDraw((void *)&state);
    refresh();
    overlay_data = &state;
    DrawOverlay = InputBoxDraw;
    pthread_mutex_unlock(&draw_mutex);

    GetLineBegin(
        win,
        entryx + 1,
        entryy,
        false,
        width - 3,
        NULL,
        InputBoxChange,
        InputBoxFinish,
        true
    );

    int ch;
    while (state.str == NULL) {
        ch = GetChar();
        if (ch == KEY_RESIZE) {
            HandleResize();
        }
        HandleInput(ch);
    }

    pthread_mutex_lock(&draw_mutex);
    DrawOverlay = NULL;
    overlay_data = NULL;
    pthread_mutex_unlock(&draw_mutex);
    delwin(win);
    ungetch(KEY_REFRESH);

    return state.str;
}
