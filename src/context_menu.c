#include "context_menu.h"
#include "util.h"
#include "sm.h"

typedef struct {
  Context_Menu *menu;
  int sel, prev_sel;
  WINDOW *win;
} Active_Context_Menu;

Context_Menu
ContextMenuCreate (const char **names, void (**functions) ())
{
  int len;
  Context_Menu menu;
  menu.names = names;
  menu.functions = functions;
  menu.count = 0;
  menu.width = 0;
  while (*names)
    {
      len = strlen (*names);
      if (len > menu.width)
        menu.width = len;
      ++menu.count;
      ++names;
    }
  return menu;
}

static void
ContextMenuDraw (void *data)
{
  Active_Context_Menu *state = (Active_Context_Menu *)data;
  WINDOW *win = state->win;
  int i, count;
  for (i = 0, count = state->menu->count; i < count; ++i)
    {
      wmove (win, 1 + i, 1);
      if (i == state->sel)
        wattron (win, A_REVERSE);
      waddstr (win, state->menu->names[i]);
      if (i == state->sel)
        wattroff (win, A_REVERSE);
    }
  redrawwin (win);
  wrefresh (win);
  state->prev_sel = state->sel;
}

void
ContextMenuShow (Context_Menu *menu, int x, int y)
{
  if (y + menu->count + 2 > LINES)
    y = LINES - 1 - menu->count;
  if (x + menu->width + 2 > COLS)
    x = COLS - 1 - menu->count;

  WINDOW *win = newwin (menu->count + 2, menu->width + 2, y - 1, x - 1);
  Active_Context_Menu state = {
    .menu = menu,
    .sel = 0,
    .prev_sel = -2,
    .win = win
  };
  const int maxx = x + menu->width -1, maxy = y + menu->count - 1;

  wattron (win, COLOR_PAIR (C_BORDER));
  Border (win);
  wattroff (win, COLOR_PAIR (C_BORDER));

  pthread_mutex_lock (&draw_mutex);
  ContextMenuDraw (&state);
  refresh ();
  overlay_data = &state;
  DrawOverlay = ContextMenuDraw;
  pthread_mutex_unlock (&draw_mutex);

  ReportMouseMoveEvents (true, false);
  int key;
  MEVENT mouse;
  bool running = true, quit_key_was_resize = false, inside;
  while (running)
    {
      if (state.sel != state.prev_sel)
        {
          state.prev_sel = state.sel;
          ContextMenuDraw (&state);
        }
      switch (key = GetChar ())
        {
        case 'k':
        case KEY_UP:
          if (state.sel)
            --state.sel;
          break;

        case 'j':
        case KEY_DOWN:
          if (state.sel + 1 < menu->count)
            ++state.sel;
          break;

        case ' ':
        case '\n':
          running = false;
          break;

        case KEY_MOUSE:
          if (getmouse (&mouse) == ERR)
            break;
          inside = !(mouse.x < x || mouse.x > maxx || mouse.y < y || mouse.y > maxy);
          if (mouse.bstate == REPORT_MOUSE_POSITION)
            {
              if (inside)
                state.sel = mouse.y - y;
            }
          else if (mouse.bstate == BUTTON1_CLICKED)
            {
              if (inside)
                state.sel = mouse.y - y;
              else
                state.sel = -1;
              running = false;
            }
          break;

        case KEY_RESIZE:
          quit_key_was_resize = true;
          /* FALLTHROUGH */
        default:
          running = false;
          state.sel = -1;
          break;
        }
    }
  ReportMouseMoveEvents (false, false);

  pthread_mutex_lock (&draw_mutex);
  DrawOverlay = NULL;
  overlay_data = NULL;
  pthread_mutex_unlock (&draw_mutex);

  delwin (win);
  flushinp ();

  if (quit_key_was_resize)
    ungetch (KEY_RESIZE);
  else
    ungetch (KEY_REFRESH);

  if (state.sel >= 0)
    menu->functions[state.sel] ();
}
