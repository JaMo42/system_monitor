#ifndef LIST_H
#define LIST_H
#include <stddef.h>
#include <stdint.h>

typedef struct List_Node
{
  union
    {
      double f;
      intmax_t d;
      uintmax_t u;
      char *s;
      void *p;
    };
  struct List_Node *next;
} List_Node;

typedef struct List
{
  List_Node *front;
  List_Node *back;
  size_t count;
} List;

List *list_create ();
void list_delete (List *);

List_Node *list_push_back (List *);
List_Node *list_push_front (List *);
void list_pop_back (List *);
void list_pop_front (List *);

List_Node *list_rotate_left (List *);

void list_clear (List *);

#define list_for_each(l, it)\
  for (List_Node *it = l->front; it != NULL; it = it->next)

#endif /* !LIST_H */
