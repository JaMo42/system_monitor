#include "layout_parser.h"
#include "sm.h"
#include "ic.h"
#include <assert.h>

#define C_ERROR "\x1b[1;91m"
#define C_NOTE "\x1b[1;92m"
#define C_MESSAGE "\x1b[1m"
#define C_STRUCT "\x1b[1;94m"

typedef enum Token_Kind Token_Kind;
typedef struct Token Token;
typedef struct Used_Widget Used_Widget;

enum Token_Kind {
  TOK_OPEN_PAREN,
  TOK_CLOSE_PAREN,
  TOK_OPEN_BRACKET,
  TOK_CLOSE_BRACKET,
  TOK_SHAPE_ROWS,
  TOK_SHAPE_COLS,
  TOK_WIDGET_NAME,
  TOK_NUMBER,
  TOK_PERCENT,
  TOK_STRICT,
  TOK_END
};

struct Token
{
  Token_Kind kind;
  union
  {
    const char *text;
    intmax_t number;
  };
  size_t begin;
  size_t length;
  Token *next;
};

struct Used_Widget
{
  Widget *widget;
  Token *token;
  Used_Widget *next;
};

static Token *tokens;
static const char *source;
static const char *source_name;
static const char *widget_choices;
static Used_Widget used_widgets_proxy = { .next = NULL };
static Used_Widget *used_widgets_last = &used_widgets_proxy;

static void
IndexToPosition (size_t idx, int *line, int *col)
{
  size_t i;
  *line = 1;
  *col = 1;
  for (i = 0; i < idx; ++i)
    {
      if (source[i] == '\n')
        {
          ++(*line);
          *col = 1;
        }
      else
        ++(*col);
    }
}

static void
Error (const char *format, ...)
{
  va_list ap;
  fputs (C_ERROR"error:\x1b[0m "C_MESSAGE, stderr);
  va_start (ap, format);
  vfprintf (stderr, format, ap);
  va_end (ap);
  fputs ("\x1b[0m\n", stderr);
}

static int
ShowSourceImpl (size_t idx, size_t len, int line_num_width,
                const char *highlight, const char *format, va_list ap)
{
  const char *line = source + idx;
  int line_length = 0, line_number, column, i;
  IndexToPosition (idx, &line_number, &column);
  while (line > source && *line != '\n')
    --line;
  if (*line == '\n')
    ++line;
  while (line[line_length] && line[line_length] != '\n')
    ++line_length;
  fprintf (stderr, "%*c"C_STRUCT"-->\x1b[0m %s:%d:%d\n",
           line_num_width, ' ', source_name, line_number, column);
  fprintf (stderr, C_STRUCT"%*c |\x1b[0m\n", line_num_width, ' ');
  fprintf (stderr, C_STRUCT"%*d |\x1b[0m %.*s\n", line_num_width, line_number,
           line_length, line);
  fprintf (stderr, C_STRUCT"%*c |\x1b[0m ", line_num_width, ' ');
  for (i = 1; i < column; ++i)
    fputc (' ', stderr);
  fputs (highlight, stderr);
  for (i = 0; i < (int)len; ++i)
    fputc ('^', stderr);
  fputc (' ', stderr);
  if (*format)
    vfprintf (stderr, format, ap);
  fputs ("\x1b[0m\n", stderr);
  return line_num_width;
}

static int
ErrorShowSource (size_t idx, size_t len, const char *format, ...)
{
  va_list ap;
  int width, line_number, column;
  IndexToPosition (idx, &line_number, &column);
  width = snprintf (NULL, 0, "%d", line_number);
  va_start (ap, format);
  const int result = ShowSourceImpl (idx, len, width, C_ERROR, format, ap);
  va_end (ap);
  return result;
}

static void
HintValid (int line_num_width, const char *for_, const char *choices)
{
  const char *p;
  int len = 0;
  fprintf (stderr, C_STRUCT"%*c |\x1b[0m\n"C_NOTE"hint:\x1b[0m valid %s are\n",
           line_num_width, ' ', for_);
  fprintf (stderr, C_STRUCT"%*c |\x1b[0m - ", line_num_width, ' ');
  while ((p = strpbrk (choices, ",;")))
    {
      len = p - choices;
      if (*p == ',')
        fprintf (stderr, "‘%.*s’, ", len, choices);
      else
        fprintf (stderr, "‘%.*s’\n"C_STRUCT"%*c |\x1b[0m - ",
                 len, choices, line_num_width, ' ');
      choices = p + 1;
    }
  fprintf (stderr, "‘%s’\n", choices);
}

static void
NoteVA (int line_num_width, bool inline_, const char *format, va_list ap)
{
  fprintf (stderr, C_STRUCT"%*c |\x1b[0m\n", line_num_width, ' ');
  if (inline_)
    fprintf (stderr, C_STRUCT"%*c =\x1b[0m"C_MESSAGE" note:\x1b[0m ",
             line_num_width, ' ');
  else
    fprintf (stderr, C_NOTE"note:\x1b[0m ");
  vfprintf (stderr, format, ap);
  fputc ('\n', stderr);
}

static void
Note (int line_num_width, bool inline_, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  NoteVA (line_num_width, inline_, format, ap);
  va_end (ap);
}

static void
ErrorDuplicate (Token *first, Token *second, const char *first_format,
                const char *second_format, ...)
{
  va_list ap;
  int first_width, second_width, line_number, column;
  IndexToPosition (first->begin, &line_number, &column);
  first_width = snprintf (NULL, 0, "%d", line_number);
  IndexToPosition (second->begin, &line_number, &column);
  second_width = snprintf (NULL, 0, "%d", line_number);
  if (second_width > first_width)
    first_width = second_width;

  va_start (ap, second_format);
  ShowSourceImpl (first->begin, first->length, first_width, C_ERROR,
                  first_format, ap);
  NoteVA (first_width, false, second_format, ap);
  ShowSourceImpl (second->begin, second->length, first_width, C_NOTE,
                  "", ap);
  va_end (ap);
}

static void
Tokenize ()
{
  Token dummy;
  Token *last = &dummy;
  size_t pos = 0, end, len;
  char ch;
  bool next_word_is_layout_shape = false;
  bool is_first = true;
  char *end_ptr;
  const char *tok;
  intmax_t number;
  bool whitespace;

  #define PUSH(k, l) \
    do { \
      last->next = malloc (sizeof (Token)); \
      last = last->next; \
      last->kind = k; \
      last->text = source + pos; \
      last->begin = pos; \
      last->length = l; \
      pos += l; \
    } while (0)

  #define TOKEQ(s) \
    (sizeof (s)-1 == len && strncmp (tok, s, len) == 0)

  while ((ch = source[pos]))
    {
      whitespace = false;
      switch (ch)
        {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\v':
        case '\f':
          ++pos;
          whitespace = true;
          break;

        case '(':
          PUSH (TOK_OPEN_PAREN, 1);
          next_word_is_layout_shape = true;
          break;
        case ')':
          PUSH (TOK_CLOSE_PAREN, 1);
          break;

        case '[':
          PUSH (TOK_OPEN_BRACKET, 1);
          break;
        case ']':
          PUSH (TOK_CLOSE_BRACKET, 1);
          break;

        case 'a'...'z':
        case 'A'...'Z':
          end = pos;
          while (source[end] && isalnum (source[end]))
            ++end;
          tok = source + pos;
          len = end - pos;
          if (TOKEQ ("strict"))
            {
              if (!is_first)
                {
                  Error ("‘strict’ must be at the beginning of the layout string");
                  ErrorShowSource (pos, len, "");
                  exit (1);
                }
              PUSH (TOK_STRICT, len);
            }
          else if (next_word_is_layout_shape)
            {
              if (TOKEQ ("rows") || TOKEQ ("horizontal"))
                PUSH (TOK_SHAPE_ROWS, len);
              else if (TOKEQ ("cols") || TOKEQ ("columns") || TOKEQ ("vertical"))
                PUSH (TOK_SHAPE_COLS, len);
              else
                {
                  Error ("unknown layout shape ‘%.*s’", (int)len, tok);
                  len = ErrorShowSource (pos, len, "expected layout shape");
                  HintValid (len, "shape names", "rows,horizontal;cols,columns,vertical");
                  exit (1);
                }
              next_word_is_layout_shape = false;
            }
          else
            PUSH (TOK_WIDGET_NAME, end-pos);
          break;

        case '0'...'9':
          end = pos;
          while (source[end] && isdigit (source[end]))
            ++end;
          number = strtoll (source+pos, &end_ptr, 10);
          assert (end_ptr == source+end);
          if (strchr (" \t\n\r\v\f()[]%", *end_ptr) == NULL)
            {
              pos = end;
              while (isalnum (source[pos]))
                ++pos;
              Error ("unexpected suffix on unmber");
              ErrorShowSource (end, pos - end, "");
              exit (1);
            }
          PUSH (TOK_NUMBER, end-pos);
          last->number = number;
          break;

        case '%':
          PUSH (TOK_PERCENT, 1);
          break;

        default:
          Error ("unexpected symbol");
          ErrorShowSource (pos, 1, "");
          exit (1);
        }
      if (!whitespace)
        is_first = false;
    }

  PUSH (TOK_END, 1);
  last->next = NULL;
  tokens = dummy.next;
#undef PUSH
}

static inline Token *
Next (const char *expected)
{
  Token *const tok = tokens;
  if (tok->kind == TOK_END)
    {
      Error ("unexpected end of layout string");
      ErrorShowSource (tok->begin, tok->length, "expected %s", expected);
      exit (1);
    }
  tokens = tok->next;
  return tok;
}

static inline Token *
Peek (const char *expected)
{
  Token *const tok = tokens;
  if (tok->kind == TOK_END)
    {
      Error ("unexpected end of layout string");
      ErrorShowSource (tok->begin, tok->length, "expected %s", expected);
      exit (1);
    }
  return tok;
}

/* Generates a format string for HintValid with all available widgets. */
static inline void
MakeWidgetChoices ()
{
  char *str;
  int count = 0, length = 0;
  Widget **it;
  for (it = all_widgets; *it != NULL; ++it)
    {
      ++count;
      length += strlen ((*it)->name);
    }
  str = malloc (length + count);
  widget_choices = str;
  for (it = all_widgets; *it != NULL; ++it)
    {
      length = strlen ((*it)->name);
      memcpy (str, (*it)->name, length);
      str += length;
      *str++ = ';';
    }
  str[-1] = '\0';
}

static inline Widget *
ParseWidget ()
{
  Token *tok = Next ("");
  Widget **it, *w, *match = NULL;
  int line_num_width;
  Used_Widget *used_it, *use;

  for (it = all_widgets; *it != NULL; ++it)
    {
      w = *it;
      if (strncmp (tok->text, w->name, tok->length) == 0)
        {
          if (match)
            {
              Error ("ambiguous widget name");
              line_num_width = ErrorShowSource (tok->begin, tok->length, "widget referenced here");
              Note (line_num_width, true, "matches ‘%s’ and ‘%s’", match->name, w->name);
              exit (1);
            }
          match = w;
        }
    }
  if (!match)
    {
      MakeWidgetChoices ();
      Error ("no matching widget");
      line_num_width = ErrorShowSource (tok->begin, tok->length, "widget referenced here");
      HintValid (line_num_width, "widget names", widget_choices);
      exit (1);
    }
  for (used_it = used_widgets_proxy.next; used_it; used_it = used_it->next)
    {
      if (used_it->widget == match)
        {
          Error ("duplicate widget");
          ErrorDuplicate (tok, used_it->token, "‘%s’ reused here",
                          "previous usage here", match->name);
          exit (1);
        }
    }
  use = malloc (sizeof (Used_Widget));
  *use = (Used_Widget) {
    .widget = match,
    .token = tok,
    .next = NULL
  };
  used_widgets_last->next = use;
  used_widgets_last = used_widgets_last->next;
  return match;
}

static inline int
ParsePriority (size_t name_end)
{
  Token *tok;
  size_t num_end;
  int priority;
  if ((tok = Next ("‘[’"))->kind != TOK_OPEN_BRACKET)
    {
      Error ("missing widget priority");
      ErrorShowSource (name_end - 1, 1, "expected ‘[’");
      exit (1);
    }
  if ((tok = Next ("widget priority"))->kind != TOK_NUMBER)
    {
      Error ("missing widget priority");
      ErrorShowSource (tok->begin, tok->length, "expected number");
      exit (1);
    }
  priority = tok->number;
  num_end = tok->begin + tok->length;
  if ((tok = Next ("‘]’"))->kind != TOK_CLOSE_BRACKET)
    {
      Error ("missing widget priority");
      ErrorShowSource (num_end, 1, "expected ‘]’");
      exit (1);
    }
  return priority;
}

static Layout* ParseLayout ();

static inline void
ParseChild (Layout *layout)
{
  Token *tok;
  Widget *widget;
  int priority;

  tok = Peek ("widget or layout");
  switch (tok->kind)
    {
    case TOK_WIDGET_NAME:
      widget = ParseWidget ();
      priority = ParsePriority (tokens->begin + tokens->length);
      UIAddWidget (layout, widget, priority);
      break;

    case TOK_OPEN_PAREN:
      UIAddLayout (layout, ParseLayout ());
      break;

    default:
      Error ("invalid layout child");
      ErrorShowSource (tok->begin, tok->length, "expected widget or sub-layout");
      exit (1);
      break;
    }
}

static Layout *
ParseLayout ()
{
  Token *tok;
  LayoutType type;
  int line_num_width;
  size_t pos, len;
  float percent_first = 0.5f;
  Layout *layout;

  Next ("");  // Consume '('

  switch ((tok = Next ("layout shape"))->kind)
    {
    case TOK_SHAPE_COLS:
      type = UI_COLS;
      break;
    case TOK_SHAPE_ROWS:
      type = UI_ROWS;
      break;
    default:
      Error ("missing layout shape");
      line_num_width = ErrorShowSource (tok->begin, tok->length, "expected layout shape");
      HintValid (line_num_width, "shape names", "rows,horizontal;cols,columns,vertical");
      exit (1);
    }

  tok = Peek ("percentage or widget name");
  if (tok->kind == TOK_NUMBER)
    {
      Next ("");
      pos = tok->begin;
      len = tok->length;
      if (tok->number > 100)
        tok->number = 100;
      percent_first = (float)tok->number / 100.0f;
      if ((tok = Next ("‘%’"))->kind != TOK_PERCENT)
        {
          Error ("relative size of first child must be a percentage");
          ErrorShowSource (pos+len, 1, "expected ‘%’");
          exit (1);
        }
    }

  layout = UICreateLayout (type, percent_first);

  ParseChild (layout);
  ParseChild (layout);

  if ((tok = Next ("‘)’"))->kind != TOK_CLOSE_PAREN)
    {
      Error ("too many items in layout");
      ErrorShowSource (tok->begin, tok->length, "expected ‘)’");
      exit (1);
    }

  return layout;
}

Layout *
ParseLayoutString (const char *source_, const char *source_name_)
{
  source = source_;
  source_name = source_name_;
  Tokenize ();

  int line_num_width;
  Token *tok = Peek ("‘strict’ or ‘(’");
  Layout *layout = NULL;
  if (tok->kind == TOK_STRICT)
    {
      Next ("");
      ui_strict_size = true;
      tok = Peek ("layout");
    }
  if (tok->kind == TOK_OPEN_PAREN)
    {
      layout = ParseLayout ();
    }
  else
    {
      Error (tok->kind == TOK_END
             ? "unexpected end of layout string"
             : "unexpected token");
      line_num_width = ErrorShowSource (tok->begin, tok->length,
                                        "expected toplevel layout");
      Note (line_num_width, true, "the layout string should look like "
                                  "‘(...)’ or ‘strict (...)’");
      exit (1);
    }
  if (tokens->kind != TOK_END)
    {
      Error ("unexpected token after toplevel layout");
      line_num_width = ErrorShowSource (tokens->begin, tokens->length, "");
      Note (line_num_width, true, "the layout string should look like "
                                  "‘(...)’ or ‘strict (...)’");
      exit (1);
    }

  return layout;
}

