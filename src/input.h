#pragma once
#include "stdafx.h"

#define LINE_SIZE 256

typedef struct Widget Widget;
typedef struct Layout Layout;

enum {
  MOUSE_WHEEL_UP = 0x10000,
  MOUSE_WHEEL_DOWN = 0x200000
};

enum
{
  // Custom key codes
  KEY__CTRL_A = 0x1000000,
#define KEY_CTRL(x) (0x1000000 + (int)(x) - 'a')
  KEY__CTRL_Z = 0x1000000 + 'z' - 'a',
  KEY_CTRL_UP,
  KEY_CTRL_DOWN,
  KEY_CTRL_RIGHT,
  KEY_CTRL_LEFT,
  KEY_SHIFT_UP,
  KEY_SHIFT_DOWN,
  KEY_SHIFT_RIGHT,
  KEY_SHIFT_LEFT,
  KEY_SHIFT_HOME,
  KEY_SHIFT_END,
  KEY_CTRL_DC,

  // Utility/redfinitions
#undef KEY_BACKSPACE
  KEY_BACKSPACE = 127,  // May depend on terminal?
  KEY_ESCAPE = 27,
#undef KEY_ENTER
  KEY_ENTER = '\n',
  KEY_CTRL_BACKSPACE = 8  // Same as Ctrl+H?
};

typedef struct Input_String Input_String;
struct Input_String {
  char data[LINE_SIZE+1];
  int length;
};

typedef struct History_Entry History_Entry;
struct History_Entry
{
  History_Entry *older;
  History_Entry *newer;
  Input_String value;
};

typedef struct History History;
struct History
{
  History_Entry *most_recent;
  Input_String history_value;
  Input_String new_value;
};

typedef struct More_Marker More_Marker;
struct More_Marker
{
  const char *chars;
  int size;
  unsigned attributes;
};

#define MORE_MARKER(chars_, attrs_) \
  ((More_Marker) {                  \
    .chars = (chars_),              \
    .size = sizeof (chars_) - 1,    \
    .attributes = (attrs_)          \
  })

typedef struct Mouse_Event Mouse_Event;
struct Mouse_Event
{
  Widget *widget;
  mmask_t button;
  int x, y;
};

/** checks if the string is null or empty. */
static inline bool StrEmpty (Input_String *s)
{
  return s == NULL || s->length == 0;
}

/**
 * @param string: the current string.
 * @param text_changed: whether the content of the string has changed.
 */
typedef void (*Get_Line_Change_Callback) (Input_String *string,
                                          bool text_changed);

/**
 * @param string_or_null: the final string.
 * If the `history` parameter to @ref GetLineBegin was NULL the returned value
 * is invalidated by the next call to @ref GetLineBegin.
 * If a history was used and the length of the string is greater than 0 the
 * returned string points to the value inside the history and is valid until
 * @ref DeleteHistory is called on the history.
 */
typedef void (*Get_Line_Finish_Callback) (Input_String *string);

/**
 * reads a single character handling escape sequences.
 *
 * Only some escape sequences are handled (see `KEY_` enumerations), unhandled
 * ones result in 0 being returned.
 */
int GetChar ();

/**
 * @param win: window to use for resizing and drawing
 * @param x, y: coordinate to draw at
 * @param move_y_on_resize: whether to adjust Y position when the window size
 *                          changes
 * @param width: available width for drawing
 * @param history: history to use or NULL
 * @param change_callback: function to call when text changes or should be
 *                         redrawn
 * @param finish_callback: function to call with the final string
 */
void GetLineBegin (WINDOW *win, int x, int y, bool move_y_on_resize, int width,
                   History *history, Get_Line_Change_Callback change_callback,
                   Get_Line_Finish_Callback finish_callback);

/**
 * draws the current get-line text.
 *
 * The window, position and width are set by the respective parameters to
 * @ref GetLineBegin.
 *
 * @return number of characters written
 */
int GetLineDraw ();

History *NewHistory ();

void DeleteHistory (History *self);

void ResolveMouseEvent (MEVENT *event, Layout *ui, Mouse_Event *out);

