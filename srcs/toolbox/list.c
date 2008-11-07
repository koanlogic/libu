/* 
 * Copyright (c) 2005-2008 by KoanLogic s.r.l. - All rights reserved.  
 */

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
 *  \param plist   the newly created list object as a result argument
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
 *  \param list    the list object that has to be disposed
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
 *  \param list    the parent list object (created via u_list_new)
 *  \param ptr     the element that has to be push'd
 *
 *  \return \c 0 on success, \c ~0 on error
 */ 
int u_list_add(u_list_t *list, void *ptr)
{
    return u_list_insert(list, ptr, list->count);
}

/**
 *  \brief  Pop an element from the list
 *
 *  \param list    the parent list object (created via u_list_new)
 *  \param ptr     the element that has to be pop'd
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
 *  \param list    a list object
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
 *  \param list    a list object
 *  \param n       the ordinal of the element that should be retrieved
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
 *  \brief  Insert an element at the given position
 *
 *  \param list    the parent list object (created via u_list_new)
 *  \param ptr     the element that has to be push'd
 *  \param n       the position in the list (from zero to N)
 *
 *  \return \c 0 on success, \c ~0 on error
 */ 
int u_list_insert(u_list_t *list, void *ptr, size_t n)
{
    u_list_item_t *prev, *item = NULL;

    dbg_return_if (list == NULL, ~0);
    dbg_return_if (n > list->count, ~0);

    item = (u_list_item_t*)u_zalloc(sizeof(u_list_item_t));
    dbg_err_if(item == NULL);

    item->ptr = ptr;
            
    if(n == 0)
    {
        TAILQ_INSERT_HEAD(&list->head, item, np);
    } else if (n == list->count) {
        TAILQ_INSERT_TAIL(&list->head, item, np);
    } else {
        /* find the current n-th elem */
        TAILQ_FOREACH(prev, &list->head, np)
        {
            if(n-- == 0)
                break;
        }

        TAILQ_INSERT_BEFORE(prev, item, np);
    }

    list->count++;

    return 0;
err:
    if(item)
        u_free(item);
    return ~0;
}

/**
 *  \brief  Return the first item of the list (use to iterate)
 *
 *  \param list    the list object (created via u_list_new)
 *
 *  \return \c the first item or NULL if the list is empty 
 */ 
void* u_list_first(u_list_t *list, void **it)
{
    u_list_item_t *item;

    dbg_return_if (list == NULL, NULL);

    item = TAILQ_FIRST(&list->head);

    if(it)
        *it = item;
   
    if(item)
        return item->ptr;

    return NULL;
}

/**
 *  \brief  Return the list element next to the given one
 *
 *  \param list     the list object (created via u_list_new)
 *  \param item     the item that preceding the requested one
 *
 *  Example: iterate on a u_list
 *
 *  void *elem;
 *  for(elem = u_list_first(list); elem; elem = u_list_next(list, elem))
 *      ...
 * 
 *  \return \c the requested item or NULL if \c item is the last one
 */ 
void* u_list_next(u_list_t *list, void **it)
{
    u_list_item_t *item;

    dbg_return_if (list == NULL, NULL);
    dbg_return_if (it == NULL, NULL);
    dbg_return_if (*it == NULL, NULL);

    item = *it;

    *it = item = TAILQ_NEXT(item, np);

    if(item)
        return item->ptr;

    return NULL;
}

/**
 *  \brief  Delete an element given its position in the list
 *
 *  \param list    the parent list object (created via u_list_new)
 *  \param n       element position in the list
 *  \param pptr    element original pointer
 *
 *  \return \c 0 if \p ptr has been removed, \c ~0 if \p ptr was not found
 */ 
int u_list_del_n(u_list_t *list, size_t n, void **pptr)
{
    u_list_item_t *item = NULL;

    dbg_return_if (list == NULL, ~0);
    dbg_return_if (n >= list->count, ~0);

    TAILQ_FOREACH(item, &list->head, np)
    {
        if(n-- == 0)
            break;
    }

    if(pptr)
        *pptr = item->ptr;

    TAILQ_REMOVE(&list->head, item, np);

    list->count--;

    u_free(item);

    return 0;
}

/**
 *  \}
 */
