/*  $Id: queue.h,v 1.1 2006/11/20 13:36:12 tho Exp $ */
/*  $OpenBSD: queue.h,v 1.22 2001/06/23 04:39:35 angelos Exp $  */
/*  $NetBSD: queue.h,v 1.11 1996/05/16 05:17:14 mycroft Exp $   */

#ifndef _U_QUEUE_H_
#define _U_QUEUE_H_

/* A list is headed by a single forward pointer (or an array of forward
 * pointers for a hash table header). The elements are doubly linked so that
 * an arbitrary element can be removed without a need to traverse the list. 
 * New elements can be added to the list before or after an existing element 
 * or at the head of the list. A list may only be traversed in the forward 
 * direction. */

#define LIST_HEAD(name, type)                                           \
struct name {                                                           \
        struct type *lh_first;  /* first element */                     \
}

#define LIST_HEAD_INITIALIZER(head)                                     \
        { NULL }

#define LIST_ENTRY(type)                                                \
struct {                                                                \
        struct type *le_next;   /* next element */                      \
        struct type **le_prev;  /* address of previous next element */  \
}

#define LIST_FIRST(head)                ((head)->lh_first)
#define LIST_END(head)                  NULL
#define LIST_EMPTY(head)                (LIST_FIRST(head) == LIST_END(head))
#define LIST_NEXT(elm, field)           ((elm)->field.le_next)

#define LIST_FOREACH(var, head, field)                                  \
        for((var) = LIST_FIRST(head);                                   \
            (var)!= LIST_END(head);                                     \
            (var) = LIST_NEXT(var, field))

#define LIST_INIT(head) do {                                            \
        LIST_FIRST(head) = LIST_END(head);                              \
} while (0)

#define LIST_INSERT_AFTER(listelm, elm, field) do {                     \
        if (((elm)->field.le_next = (listelm)->field.le_next) != NULL)  \
                (listelm)->field.le_next->field.le_prev =               \
                    &(elm)->field.le_next;                              \
        (listelm)->field.le_next = (elm);                               \
        (elm)->field.le_prev = &(listelm)->field.le_next;               \
} while (0)

#define LIST_INSERT_BEFORE(listelm, elm, field) do {                    \
        (elm)->field.le_prev = (listelm)->field.le_prev;                \
        (elm)->field.le_next = (listelm);                               \
        *(listelm)->field.le_prev = (elm);                              \
        (listelm)->field.le_prev = &(elm)->field.le_next;               \
} while (0)

#define LIST_INSERT_HEAD(head, elm, field) do {                         \
        if (((elm)->field.le_next = (head)->lh_first) != NULL)          \
                (head)->lh_first->field.le_prev = &(elm)->field.le_next;\
        (head)->lh_first = (elm);                                       \
        (elm)->field.le_prev = &(head)->lh_first;                       \
} while (0)

#define LIST_REMOVE(elm, field) do {                                    \
        if ((elm)->field.le_next != NULL)                               \
                (elm)->field.le_next->field.le_prev =                   \
                    (elm)->field.le_prev;                               \
        *(elm)->field.le_prev = (elm)->field.le_next;                   \
} while (0)

#define LIST_REPLACE(elm, elm2, field) do {                             \
        if (((elm2)->field.le_next = (elm)->field.le_next) != NULL)     \
                (elm2)->field.le_next->field.le_prev =                  \
                    &(elm2)->field.le_next;                             \
        (elm2)->field.le_prev = (elm)->field.le_prev;                   \
        *(elm2)->field.le_prev = (elm2);                                \
} while (0)


/* A tail queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list after
 * an existing element, at the head of the list, or at the end of the
 * list. A tail queue may only be traversed in the forward direction. */

/* Tail queue definitions. */
#define TAILQ_HEAD(name, type)                                              \
struct name {                                                               \
        struct type *tqh_first;     /* first element */                     \
        struct type **tqh_last;     /* addr of last next element */         \
}

#define TAILQ_ENTRY(type)                                                   \
struct {                                                                    \
        struct type *tqe_next;      /* next element */                      \
        struct type **tqe_prev;     /* address of previous next element */  \
}

/* Tail queue functions. */
#define TAILQ_INIT(head) {                                                  \
        (head)->tqh_first = NULL;                                           \
        (head)->tqh_last = &(head)->tqh_first;                              \
}

#define TAILQ_INSERT_HEAD(head, elm, field) {                               \
        if (((elm)->field.tqe_next = (head)->tqh_first) != NULL)            \
                (elm)->field.tqe_next->field.tqe_prev =                     \
                    &(elm)->field.tqe_next;                                 \
        else                                                                \
                (head)->tqh_last = &(elm)->field.tqe_next;                  \
        (head)->tqh_first = (elm);                                          \
        (elm)->field.tqe_prev = &(head)->tqh_first;                         \
}

#define TAILQ_INSERT_TAIL(head, elm, field) {                               \
        (elm)->field.tqe_next = NULL;                                       \
        (elm)->field.tqe_prev = (head)->tqh_last;                           \
        *(head)->tqh_last = (elm);                                          \
        (head)->tqh_last = &(elm)->field.tqe_next;                          \
}

#define TAILQ_INSERT_AFTER(head, listelm, elm, field) {                     \
        if (((elm)->field.tqe_next = (listelm)->field.tqe_next) != NULL)    \
                (elm)->field.tqe_next->field.tqe_prev =                     \
                    &(elm)->field.tqe_next;                                 \
        else                                                                \
                (head)->tqh_last = &(elm)->field.tqe_next;                  \
        (listelm)->field.tqe_next = (elm);                                  \
        (elm)->field.tqe_prev = &(listelm)->field.tqe_next;                 \
}

#define TAILQ_INSERT_BEFORE(listelm, elm, field) do {                       \
        (elm)->field.tqe_prev = (listelm)->field.tqe_prev;                  \
        (elm)->field.tqe_next = (listelm);                                  \
        *(listelm)->field.tqe_prev = (elm);                                 \
        (listelm)->field.tqe_prev = &(elm)->field.tqe_next;                 \
} while (0)

#define TAILQ_REMOVE(head, elm, field) {                                    \
        if (((elm)->field.tqe_next) != NULL)                                \
                (elm)->field.tqe_next->field.tqe_prev =                     \
                    (elm)->field.tqe_prev;                                  \
        else                                                                \
                (head)->tqh_last = (elm)->field.tqe_prev;                   \
        *(elm)->field.tqe_prev = (elm)->field.tqe_next;                     \
}


/* A circle queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before or after
 * an existing element, at the head of the list, or at the end of the list.
 * A circle queue may be traversed in either direction, but has a more
 * complex end of list detection. */

/* Circular queue definitions. */
#define CIRCLEQ_HEAD(name, type)                                            \
struct name {                                                               \
        struct type *cqh_first;               /* first element */           \
        struct type *cqh_last;                /* last element */            \
}

#define CIRCLEQ_ENTRY(type)                                                 \
struct {                                                                    \
        struct type *cqe_next;                /* next element */            \
        struct type *cqe_prev;                /* previous element */        \
}

/* Circular queue functions. */
#define CIRCLEQ_INIT(head) {                                                \
        (head)->cqh_first = (void *)(head);                                 \
        (head)->cqh_last = (void *)(head);                                  \
}

#define CIRCLEQ_INSERT_AFTER(head, listelm, elm, field) {                   \
        (elm)->field.cqe_next = (listelm)->field.cqe_next;                  \
        (elm)->field.cqe_prev = (listelm);                                  \
        if ((listelm)->field.cqe_next == (void *)(head))                    \
                (head)->cqh_last = (elm);                                   \
        else                                                                \
                (listelm)->field.cqe_next->field.cqe_prev = (elm);          \
        (listelm)->field.cqe_next = (elm);                                  \
}

#define CIRCLEQ_INSERT_BEFORE(head, listelm, elm, field) {                  \
        (elm)->field.cqe_next = (listelm);                                  \
        (elm)->field.cqe_prev = (listelm)->field.cqe_prev;                  \
        if ((listelm)->field.cqe_prev == (void *)(head))                    \
                (head)->cqh_first = (elm);                                  \
        else                                                                \
                (listelm)->field.cqe_prev->field.cqe_next = (elm);          \
        (listelm)->field.cqe_prev = (elm);                                  \
}

#define CIRCLEQ_INSERT_HEAD(head, elm, field) {                             \
        (elm)->field.cqe_next = (head)->cqh_first;                          \
        (elm)->field.cqe_prev = (void *)(head);                             \
        if ((head)->cqh_last == (void *)(head))                             \
                (head)->cqh_last = (elm);                                   \
        else                                                                \
                (head)->cqh_first->field.cqe_prev = (elm);                  \
        (head)->cqh_first = (elm);                                          \
}

#define CIRCLEQ_INSERT_TAIL(head, elm, field) {                             \
        (elm)->field.cqe_next = (void *)(head);                             \
        (elm)->field.cqe_prev = (head)->cqh_last;                           \
        if ((head)->cqh_first == (void *)(head))                            \
                (head)->cqh_first = (elm);                                  \
        else                                                                \
                (head)->cqh_last->field.cqe_next = (elm);                   \
        (head)->cqh_last = (elm);                                           \
}

#define CIRCLEQ_REMOVE(head, elm, field) {                                  \
        if ((elm)->field.cqe_next == (void *)(head))                        \
                (head)->cqh_last = (elm)->field.cqe_prev;                   \
        else                                                                \
                (elm)->field.cqe_next->field.cqe_prev =                     \
                    (elm)->field.cqe_prev;                                  \
        if ((elm)->field.cqe_prev == (void *)(head))                        \
                (head)->cqh_first = (elm)->field.cqe_next;                  \
        else                                                                \
                (elm)->field.cqe_prev->field.cqe_next =                     \
                    (elm)->field.cqe_next;                                  \
}

#ifndef LIST_ENTRY_NULL
#define LIST_ENTRY_NULL { NULL, NULL }
#endif

#ifndef LIST_FOREACH_SAFE
#define LIST_FOREACH_SAFE(var, head, field, tvar)                       \
        for ((var) = LIST_FIRST((head));                                \
            (var) && ((tvar) = LIST_NEXT((var), field), 1);             \
            (var) = (tvar))
#endif

#ifndef LIST_FIRST
#define LIST_FIRST(head)    ((head)->lh_first)
#endif 

#ifndef TAILQ_ENTRY_NULL
#define TAILQ_ENTRY_NULL { NULL, NULL }
#endif

#ifndef TAILQ_FIRST
#define TAILQ_FIRST(head)   ((head)->tqh_first)
#endif 

#ifndef TAILQ_END
#define TAILQ_END(head)     NULL
#endif

#ifndef TAILQ_NEXT
#define TAILQ_NEXT(elm, field)  ((elm)->field.tqe_next)
#endif

#ifndef TAILQ_LAST
#define TAILQ_LAST(head, headname)                                          \
        (*(((struct headname *)((head)->tqh_last))->tqh_last))
#endif

#ifndef TAILQ_PREV
#define TAILQ_PREV(elm, headname, field)                                    \
        (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))
#endif

#ifndef TAILQ_EMPTY
#define TAILQ_EMPTY(head)                                                   \
        (TAILQ_FIRST(head) == TAILQ_END(head))
#endif

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(var, head, field)                                     \
        for((var) = TAILQ_FIRST(head);                                      \
            (var) != TAILQ_END(head);                                       \
            (var) = TAILQ_NEXT(var, field))
#endif

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)                      \
        for ((var) = TAILQ_FIRST((head));                               \
            (var) && ((tvar) = TAILQ_NEXT((var), field), 1);            \
            (var) = (tvar))
          
#endif
#ifndef TAILQ_FOREACH_REVERSE
#define TAILQ_FOREACH_REVERSE(var, head, headname, field)               \
        for ((var) = TAILQ_LAST((head), headname);                      \
            (var);                                                      \
            (var) = TAILQ_PREV((var), headname, field))

#endif

#ifndef TAILQ_FOREACH_REVERSE_SAFE
#define TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)    \
        for ((var) = TAILQ_LAST((head), headname);                      \
            (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1);  \
            (var) = (tvar))
#endif

#endif /* !_U_QUEUE_H_ */
