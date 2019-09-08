/*
 * Open Surge Engine
 * fasthash.h - a fast tiny hash table with integer keys and linear probing
 * Copyright (C) 2019  Alexandre Martins <alemartf@gmail.com>
 * http://opensurge2d.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <stdbool.h>

typedef struct fasthash_t fasthash_t;

fasthash_t* fasthash_create(void (*element_destructor)(void*));
fasthash_t* fasthash_destroy(fasthash_t* hashtable);
void* fasthash_get(fasthash_t* hashtable, uint32_t key);
void fasthash_put(fasthash_t* hashtable, uint32_t key, void* value);
bool fasthash_delete(fasthash_t* hashtable, uint32_t key);
void* fasthash_find(fasthash_t* hashtable, bool (*predicate)(void*,void*), void* data);