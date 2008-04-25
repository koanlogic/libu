/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

static const char rcsid[] =
    "$Id: list.c,v 1.3 2008/04/25 19:29:18 tat Exp $";

#include <u/libu_conf.h>
#include <u/libu.h>
#include <toolbox/list.h>

typedef struct u_list_item_s
{
    TAILQ_ENTRY(u_list_item_s) np;
    void *ptr;
} u_list_item_t;

struct u_list_s
{
    TAILQ_HEAD(u_list_head_s, u_list_item_s) head;
    size_t count;
};

/**
 *  \defgroup list Lists
 *  \{
 */

/**
 *  \brief  Create a new list object
 *
 *  \params plist   the newly created list object as a result argument
 *
 *  \return \c 0 on success, \c ~0 on error
 */ 
int u_list_create(u_list_t **plist)
{
    u_list_t *list = NULL;

    list = u_zalloc(sizeof(u_list_t));
    dbg_err_if(list == NULL); 

    TAILQ_INIT(&list->head);

    *plist = list;

    return 0;
err:
    if(list)
        u_free(list);
    return ~0;
}

/**
 *  \brief  Free the list object: note the list doesn't own the pointers in it,
 *          the client must free them
 *
 *  \params list    the list object that has to be disposed
 *
 *  \return nothing 
 */ 
void u_list_free(u_list_t *list)
{
    u_list_item_t *item = NULL;

    dbg_return_if(list == NULL, );

    while((item = TAILQ_FIRST(&list->head)) != NULL)
    {
        TAILQ_REMOVE(&list->head, item, np);

        u_free(item);
    }

    u_free(list);

    return;
}

/**
 *  \brief  Push an element to the list
 *
 *  \params list    the partent list object (created via u_list_new)
 *  \params ptr     the element that has to be push'd
 *
 *  \return \c 0 on success, \c ~0 on error
 */ 
int u_list_add(u_list_t *list, void *ptr)
{
    u_list_item_t *item = NULL;

    dbg_return_if (list == NULL, ~0);
    dbg_return_if (ptr == NULL, ~0);

    item = (u_list_item_t*)u_zalloc(sizeof(u_list_item_t));
    dbg_err_if(item == NULL);

    item->ptr = ptr;
        
    TAILQ_INSERT_TAIL(&list->head, item, np);
    list->count++;

    return 0;
err:
    if(item)
        u_free(item);
    return ~0;
}

/**
 *  \brief  Pop an element from the list
 *
 *  \params list    the partent list object (created via u_list_new)
 *  \params ptr     the element that has to be pop'd
 *
 *  \return \c 0 if \p ptr has been removed, \c ~0 if \p ptr was not found
 */ 
int u_list_del(u_list_t *list, void *ptr)
{
    u_list_item_t *item = NULL;

    TAILQ_FOREACH(item, &list->head, np)
    {
        if(item->ptr == ptr)
        {
            TAILQ_REMOVE(&list->head, item, np);
            list->count--;
            u_free(item);
            return 0; /* removed */
        }
    }

    return ~0; /* not found */
}

/**
 *  \brief  Count elements in list
 *
 *  \params list    a list object
 *
 *  \return the number of elements in \p list
 */ 
size_t u_list_count(u_list_t *list)
{
    /* a SIGBUS is better than returning 0 if list == NULL */
    return list->count;
}

/**
 *  \brief  Get the n-th element in list
 *
 *  \params list    a list object
 *  \params n       the ordinal of the element that should be retrieved
 *
 *  \return the pointer to the n-th element or \c NULL if no n-th element has
 *          been found
 */ 
void* u_list_get_n(u_list_t *list, size_t n)
{
    u_list_item_t *item = NULL;

    dbg_return_if (list == NULL, NULL);
    dbg_return_if (n > list->count, NULL);

    TAILQ_FOREACH(item, &list->head, np)
    {
        if(n-- == 0)
            return item->ptr;
    }

    return NULL;
}

/**
 *  \}
 */
