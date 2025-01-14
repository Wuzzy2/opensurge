/*
 * Open Surge Engine
 * lang.c - language/translation module
 * Copyright (C) 2009-2010, 2019  Alexandre Martins <alemartf@gmail.com>
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

#include <stdio.h>
#include <ctype.h>
#include "lang.h"
#include "util.h"
#include "assetfs.h"
#include "stringutil.h"
#include "logfile.h"
#include "hashtable.h"
#include "nanoparser/nanoparser.h"

/* default language file */
const char* DEFAULT_LANGUAGE_FILEPATH = "languages/english.lng";

/* fake string type */
typedef struct { char *data; } stringadapter_t;
static stringadapter_t* stringadapter_create(const char *data);
static const char* stringadapter_get_data(const stringadapter_t *s);
static void stringadapter_set_data(stringadapter_t *s, const char *data);
static void stringadapter_destroy(stringadapter_t *s);

/* hash table */
HASHTABLE_GENERATE_CODE(stringadapter_t, stringadapter_destroy);
static HASHTABLE(stringadapter_t, strings);

/* private stuff */
typedef struct { const char *key; const char *value; } inout_t;
static int traverse(const parsetree_statement_t *stmt);
static int traverse_inout(const parsetree_statement_t *stmt, void *inout);


/*
 * lang_init()
 * Initializes the language module
 */
void lang_init()
{
    logfile_message("Initializing the language module");
    strings = hashtable_stringadapter_t_create();
    lang_loadfile(DEFAULT_LANGUAGE_FILEPATH);
    logfile_message("The language module has been initialized");
}


/*
 * lang_release()
 * Releases the language module
 */
void lang_release()
{
    logfile_message("Releasing the language module...");
    strings = hashtable_stringadapter_t_destroy(strings);
}


/*
 * lang_loadfile()
 * Loads a language definition file
 */
void lang_loadfile(const char *filepath)
{
    const char* fullpath;
    int ver, subver, wipver;
    parsetree_program_t *prog;

    logfile_message("Loading language file \"%s\"...", filepath);
    if(!assetfs_exists(filepath)) {
        if(0 != strcmp(filepath, DEFAULT_LANGUAGE_FILEPATH)) {
            logfile_message("File \"%s\" doesn't exist.", filepath);
            lang_loadfile(DEFAULT_LANGUAGE_FILEPATH);
            return;
        }
        else
            fatal_error("Missing default language file: \"%s\". Please reinstall the game.", DEFAULT_LANGUAGE_FILEPATH);
    }

    lang_readcompatibility(filepath, &ver, &subver, &wipver);
    if(game_version_compare(ver, subver, wipver) < 0) /* backwards compatibility */
        fatal_error("Language file \"%s\" (version %d.%d.%d) is not compatible with this version of the engine (%d.%d.%d)!", filepath, ver, subver, wipver, GAME_VERSION, GAME_SUB_VERSION, GAME_WIP_VERSION);

    fullpath = assetfs_fullpath(filepath);
    prog = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program(prog, traverse);
    prog = nanoparser_deconstruct_tree(prog);
}


/*
 * lang_readstring()
 * Reads the contents of the desired key directly from the
 * language file (without loading it into memory)
 */
void lang_readstring(const char *filepath, const char *desired_key, char *str, size_t str_size)
{
    inout_t param;
    parsetree_program_t *prog;
    const char* fullpath;

    fullpath = assetfs_fullpath(filepath);
    param.key = desired_key;
    param.value = NULL;

    prog = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program_ex(prog, (void*)(&param), traverse_inout);

    if(param.value == NULL)
        fatal_error("lang_readstring(\"%s\", \"%s\") failed", filepath, desired_key);
    else
        str_cpy(str, param.value, str_size);

    prog = nanoparser_deconstruct_tree(prog);
}


/*
 * lang_getstring()
 * Retrieves some string from the language definition file
 */
void lang_getstring(const char *desired_key, char *str, size_t str_size)
{
    const stringadapter_t *s = hashtable_stringadapter_t_find(strings, desired_key);

    if(s != NULL)
        str_cpy(str, stringadapter_get_data(s), str_size);
    else
        str_cpy(str, "null", str_size);
}



/*
 * lang_get()
 * Like lang_getstring(), but returns the string as a static char*
 */
const char *lang_get(const char *desired_key)
{
    static char buf[1024];
    lang_getstring(desired_key, buf, sizeof(buf));
    return buf;
}



/*
 * lang_readcompatibility()
 * Language files are made for specific game versions
 */
void lang_readcompatibility(const char *filename, int *ver, int *subver, int *wipver)
{
    char compat[32];
    lang_readstring(filename, "LANG_COMPATIBILITY", compat, sizeof(compat));
    if(sscanf(compat, "%d.%d.%d", ver, subver, wipver) < 3)
        *ver = *subver = *wipver = 0;
}


/*
 * lang_haskey()
 * Checks if a key exists
 */
bool lang_haskey(const char *desired_key)
{
    const stringadapter_t *s = hashtable_stringadapter_t_find(strings, desired_key);
    return s != NULL;
}


/* private stuff */

int traverse(const parsetree_statement_t *stmt)
{
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);
    const parsetree_parameter_t *p = nanoparser_get_nth_parameter(param_list, 1);
    const char *key, *value;
    stringadapter_t *s;

    if(nanoparser_get_number_of_parameters(param_list) != 1) 
        fatal_error("Language file error: invalid syntax at line %d in\n\"%s\"", nanoparser_get_line_number(stmt), nanoparser_get_file(stmt));

    nanoparser_expect_string(p, "a string is expected after each key of the language file");
    key = id;
    value = nanoparser_get_string(p);

    if(NULL == (s = hashtable_stringadapter_t_find(strings, key)))
        hashtable_stringadapter_t_add(strings, key, stringadapter_create(value));
    else
        stringadapter_set_data(s, value);

    return 0;
}

int traverse_inout(const parsetree_statement_t *stmt, void *inout)
{
    inout_t *x = (inout_t*)inout;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);
    const parsetree_parameter_t *p = nanoparser_get_nth_parameter(param_list, 1);

    if(nanoparser_get_number_of_parameters(param_list) != 1) 
        fatal_error("Language file error: invalid syntax at line %d in\n\"%s\"", nanoparser_get_line_number(stmt), nanoparser_get_file(stmt));

    nanoparser_expect_string(p, "a string is expected after each key of the language file");
    if(str_icmp(id, x->key) == 0) {
        x->value = nanoparser_get_string(p);
        return 1; /* stop the enumeration */
    }

    return 0;
}

stringadapter_t* stringadapter_create(const char *data)
{
    stringadapter_t *s = mallocx(sizeof *s);
    s->data = str_dup(data);
    return s;
}

void stringadapter_destroy(stringadapter_t *s)
{
    free(s->data);
    free(s);
}

const char* stringadapter_get_data(const stringadapter_t *s)
{
    return s->data;
}

void stringadapter_set_data(stringadapter_t *s, const char *data)
{
    free(s->data);
    s->data = str_dup(data);
}

