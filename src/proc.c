#include "proc.h"
#include "util.h"

Widget proc_widget = WIDGET(Proc);

void ProcInit (WINDOW *win, unsigned graph_scale) {
  (void)graph_scale;
  DrawWindow (win, "Processes");
  DrawWindowInfo (win, "? - ? of ?");
}

void ProcQuit () {}

void ProcUpdate () {}

void ProcDraw (WINDOW *win) {
  (void)win;
}

void ProcResize (WINDOW *win) {
  DrawWindow (win, "Processes");
  DrawWindowInfo (win, "? - ? of ?");
}

