/*
 * Open Surge Engine
 * langselect.c - language selection screen
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

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "langselect.h"
#include "options.h"
#include "../core/util.h"
#include "../core/scene.h"
#include "../core/assetfs.h"
#include "../core/stringutil.h"
#include "../core/logfile.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/audio.h"
#include "../core/lang.h"
#include "../core/input.h"
#include "../core/timer.h"
#include "../core/font.h"
#include "../core/prefs.h"
#include "../core/modmanager.h"
#include "../entities/actor.h"
#include "../entities/background.h"
#include "../entities/sfx.h"


/* private data */
#define LANG_BGFILE             "themes/scenes/langselect.bg"
#define LANG_MAXPERPAGE         7
typedef struct {
    char title[128];
    char author[128];
    char filepath[1024];
} lngdata_t;

static bool quit;
static int lngcount;
static font_t *title, **lngfnt[2], *page_label, *author_label;
static lngdata_t *lngdata;
static int option; /* current option: 0..n-1 */
static actor_t *arrow;
static input_t *input;
static float scene_time;
static bgtheme_t *bgtheme;
static music_t *music;
static bool fresh_install;
static bool came_from_options;

/* private functions */
static void save_preferences(const char *filepath);
static void load_lang_list();
static void unload_lang_list();
static int dirfill(const char *filename, void *param);
static int dircount(const char *filename, void *param);
static int sort_cmp(const void *a, const void *b);






/* public functions */

/*
 * langselect_init()
 * Initializes the scene
 */
void langselect_init(void *param)
{
    prefs_t* prefs = modmanager_prefs();

    option = 0;
    quit = false;
    scene_time = 0;
    fresh_install = !prefs_has_item(prefs, ".langpath");
    came_from_options = (param != NULL) && *((bool*)param);
    input = input_create_user(NULL);
    music = music_load(OPTIONS_MUSICFILE);

    page_label = font_create("menu.text");
    author_label = font_create("menu.text");

    title = font_create("menu.title");
    font_set_text(title, "%s", "<color=$COLOR_TITLE>SELECT YOUR\nLANGUAGE</color>");
    font_set_position(title, v2d_new(VIDEO_SCREEN_W/2, 5));
    font_set_align(title, FONTALIGN_CENTER);

    bgtheme = background_load(LANG_BGFILE);

    arrow = actor_create();
    actor_change_animation(arrow, sprite_get_animation("UI Pointer", 0));

    load_lang_list();
    if(lngcount <= 1) {
        if(came_from_options)
            video_showmessage("No translations are available!");
        scenestack_pop();
        return;
    }

    fadefx_in(color_rgb(0,0,0), 1.0);
}


/*
 * langselect_release()
 * Releases the scene
 */
void langselect_release()
{
    unload_lang_list();
    bgtheme = background_unload(bgtheme);

    actor_destroy(arrow);
    font_destroy(title);
    font_destroy(author_label);
    font_destroy(page_label);
    input_destroy(input);
    music_unref(music);
}


/*
 * langselect_update()
 * Updates the scene
 */
void langselect_update()
{
    float dt = timer_get_delta();
    int current_page, max_pages;
    v2d_t pos;

    scene_time += dt;

    /* background movement */
    background_update(bgtheme);

    /* menu option */
    arrow->position = font_get_position(lngfnt[0][option]);
    arrow->position.x += -20 + 5*cos(2*PI * scene_time);
    if(!quit && !fadefx_is_fading()) {
        if(input_button_pressed(input, IB_DOWN)) {
            option = (option+1)%lngcount;
            sound_play(SFX_CHOOSE);
        }
        if(input_button_pressed(input, IB_UP)) {
            option = (((option-1)%lngcount)+lngcount)%lngcount;
            sound_play(SFX_CHOOSE);
        }
        if(input_button_pressed(input, IB_FIRE1) || input_button_pressed(input, IB_FIRE3)) {
            char *filepath = lngdata[option].filepath;
            logfile_message("Loading language \"%s\", \"%s\"", lngdata[option].title, filepath);
            lang_loadfile(DEFAULT_LANGUAGE_FILEPATH); /* just in case of missing strings... */
            lang_loadfile(filepath);
            save_preferences(filepath);
            sound_play(SFX_CONFIRM);
            quit = true;
        }
        if(input_button_pressed(input, IB_FIRE4)) {
            sound_play(SFX_BACK);
            quit = true;
        }
    }

    /* page label */
    current_page = 1 + option / LANG_MAXPERPAGE;
    max_pages = 1 + max(0, lngcount - 1) / LANG_MAXPERPAGE;
    font_set_text(page_label, "page %d/%d", current_page, max_pages);
    pos.x = VIDEO_SCREEN_W - font_get_textsize(page_label).x - 10;
    pos.y = VIDEO_SCREEN_H - font_get_textsize(page_label).y - 5;
    font_set_position(page_label, pos);

    /* author label */
    font_set_text(author_label, "<color=$COLOR_HIGHLIGHT>Translation by:</color> %s", lngdata[option].author);
    pos.x = 10;
    pos.y = VIDEO_SCREEN_H - font_get_textsize(author_label).y - 5;
    font_set_position(author_label, pos);

    /* music */
    if(!music_is_playing() && !fresh_install)
        music_play(music, true);

    /* quit */
    if(quit) {
        if(fadefx_is_over()) {
            scenestack_pop();
            return;
        }
        fadefx_out(color_rgb(0,0,0), 1.0);
    }
}



/*
 * langselect_render()
 * Renders the scene
 */
void langselect_render()
{
    int i;
    v2d_t cam = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    background_render_bg(bgtheme, cam);
    background_render_fg(bgtheme, cam);

    font_render(title, cam);
    font_render(page_label, cam);
    font_render(author_label, cam);

    for(i=0; i<lngcount; i++) {
        if(i/LANG_MAXPERPAGE == option/LANG_MAXPERPAGE)
            font_render(lngfnt[option==i ? 1 : 0][i], cam);
    }

    actor_render(arrow, cam);
}






/* private methods */

/* saves the user preferences */
void save_preferences(const char *filepath)
{
    prefs_t* prefs = modmanager_prefs();
    prefs_set_string(prefs, ".langpath", filepath);
}

/* reads the language list from the languages/ folder */
void load_lang_list()
{
    int i, c = 0;
    logfile_message("load_lang_list()");

    /* loading language data */
    lngcount = 0;
    assetfs_foreach_file("languages", ".lng", dircount, (void*)&lngcount, true);

    /* fatal error */
    if(lngcount == 0)
        fatal_error("FATAL ERROR: no language files were found! Please reinstall the game.");
    else
        logfile_message("%d languages found.", lngcount);

    /* grabbing language data */
    lngdata = mallocx(lngcount * sizeof(lngdata_t));
    assetfs_foreach_file("languages", ".lng", dirfill, (void*)&c, true);
    qsort(lngdata, lngcount, sizeof(lngdata_t), sort_cmp);

    /* other stuff */
    lngfnt[0] = mallocx(lngcount * sizeof(font_t*));
    lngfnt[1] = mallocx(lngcount * sizeof(font_t*));
    for(i=0; i<lngcount; i++) {
        lngfnt[0][i] = font_create("menu.text");
        lngfnt[1][i] = font_create("menu.text");
        font_set_text(lngfnt[0][i], "%2d. %s", i+1, lngdata[i].title);
        font_set_text(lngfnt[1][i], "<color=$COLOR_HIGHLIGHT>% 2d. %s</color>", i+1, lngdata[i].title);
        font_set_position(lngfnt[0][i], v2d_new(25, 72 + 20*(i%LANG_MAXPERPAGE)));
        font_set_position(lngfnt[1][i], v2d_new(25, 72 + 20*(i%LANG_MAXPERPAGE)));
    }
}

int dirfill(const char *filename, void *param)
{
    int *c = (int*)param;
    int ver, subver, wipver;

    lang_readcompatibility(filename, &ver, &subver, &wipver);
    if(game_version_compare(ver, subver, wipver) >= 0) {
        str_cpy(lngdata[*c].filepath, filename, sizeof(lngdata[*c].filepath));
        lang_readstring(filename, "LANG_NAME", lngdata[*c].title, sizeof( lngdata[*c].title ));
        lang_readstring(filename, "LANG_AUTHOR", lngdata[*c].author, sizeof( lngdata[*c].author ));
        (*c)++;
    }
    if(game_version_compare(ver, subver, wipver) != 0)
        logfile_message("Warning: language file \"%s\" (compatibility: %d.%d.%d) may not be fully compatible with this version of the engine (%s)", filename, ver, subver, wipver, GAME_VERSION_STRING);

    return 0;
}

int dircount(const char *filename, void *param)
{
    int *lngcount = (int*)param;
    int ver, subver, wipver;

    lang_readcompatibility(filename, &ver, &subver, &wipver);
    if(game_version_compare(ver, subver, wipver) >= 0)
        (*lngcount)++;

    return 0;
}

/* unloads the language list */
void unload_lang_list()
{
    int i;

    logfile_message("unload_lang_list()");

    for(i=0; i<lngcount; i++) {
        font_destroy(lngfnt[0][i]);
        font_destroy(lngfnt[1][i]);
    }

    free(lngfnt[0]);
    free(lngfnt[1]);
    free(lngdata);
    lngcount = 0;
}


/* comparator */
int sort_cmp(const void *a, const void *b)
{
    lngdata_t *q[2] = { (lngdata_t*)a, (lngdata_t*)b };
    if(str_icmp(q[0]->title, "English") == 0) return -1;
    if(str_icmp(q[1]->title, "English") == 0) return 1;
    return str_icmp(q[0]->title, q[1]->title);
}
