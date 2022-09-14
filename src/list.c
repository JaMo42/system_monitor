#include "list.h"
#include <stdlib.h>

List *
list_create ()
{
  List *l = (List *)malloc (sizeof (List));
  l->front = NULL;
  l->back = NULL;
  l->count = 0;
  return l;
}

static void
list__delete (List_Node *n)
{
  if (n->next)
    list__delete (n->next);
  free (n);
}

void
list_delete (List *l)
{
  if (l->front)
    list__delete (l->front);
  free (l);
}

#define LIST_PUSH_EMPTY(l)\
  if (l->front == NULL) \
    { \
      l->front = (List_Node *)malloc (sizeof (List_Node)); \
      l->front->next = NULL; \
      l->back = l->front; \
      return l->front; \
    }

List_Node *
list_push_back (List *l)
{
  ++l->count;
  LIST_PUSH_EMPTY (l);
  l->back->next = (List_Node *)malloc (sizeof (List_Node));
  l->back = l->back->next;
  l->back->next = NULL;
  return l->back;
}

List_Node *
list_push_front (List *l)
{
  ++l->count;
  LIST_PUSH_EMPTY (l);
  List_Node *n = (List_Node *)malloc (sizeof (List_Node));
  n->next = l->front;
  l->front = n;
  return l->front;
}

void
list_pop_back (List *l)
{
  if (l->front)
    {
      List_Node *n = l->front;
      while (n->next != l->back)
        n = n->next;
      free (l->back);
      l->back = n;
      l->back->next = NULL;
      --l->count;
    }
}

void
list_pop_front (List *l)
{
  if (l->front)
    {
      List_Node *n = l->front;
      l->front = l->front->next;
      free (n);
      --l->count;
    }
}

List_Node *
list_rotate_left (List *l)
{
  if (l->front && l->back != l->front)
    {
      List_Node *take = l->front;
      l->front = l->front->next;
      l->back->next = take;
      l->back = take;
      l->back->next = NULL;
    }
  return l->back;
}

void list_clear (List *l)
{
  if (!l->front)
    return;
  list__delete (l->front);
  l->front = NULL;
  l->back = NULL;
  l->count = 0;
}
