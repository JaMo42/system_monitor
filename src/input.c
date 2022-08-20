#include "input.h"
#include "sm.h"
#include <bits/types/mbstate_t.h>
#include <curses.h>
#include <stdlib.h>
#include <wchar.h>

#define SELECTION_BEGIN() \
  Min (S.cursor, S.selection_start)

#define SELECTION_END() \
  Max (S.cursor, S.selection_start)


static struct Get_Line_State
{
  /**** Arguments ****/

  WINDOW *window;
  int x;
  int y;
  bool move_y_on_resize;
  int width;
  History *history;
  Get_Line_Change_Callback change_callback;
  Get_Line_Finish_Callback finish_callback;

  /**** State ****/

  /** Input value used when no history is provided. */
  Input_String no_history_value;
  /** The currently used input value */
  Input_String *input;
  /* Cursor position */
  int cursor;
  /* Begin of the selection */
  int selection_start;
  /* Whether there is a selection */
  bool selection_active;
  /** Cursor position when Ctrl+A is used as the actual cursor position gets
      moved to mark the slection but the view does not change. */
  int cursor_before;
  /** Range to show for @ref GetLineDraw */
  int view_begin, view_end;
  /** Which history value is currently selected, NULL for the new value */
  History_Entry *history_selected;
  /** Current window size, used to adjust with and Y position when window size
      changes. */
  int window_width, window_height;
} S;

static const More_Marker more_left = MORE_MARKER ("...", A_DIM);
static const More_Marker more_right = MORE_MARKER ("...", A_DIM);


static int
GetEscape ()
{
  const char CTRL_FLAG = '5';
  const char SHIFT_FLAG = '2';
  // Check whether if was just an escape / consume the '[' after it
  nodelay (stdscr, true);
  if (getch () == ERR)
    {
      nodelay (stdscr, false);
      return 27;
    }
  nodelay (stdscr, false);
  char a, b = 0, c = 0;
  switch (a = getch ())
    {
      case 'A': return KEY_UP;
      case 'B': return KEY_DOWN;
      case 'C': return KEY_RIGHT;
      case 'D': return KEY_LEFT;
      case 'F': return KEY_END;
      case 'H': return KEY_HOME;
    }
  if (getch () == ';')
    {
      b = getch ();
      c = getch ();
    }
  switch (a)
    {
      case '1':
        if (b == CTRL_FLAG)
          {
            switch (c)
              {
                case 'A': return KEY_CTRL_UP;
                case 'B': return KEY_CTRL_DOWN;
                case 'C': return KEY_CTRL_RIGHT;
                case 'D': return KEY_CTRL_LEFT;
              }
          }
        else if (b == SHIFT_FLAG)
          {
            switch (c)
              {
                case 'A': return KEY_SHIFT_UP;
                case 'B': return KEY_SHIFT_DOWN;
                case 'C': return KEY_SHIFT_RIGHT;
                case 'D': return KEY_SHIFT_LEFT;
                case 'F': return KEY_SHIFT_END;
                case 'H': return KEY_SHIFT_HOME;
              }
          }
        else if (b == 0)
          // ^[[1~
          return KEY_HOME;
        break;

      case '4':
        if (b == 0)
          // ^[[4~
          return KEY_END;
        break;

      case '3':
        return b ? (b == CTRL_FLAG ? KEY_CTRL_DC : 0) : KEY_DC;

      case '5':
        return KEY_PPAGE;

      case '6':
        return KEY_NPAGE;
    }
  return 0;
}

int
GetChar ()
{
  int input = getch ();
  if (input == 27)
    return GetEscape ();
  else if (
    // These overlap the ctrl+alpha keys
    input == '\n'  // KEY_ENTER
    || input == 8  // KEY_CTRL_BACKSPACE
  )
    return input;
  else if (input >= 1 && input <= 26)
    return KEY_CTRL ('a' - 1 + input);
  else
    return input;
}

static Input_String *
HistoryNewer ()
{
  if (!S.history)
    return &S.no_history_value;
  else if (S.history_selected == NULL)
    return &S.history->new_value;
  if (S.history->most_recent == NULL || S.history_selected->newer == NULL)
    {
      S.history_selected = NULL;
      return &S.history->new_value;
    }
  else
    {
      S.history_selected = S.history_selected->newer;
      memcpy (&S.history->history_value,&S.history_selected->value, sizeof (Input_String));
      return &S.history->history_value;
    }
}

static Input_String *
HistoryOlder ()
{
  if (!S.history)
    return &S.no_history_value;
  else if (S.history->most_recent == NULL)
    return &S.history->new_value;
  else if (S.history_selected == NULL)
    {
      S.history_selected = S.history->most_recent;
      memcpy (&S.history->history_value, &S.history_selected->value, sizeof (Input_String));
      return &S.history->history_value;
    }
  else if (S.history_selected->older == NULL)
    return &S.history->history_value;
  else
    {
      S.history_selected = S.history_selected->older;
      memcpy (&S.history->history_value, &S.history_selected->value, sizeof (Input_String));
      return &S.history->history_value;
    }
}

static inline void
StrPush (Input_String *s, char ch)
{
  if (s->length == LINE_SIZE)
    return;
  s->data[s->length++] = ch;
  s->data[s->length] = '\0';
}

static inline void
StrPop (Input_String *s)
{
  if (s->length > 0)
    s->data[--s->length] = '\0';
}

static inline void
StrClear (Input_String *s)
{
  s->data[s->length = 0] = '\0';
}

static inline void
StrInsert (Input_String *s, int idx, char ch)
{
  if (idx != s->length)
    memmove (s->data + idx + 1, s->data + idx, s->length - idx);
  s->data[idx] = ch;
  ++s->length;
}

static inline void
StrErase (Input_String *s, int begin, int end)
{
  const int n = end - begin;
  if (!n)
    return;
  const int a = s->length - end;
  if (a)
    memmove (s->data + begin, s->data + end, a);
  s->length -= n;
  s->data[s->length] = '\0';
}

static inline void
StrReplace (Input_String *s, int begin, int end, char ch)
{
  s->data[begin] = ch;
  StrErase (s, begin + 1, end);
}

static inline void
GetLineFinish ()
{
  curs_set (false);
  HandleInput = MainHandleInput;
  if (!S.history)
    S.finish_callback (&S.no_history_value);
  else if (S.input->length == 0)
    {
      memcpy (&S.no_history_value, S.input, sizeof (Input_String));
      S.finish_callback (&S.no_history_value);
    }
  else
    {
      /* Don't add if it's the same as the most recent entry. */
      if (S.history->most_recent == NULL
          || memcmp (S.history->most_recent->value.data,
                     S.input->data,
                     S.input->length + 1))
        {
          History_Entry *new_entry = malloc (sizeof (History_Entry));
          new_entry->older = S.history->most_recent;
          new_entry->newer = NULL;
          memcpy (&new_entry->value, S.input, sizeof (Input_String));
          if (S.history->most_recent)
            S.history->most_recent->newer = new_entry;
          S.history->most_recent = new_entry;
        }
      S.finish_callback (&S.history->most_recent->value);
    }
}

int
GetLineDraw ()
{
  int selection_begin = -1, selection_end = -1;
  int real_cursor = (S.cursor_before == -1 ? S.cursor : S.cursor_before);
  int i = 0, c = real_cursor - S.view_begin;
  const char *p = S.input->data + S.view_begin;
  if (S.selection_active)
    {
      selection_begin = SELECTION_BEGIN ();
      selection_end = SELECTION_END ();
    }

  wmove (S.window, S.y, S.x);
  for (i = 0; i < S.width; ++i)
    waddch (S.window, ' ');

  wmove (S.window, S.y, S.x);
  if (S.view_begin != 0)
    {
      wattron (S.window, more_left.attributes);
      waddstr (S.window, more_left.chars);
      wattroff (S.window, more_left.attributes);
      c += more_left.size;
    }
  if (InRange (selection_begin, 0, S.view_begin))
    wattron (S.window, A_REVERSE);
  for (i = S.view_begin; i < S.view_end; ++i)
    {
      if (i == selection_begin)
        wattron (S.window, A_REVERSE);
      else if (i == selection_end)
        wattroff (S.window, A_REVERSE);
      waddch (S.window, *p++);
    }
  if (selection_end >= S.view_end)
    wattroff (S.window, A_REVERSE);
  if (S.view_end != S.input->length)
    {
      wattron (S.window, more_right.attributes);
      waddstr (S.window, more_right.chars);
      wattroff (S.window, more_right.attributes);
    }
  wmove (S.window, S.y, S.x + c);
  return S.input->length;
}

static inline void
AdjustView ()
{
  const int L = S.input->length;
  const bool cursor_at_end = S.cursor == L;
  if (L < S.width + !cursor_at_end)
    {
      S.view_begin = 0;
      S.view_end = L;
    }
  else if (S.cursor < more_left.size + 2)
    {
      S.view_begin = 0;
      S.view_end = S.width - more_right.size;
    }
  else if (S.cursor < S.view_begin + 1)
    {
      S.view_begin = S.cursor - 1;
      S.view_end = Min (S.view_begin - more_left.size + S.width, L);
      if (S.view_end != L)
        S.view_end -= more_right.size;
    }
  else if (S.cursor >= L - (more_right.size + 2))
    {
      S.view_end = L;
      S.view_begin = S.view_end - S.width + more_left.size + cursor_at_end;
    }
  else if (S.cursor > S.view_end - 2)
    {
      S.view_end = S.cursor + 2;
      S.view_begin = Max (S.view_end + more_right.size - S.width, 0);
      if (S.view_begin != 0)
        S.view_begin += more_left.size;
    }
}

static inline void
MoveCursor (int by)
{
  if (S.cursor_before != -1)
    {
      S.cursor = S.cursor_before;
      S.cursor_before = -1;
    }
  S.cursor += by;
  S.cursor = Clamp (S.cursor, 0, S.input->length);
  AdjustView ();
}

static inline void
SetCursor (int to)
{
  S.cursor_before = -1;
  S.cursor = Min (to, S.input->length);
  AdjustView ();
}

static inline void
DeleteSelection ()
{
  const int begin = SELECTION_BEGIN ();
  const int end = SELECTION_END ();
  StrErase (S.input, begin, end);
  S.selection_active = false;
  SetCursor (begin);
}

static inline int
JumpLeft ()
{
  if (S.cursor == 0)
    return 0;
  int cursor = S.cursor - 1;
  const char *p = S.input->data + cursor;
  if (isalnum (*p--))
    {
      for (; cursor > 0; --cursor, --p)
        {
          if (!isalnum (*p))
            break;
        }
    }
  else
    {
      for (; cursor > 0; --cursor, --p)
        {
          if (isalnum (*p))
            break;
        }
    }
  return cursor;
}

static inline int
JumpRight ()
{
  const int L = S.input->length;
  int cursor = S.cursor;
  const char *p = S.input->data + cursor;
  if (isalnum (*p))
    {
      for (; cursor < L; ++cursor, ++p)
        {
          if (!isalnum (*p))
            break;
        }
    }
  else
    {
      for (; cursor < L; ++cursor, ++p)
        {
          if (isalnum (*p))
            break;
        }
    }
  return cursor;
}

static bool
GetLineHandleInput (int key)
{
  bool did_change = true, text_changed = false, keep_selection = false;
  int a, b;
  switch (key)
    {
      case KEY_ENTER:
        GetLineFinish ();
        return true;

      case KEY_ESCAPE:
        StrClear (S.input);
        GetLineFinish ();
        return true;

      case KEY_BACKSPACE:
        if (S.selection_active)
          {
            DeleteSelection ();
            text_changed = true;
          }
        else if (S.cursor == S.input->length)
          {
            text_changed = S.input->length > 0;
            StrPop (S.input);
            MoveCursor (-1);
          }
        else if (S.cursor > 0)
          {
            StrErase (S.input, S.cursor - 1, S.cursor);
            text_changed = true;
            MoveCursor (-1);
          }
        break;

      case KEY_DC:
        if (S.selection_active)
          {
            DeleteSelection ();
            text_changed = true;
          }
        else if (S.cursor + 1 == S.input->length)
          {
            StrPop (S.input);
            AdjustView ();
            text_changed = true;
          }
        else if (S.cursor < S.input->length)
          {
            StrErase (S.input, S.cursor, S.cursor+1);
            AdjustView ();
            text_changed = true;
          }
        break;

      case KEY_CTRL_BACKSPACE:
        if (S.selection_active)
          {
            DeleteSelection ();
            text_changed = true;
          }
        else if (S.cursor > 0)
          {
            a = JumpLeft ();
            StrErase (S.input, a, S.cursor);
            SetCursor (a);
            text_changed = true;
          }
        break;

      case KEY_CTRL_DC:
        if (S.selection_active)
          {
            DeleteSelection ();
            text_changed = true;
          }
        else if (S.cursor < S.input->length)
          {
            StrErase (S.input, S.cursor, JumpRight ());
            text_changed = true;
          }
        break;

      case KEY_LEFT:
        MoveCursor (-1);
        break;

      case KEY_RIGHT:
        MoveCursor (1);
        break;

      case KEY_CTRL_LEFT:
        SetCursor (JumpLeft ());
        break;

      case KEY_CTRL_RIGHT:
        SetCursor (JumpRight ());
        break;

      case KEY_HOME:
        SetCursor (0);
        break;

      case KEY_END:
        SetCursor (S.input->length);
        break;

      case KEY_SHIFT_LEFT:
      case KEY_SHIFT_RIGHT:
        if (!S.selection_active)
          {
            S.selection_active = true;
            S.selection_start = S.cursor;
          }
        MoveCursor (key == KEY_SHIFT_LEFT ? -1 : 1);
        keep_selection = S.selection_start != S.cursor;
        break;

      case KEY_SHIFT_HOME:
      case KEY_SHIFT_END:
        if (!S.selection_active)
          {
            S.selection_active = true;
            S.selection_start = S.cursor;
          }
        SetCursor (key == KEY_SHIFT_HOME ? 0 : S.input->length);
        keep_selection = S.selection_active != S.cursor;
        break;

      case KEY_CTRL ('a'):
        if (S.input->length != 0)
          {
            S.selection_active = true;
            S.selection_start = S.input->length;
            S.cursor_before = S.cursor;
            S.cursor = 0;
            keep_selection = true;
          }
        break;

      case KEY_UP:
      case KEY_DOWN:
        S.input = key == KEY_UP ? HistoryOlder () : HistoryNewer ();
        keep_selection = false;
        SetCursor (S.input->length);
        text_changed = true;
        break;

      case KEY_RESIZE:
        a = getmaxx (S.window) - S.window_width;
        S.width += a;
        S.window_width += a;
        if (S.move_y_on_resize)
          {
            b = getmaxy (S.window) - S.window_height;
            S.y += b;
            S.window_height += b;
          }
        break;

      default:
        if (key && ((key < 0x80 && isprint (key)) || key < 0x100))
          {
            if (S.selection_active)
              {
                a = SELECTION_BEGIN ();
                b = SELECTION_END ();
                StrReplace (S.input, a, b, key);
                SetCursor (a + 1);
              }
            else
              {
                StrInsert (S.input, S.cursor, key);
                MoveCursor (1);
              }
            text_changed = true;
          }
        else
          did_change =  false;
    }
  S.selection_active = keep_selection;
  if (did_change)
    S.change_callback (S.input, text_changed);
  return true;
}

void
GetLineBegin (WINDOW *win, int x, int y, bool move_y_on_resize, int width,
              History *history, Get_Line_Change_Callback change_callback,
              Get_Line_Finish_Callback finish_callback)
{
  Input_String *input;
  if (history)
    {
      memset (&history->history_value, 0, sizeof (Input_String));
      memset (&history->new_value, 0, sizeof (Input_String));
      input = &history->new_value;
    }
  else
    {
      memset (&S.no_history_value, 0, sizeof (Input_String));
      input = &S.no_history_value;
    }
  S = (struct Get_Line_State) {
    .window = win,
    .x = x,
    .y = y,
    .move_y_on_resize = move_y_on_resize,
    .width = width,
    .history = history,
    .change_callback = change_callback,
    .finish_callback = finish_callback,

    .input = input,
    .cursor = 0,
    .selection_start = 0,
    .selection_active = false,
    .cursor_before = -1,
    .view_begin = 0,
    .view_end = 0,
    .history_selected = NULL,
    .window_width = getmaxx (win),
    .window_height = getmaxy (win)
  };

  HandleInput = GetLineHandleInput;
  curs_set (true);
}

History *
NewHistory ()
{
  History *history = malloc (sizeof (History));
  history->most_recent = NULL;
  return history;
}

static inline void
DeleteHistoryImpl (History_Entry *entry)
{
  if (entry->older)
    DeleteHistoryImpl (entry->older);
  free (entry);
}

void
DeleteHistory (History *self)
{
  if (self->most_recent)
    DeleteHistoryImpl (self->most_recent);
  free (self);
}
