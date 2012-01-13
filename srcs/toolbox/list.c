/* 
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.  
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
    \defgroup list Lists
    \{
        The \ref list module implements a linked list.  Elements - actually
        element pointers - can be appended at the end of an ::u_list_t
        object (via ::u_list_add), or inserted at a given position (via 
        ::u_list_insert).  Element can be retrieved and deleted by index 
        (::u_list_get_n, ::u_list_del_n), and also evicted by direct reference
        (::u_list_del).  The ::u_list_foreach iterator is provided for safe,
        easy and efficient (forward) traversal of list objects. 

        \note   Element pointers are never owned by the ::u_list_t object to 
                which they are linked.  The disposal of the resources (possibly)
                allocated on the element is up to the user, e.g. when calling
                ::u_list_del_n, use the result-argument \p *pptr to pass the 
                "real" object to its dtor.
 */

/**
 *  \brief  Create a new list object
 *
 *  \param  plist   the newly created ::u_list_t object as a result argument
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */ 
int u_list_create (u_list_t **plist)
{
    u_list_t *list = NULL;

    list = u_zalloc(sizeof(u_list_t));
    dbg_err_sif (list == NULL); 

    TAILQ_INIT(&list->head);

    *plist = list;

    return 0;
err:
    if(list)
        u_free(list);
    return ~0;
}

/**
 *  \brief  Free a list object
 *
 *  Free the supplied ::u_list_t object \p list.  Note the list doesn't own 
 *  the pointers in it: the client must free them
 *
 *  \param  list    the ::u_list_t object that has to be disposed
 *
 *  \return nothing 
 */ 
void u_list_free (u_list_t *list)
{
    dbg_return_if(list == NULL, );

    u_list_clear(list);

    u_free(list);

    return;
}

/**
 *  \brief  Push an element to the list
 *
 *  Push the supplied pointer \p ptr to the ::u_list_t object \p list
 *
 *  \param  list    the parent ::u_list_t object (created via ::u_list_create)
 *  \param  ptr     the the reference to the element that has to be push'd
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */ 
int u_list_add (u_list_t *list, void *ptr)
{
    return u_list_insert(list, ptr, list->count);
}

/**
 *  \brief  Pop an element from the list
 *
 *  Evict the element referenced by \p ptr from the ::u_list_t object \p list
 *
 *  \param  list    the parent ::u_list_t object (created via ::u_list_create)
 *  \param  ptr     reference to the element that has to be evicted
 *
 *  \retval  0  if \p ptr has been successfully removed
 *  \retval ~0  if \p ptr was not found
 */ 
int u_list_del (u_list_t *list, void *ptr)
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
 *  Count the number of elements actually present in the supplied ::u_list_t 
 *  object \p list
 *
 *  \param  list    the ::u_list_t object that has to be queried
 *
 *  \return the number of elements in \p list
 */ 
size_t u_list_count (u_list_t *list)
{
    /* a SIGBUS/SIGSEGV is better than returning 0 if list == NULL */
    /* NOTE: perhaps we could use ssize_t instead, and return -1 on error */
    return list->count;
}

/**
 *  \brief  Get the n-th element in list
 *
 *  Get the element at index position \p n from the ::u_list_t object \p list
 *
 *  \param  list    an ::u_list_t object
 *  \param  n       the index of the element that we are supposed to retrieve
 *
 *  \return the pointer to the n-th element or \c NULL if no element was
 *          found at index \p n
 */ 
void *u_list_get_n (u_list_t *list, size_t n)
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
 *  Insert the supplied element \p ptr into the ::u_list_t object list at 
 *  index \p n
 *
 *  \param  list    the parent ::u_list_t object (created via ::u_list_create)
 *  \param  ptr     the element that has to be push'd
 *  \param  n       the position in the list (from zero to ::u_list_count)
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */ 
int u_list_insert (u_list_t *list, void *ptr, size_t n)
{
    u_list_item_t *prev, *item = NULL;

    dbg_return_if (list == NULL, ~0);
    dbg_return_if (n > list->count, ~0);

    item = u_zalloc(sizeof(u_list_item_t));
    dbg_err_sif (item == NULL);

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
 *  \brief  Return the first item of the list and initialize the iterator
 *
 *  Return the first item of the supplied ::u_list_t object and initialize the 
 *  opaque iterator \p it
 *
 *  \param  list    the ::u_list_t object (created via ::u_list_create)
 *  \param  it      opaque iterator object of type \c void**
 *
 *  \return the first item or \c NULL if the list is empty 
 */ 
void *u_list_first (u_list_t *list, void **it)
{
    u_list_item_t *item;

    dbg_return_if (list == NULL, NULL);

    item = TAILQ_FIRST(&list->head);

    if(it && item)
        *it = TAILQ_NEXT(item, np);
    else if(it)
        *it = NULL;

    if(item)
        return item->ptr;

    return NULL;
}

/**
 *  \brief  Return the next element while iterating over a list 
 *
 *  Return the next element while iterating over the supplied ::u_list_t
 *  object \p list.  The \p it iterator must have been already initialized
 *  via a previous call to ::u_list_first
 *
 *  \param  list    an ::u_list_t object created via ::u_list_create
 *  \param  it      opaque iterator already initialized with ::u_list_first
 *
 *  List iteration example:
 *  \code
    void *it;
    my_t *my;

    // indifferently one could have used: u_list_foreach (list, my, it) { ... }
    for (my = u_list_first(list, &it); my; my = u_list_next(list, &it))
        do_something_with(my);
    ...
 *  \endcode
 *
 *  \return the requested item or \c NULL if the last item in list was reached
 *
 *  \see u_list_foreach
 *  \see u_list_iforeach
 */ 
void *u_list_next (u_list_t *list, void **it)
{
    u_list_item_t *item;

    dbg_return_if (list == NULL, NULL);
    dbg_return_if (it == NULL, NULL);
    nop_return_if (*it == NULL, NULL);

    item = *it;

    if(it && item)
        *it = TAILQ_NEXT(item, np);
    else if(it)
        *it = NULL;

    if(item)
        return item->ptr;

    return NULL;
}

/**
 *  \brief  Delete an element given its position in the list
 *
 *  Delete the element at index \p n from the supplied ::u_list_t object 
 *  \p list and give back the reference to the stored object via the result
 *  argument \p pptr.  At a later stage \c *pptr can be appropriately destroyed.
 *
 *  \param  list    the parent ::u_list_t object (created via ::u_list_create)
 *  \param  n       element position in the list
 *  \param  pptr    element reference
 *
 *  \retval  0  if \p ptr has been removed
 *  \retval ~0  if \p ptr was not found
 */ 
int u_list_del_n (u_list_t *list, size_t n, void **pptr)
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
 *  \brief  Remove all elements from the list
 *
 *  Remove all elements from the supplied ::u_list_t object \p list.  Beware
 *  that the referenced objects - if any - won't be available anymore and 
 *  could be lost (usually causing memory leaking).
 *
 *  \param  list    the ::u_list_t object that must be reset
 *
 *  \retval  0  on success
 *  \retval ~0  on failure
 */ 
int u_list_clear (u_list_t *list)
{
    u_list_item_t *item;

    dbg_err_if(list == NULL);

    while((item = TAILQ_FIRST(&list->head)) != NULL)
    {
        TAILQ_REMOVE(&list->head, item, np);

        u_free(item);
    }

    return 0;
err:
    return ~0;
}

/**
 *  \}
 */
