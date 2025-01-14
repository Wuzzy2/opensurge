/*
 * Open Surge Engine
 * gameover.c - "game over" scene
 * Copyright (C) 2010, 2018  Alexandre Martins <alemartf@gmail.com>
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

#include "gameover.h"
#include "quest.h"
#include "../core/font.h"
#include "../core/scene.h"
#include "../core/audio.h"
#include "../core/util.h"
#include "../core/fadefx.h"
#include "../core/color.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/timer.h"


/* private data */
#define GAMEOVER_TIMEOUT        7.0f
#define GAMEOVER_MUSICFILE      "musics/gameover.ogg"
static font_t *gameover_fnt[2];
static image_t *gameover_buf;
static float gameover_timer;
static music_t* music;


/*
 * gameover_init()
 * Initializes the game over screen
 */
void gameover_init(void *foo)
{
    gameover_timer = 0;

    gameover_fnt[0] = font_create("gameover");
    font_set_position(gameover_fnt[0], v2d_new(-64, 112));
    font_set_align(gameover_fnt[0], FONTALIGN_RIGHT);
    font_set_text(gameover_fnt[0], "GAME");

    gameover_fnt[1] = font_create("gameover");
    font_set_position(gameover_fnt[1], v2d_new(VIDEO_SCREEN_W+64, 112));
    font_set_align(gameover_fnt[1], FONTALIGN_LEFT);
    font_set_text(gameover_fnt[1], "OVER");

    gameover_buf = image_clone(video_get_backbuffer());

    music = music_load(GAMEOVER_MUSICFILE);
    music_play(music, false);
}



/*
 * gameover_update()
 * Updates the game over screen
 */
void gameover_update()
{
    float distance = 16.0f;
    float speed = VIDEO_SCREEN_W / 2.0f;
    float dt = timer_get_delta();
    v2d_t pos;

    /* timer */
    gameover_timer += dt;
    if(gameover_timer >= GAMEOVER_TIMEOUT) {
        if(fadefx_is_over()) {
            quest_abort();
            scenestack_pop();
            return;
        }
        fadefx_out(color_rgb(0,0,0), 2);
    }

    /* "game over" text */
    pos = font_get_position(gameover_fnt[0]);
    pos.x = min(pos.x + speed * dt, (VIDEO_SCREEN_W - distance) / 2.0f);
    font_set_position(gameover_fnt[0], pos);

    pos = font_get_position(gameover_fnt[1]);
    pos.x = max(pos.x - speed * dt, (VIDEO_SCREEN_W + distance) / 2.0f);
    font_set_position(gameover_fnt[1], pos);
}



/*
 * gameover_render()
 * Renders the game over screen
 */
void gameover_render()
{
    v2d_t v = v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2);

    image_blit(gameover_buf, 0, 0, 0, 0, image_width(gameover_buf), image_height(gameover_buf));
    font_render(gameover_fnt[0], v);
    font_render(gameover_fnt[1], v);
}



/*
 * gameover_release()
 * Releases the game over screen
 */
void gameover_release()
{
    music_stop();
    music_unref(music);
    image_destroy(gameover_buf);
    font_destroy(gameover_fnt[1]);
    font_destroy(gameover_fnt[0]);
    quest_abort();
}
