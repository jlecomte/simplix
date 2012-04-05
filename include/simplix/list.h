/*===========================================================================
 *
 * list.h
 *
 * Copyright (C) 2007 - Julien Lecomte
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *===========================================================================
 *
 * Set of macros to do basic manipulations on doubly linked circular lists.
 *
 *===========================================================================*/

#ifndef _LIST_H_
#define _LIST_H_

#define list_empty(list_head) \
    ((list_head) == NULL)

#define list_init_named(list_head, prev, next) \
do {                                           \
    (list_head)->prev = (list_head);           \
    (list_head)->next = (list_head);           \
} while (0)

#define list_insert_before_named(item, new, prev, next) \
do {                                                    \
    (new)->prev = (item)->prev;                         \
    (new)->next = (item);                               \
    (new)->prev->next = (new);                          \
    (new)->next->prev = (new);                          \
} while (0)

#define list_insert_after_named(item, new, prev, next) \
do {                                                   \
    (new)->prev = (item);                              \
    (new)->next = (item)->next;                        \
    (new)->prev->next = (new);                         \
    (new)->next->prev = (new);                         \
} while (0)

#define list_append_named(list_head, item, prev, next)     \
if (list_head) {                                           \
    list_insert_before_named(list_head, item, prev, next); \
} else {                                                   \
    (list_head) = (item);                                  \
    list_init_named(list_head, prev, next);                \
}

#define list_remove_named(list_head, item, prev, next) \
if ((item)->next == (item)) {                          \
    (list_head) = NULL;                                \
} else {                                               \
    if ((item) == (list_head))                         \
        (list_head) = (item)->next;                    \
    (item)->next->prev = (item)->prev;                 \
    (item)->prev->next = (item)->next;                 \
}

#define list_pop_head_named(list_head, prev, next) ({    \
    typeof(list_head) __ret = (list_head);               \
    list_remove_named(list_head, list_head, prev, next); \
    __ret;                                               \
})

#define list_replace_named(list_head, old, new, prev, next) \
do {                                                        \
    (new)->prev = (old)->prev;                              \
    (new)->next = (old)->next;                              \
    (new)->prev->next = (new);                              \
    (new)->next->prev = (new);                              \
    if ((old) == (list_head))                               \
        (list_head) = (new);                                \
} while (0)

#define list_for_each_named(list_head, item, index, prev, next)  \
    for ((item) = (list_head), (index) = 0;                      \
         (list_head) && ((item) != (list_head) || (index) == 0); \
         (item) = (item)->next, (index)++)

/* Similar to list_for_each, but allows for the current entry to be removed */
#define list_for_each_safe_named(list_head, item, next_item, index, prev, next) \
    for ((item) = (list_head), (next_item) = (item)->next, (index) = 0;         \
         (list_head) && ((item) != (list_head) || (index) == 0);                \
         (item) = (next_item), (next_item) = (item)->next, (index)++)

/* The above macros allow for a single structure to participate in several
   lists at a time by using different list pointer names. As a convenience,
   the following macros are provided. They assume that the list pointers are
   named prev and next, making the code a bit simpler, but a single
   structure cannot participate in more than one list at a time. */

#define list_init(list_head) \
    list_init_named(list_head, prev, next)

#define list_insert_before(item, new) \
    list_insert_before_named(item, new, prev, next)

#define list_insert_after(item, new) \
    list_insert_after_named(item, new, prev, next)

#define list_append(list_head, item) \
    list_append_named(list_head, item, prev, next)

#define list_remove(list_head, item) \
    list_remove_named(list_head, item, prev, next)

#define list_pop_head(list_head) \
    list_pop_head_named(list_head, prev, next);

#define list_replace(list_head, old, new) \
    list_replace_named(list_head, old, new, prev, next)

#define list_for_each(list_head, item, index) \
    list_for_each_named(list_head, item, index, prev, next)

#define list_for_each_safe(list_head, item, next_item, index) \
    list_for_each_safe_named(list_head, item, next_item, index, prev, next)

#endif /* _LIST_H_ */
