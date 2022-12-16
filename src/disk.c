#include "disk.h"
#include "util.h"
#include "canvas/canvas.h"
#include "ui.h"
#include "sm.h"

static const double disk_radius = 9.0;
static const int disk_width = 14;
static const int disk_height = 7;

IgnoreInput (Disk);
IgnoreMouse (Disk);
Widget disk_widget = WIDGET ("disk_usage", Disk);

bool disk_vertical = false;
const char *disk_fs = NULL;

typedef struct
{
  const char *path;
  uintmax_t total;
  uintmax_t avail;
  uintmax_t used;
  bool changed;
  bool ok;
} Disk_FS_Info;

static Canvas *disk_canvas;
static List *disk_filesystems;

static int disk_margin;
static int disk_padding;

/* Parses the filesystems string.
   Note: the given string will be modified if it contains more than 1 path,
         possibly changing the value of the SM_DISK_FS environment variable
         (only for this program) */
static void
DiskParseFsString (char *s)
{
  Disk_FS_Info *fs_info;
  char *end;
  disk_filesystems = list_create ();
  do
    {
      if ((end = strchr (s, ',')))
        *end = '\0';
      fs_info = calloc (1, sizeof (Disk_FS_Info));
      fs_info->path = s;
      list_push_back (disk_filesystems)->p = fs_info;
      s = end + 1;
    }
  while (end);
}

static inline void
DiskForceUpdate ()
{
  Disk_FS_Info *fs_info;
  if (disk_filesystems)
    {
      list_for_each (disk_filesystems, it)
        {
          fs_info = it->p;
          fs_info->total = -1;
          fs_info->avail = -1;
          fs_info->used = -1;
        }
    }
}

void
DiskInit (WINDOW *win, unsigned graph_scale)
{
  (void)graph_scale;
  DiskDrawBorder (win);
  disk_canvas = CanvasCreateSized (ceil (disk_radius) + 1,
                                   ceil (disk_radius / 2));
  if (!(disk_fs || (disk_fs = getenv ("SM_DISK_FS")))
      || !*disk_fs)
    disk_fs = "/";
  DiskParseFsString ((char *)disk_fs);
  if (getenv ("SM_DISK_VERTICAL") != NULL) {
    disk_vertical = true;
  }
  WidgetFixedSize (&disk_widget, true);
  DiskUpdate ();
  DiskDraw (win);
  DiskForceUpdate ();
}

void
DiskQuit ()
{
  CanvasDelete (disk_canvas);
}

void
DiskUpdate ()
{
  struct statvfs buffer;
  uintmax_t total, free, avail, used;
  Disk_FS_Info *fs_info, before;
  bool ok;
  list_for_each (disk_filesystems, it)
    {
      fs_info = it->p;
      before = *fs_info;
      if (statvfs (fs_info->path, &buffer) == 0)
        {
          total = buffer.f_blocks * buffer.f_bsize;
          free = buffer.f_bfree * buffer.f_bsize;
          avail = buffer.f_bavail * buffer.f_bsize;
          ok = true;
        }
      else
        total = free = avail = ok = 0;
      used = total - free;
      fs_info->total = total;
      fs_info->avail = avail;
      fs_info->used = used;
      fs_info->changed = (fs_info->total != before.total
                          || fs_info->avail != before.avail
                          || fs_info->used != before.used);
      fs_info->ok = ok;
    }
}

static inline void
DiskDrawArc (double x, double y, double inner_radius, double outer_radius,
             double begin_angle, double end_angle, short color)
{
  for (double angle = begin_angle; angle < end_angle; angle += 0.02)
    CanvasDrawLine (
      disk_canvas,
      cos (angle) * inner_radius + x,
      sin (angle) * inner_radius + y,
      cos (angle) * outer_radius + x,
      sin (angle) * outer_radius + y,
      color
    );
}

static char *
DiskFormatSize (char *buf, uintmax_t size)
{
  Size_Format fmt = GetSizeFormat (size);
  if (fmt.size < 10.0 && fmt.size != 0.0)
    buf += sprintf (buf, "%.1f", fmt.size);
  else
    buf += sprintf (buf, "%"PRIuMAX, (uintmax_t)ceil (fmt.size));
  buf = StringPush (buf, fmt.unit);
  return buf;
}

void
DiskDrawFSInfo (WINDOW *win, Disk_FS_Info *fs_info, int x, int y)
{
  static char print_buf[/*disk_width+1*/15];
  const double used_percent = ((fs_info->used | fs_info->avail)
                               ? ((double)fs_info->used
                                  / (double)(fs_info->used + fs_info->avail))
                               : 0.0);
  const double outer_radius = disk_radius;
  const double inner_radius = outer_radius * 0.7;
  const double cx = outer_radius;
  const double cy = outer_radius;
  const double start = -PI / 2.0;
  const double fill = start + 2*PI * used_percent;
  const double end = start + 2*PI;
  int len;
  char *p;

  len = strlen (fs_info->path);
  if (!fs_info->ok)
    wattron (win, COLOR_PAIR (C_DISK_ERROR));
  if (len > disk_width)
    {
      wmove (win, y, x);
      waddstr (win, "...");
      waddstr (win, fs_info->path + len - disk_width + 3);
    }
  else
    {
      wmove (win, y, x + ((disk_width - len) / 2));
      waddstr (win, fs_info->path);
    }
  if (!fs_info->ok)
    wattroff (win, COLOR_PAIR (C_DISK_ERROR));

  CanvasClear (disk_canvas);
  DiskDrawArc (cx, cy, inner_radius, outer_radius, fill, end, C_DISK_FREE);
  DiskDrawArc (cx, cy, inner_radius, outer_radius, start, fill, C_DISK_USED);
  CanvasDrawAt (disk_canvas, win, x + 2, y + 1);

  wmove (win, y + 3, x + 5);
  wprintw (win, "%2d%%", (int)ceil (used_percent * 100.0));

  p = print_buf;
  p = DiskFormatSize (p, fs_info->used);
  p = StringPush (p, " of ");
  p = DiskFormatSize (p, fs_info->total);
  len = strlen (print_buf);
  wmove (win, y + 6, x + (disk_width - len) / 2);
  waddstr (win, print_buf);
}

void
DiskDraw (WINDOW *win)
{
  int x = 1, y = 1;
  if (disk_vertical)
    y += disk_margin;
  else
    x += disk_margin;
  list_for_each (disk_filesystems, it)
    {
      if (((Disk_FS_Info *)it->p)->changed)
        DiskDrawFSInfo (win, it->p, x, y);
      if (disk_vertical)
        y += disk_height + disk_padding;
      else
        x += disk_width + disk_padding;
    }
}

void
DiskResize (WINDOW *win)
{
  int total, content, space, min_padding;
  wclear (win);
  DiskForceUpdate ();

  if (disk_vertical)
    {
      total = getmaxy (win) - 2;
      content = disk_filesystems->count * disk_height;
      space = (total - content) / (disk_filesystems->count + 1);
      min_padding = 1;
    }
  else
    {
      total = getmaxx (win) - 2;
      content = disk_filesystems->count * disk_width;
      space = (total - content) / (disk_filesystems->count + 1);
      min_padding = 3;
    }

  if (space < min_padding)
    {
      disk_padding = min_padding;
      content += min_padding * (disk_filesystems->count - 1);
      disk_margin = (total - content) / 2;
    }
  else if (disk_filesystems->count == 2)
    {
      /* Example: we have 4 rows of spacing; with the default logic we would
                  get 1-1-2, this changes it to 1-2-1. */
      disk_margin = space;
      disk_padding = ((total - content + disk_filesystems->count)
                      / (disk_filesystems->count + 1));
    }
  else
    disk_margin = disk_padding = space;
}

void
DiskMinSize (int *width_return, int *height_return)
{
  if (!disk_filesystems)
    {
      /* InitWidgets not called yet, just assume one filesystem. */
      *width_return = disk_width;
      *height_return = disk_height;
    }
  else if (disk_vertical)
    {
      *width_return = disk_width;
      *height_return = disk_filesystems->count * (disk_height + 1) - 1;
    }
  else
    {
      *width_return = disk_filesystems->count * (disk_width + 3) - 3;
      *height_return = disk_height;
    }
  DiskForceUpdate ();
}

void
DiskDrawBorder (WINDOW *win)
{
  DrawWindow (win, "Disk usage");
}
