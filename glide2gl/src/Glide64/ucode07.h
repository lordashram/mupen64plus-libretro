/*
* Glide64 - Glide video plugin for Nintendo 64 emulators.
* Copyright (c) 2002  Dave2001
* Copyright (c) 2003-2009  Sergey 'Gonetz' Lipski
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//****************************************************************
//
// Glide64 - Glide Plugin for Nintendo 64 emulators
// Project started on December 29th, 2001
//
// Authors:
// Dave2001, original author, founded the project in 2001, left it in 2002
// Gugaman, joined the project in 2002, left it in 2002
// Sergey 'Gonetz' Lipski, joined the project in 2002, main author since fall of 2002
// Hiroshi 'KoolSmoky' Morii, joined the project in 2007
//
//****************************************************************
//
// To modify Glide64:
// * Write your name and (optional)email, commented by your work, so I know who did it, and so that you can find which parts you modified when it comes time to send it to me.
// * Do NOT send me the whole project or file that you modified.  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************
//
// Oct 2002 Created by Gonetz (Gonetz@ngs.ru)
// Info about this ucode is taken from TR64 OGL plugin. Thanks, Icepir8!
// Oct 2003 Modified by Gonetz (Gonetz@ngs.ru)
// Bugs fixed with help from glN64 sources. Thanks, Orkin!
//****************************************************************

uint32_t pd_col_addr = 0;

static void uc7_colorbase(uint32_t w0, uint32_t w1)
{
   LRDP("uc7_colorbase\n");
   pd_col_addr = segoffset(w1);
}

typedef struct 
{
   int16_t y;
   int16_t x;
   uint16_t idx;

   int16_t z;

   int16_t t;
   int16_t s;
} vtx_uc7;

static void uc7_vertex(uint32_t w0, uint32_t w1)
{
   uint32_t l, addr;
   uint32_t v0, i, n;
   float x, y, z;

   pre_update();

   addr = segoffset(w1);

   v0 = (w0 & 0x0F0000) >> 16;
   n = ((w0 & 0xF00000) >> 20) + 1;

   FRDP ("uc7:vertex n: %d, v0: %d, from: %08lx\n", n, v0, addr);

   vtx_uc7 *vertex = (vtx_uc7 *)&gfx.RDRAM[addr];

   for (i = 0; i < n; i++)
   {
      VERTEX *v = &rdp.vtx[v0 + i];
      x   = (float)vertex->x;
      y   = (float)vertex->y;
      z   = (float)vertex->z;
      v->flags  = 0;
      v->ou   = (float)vertex->s;
      v->ov   = (float)vertex->t;
      v->uv_scaled = 0;

      //    FRDP ("before: v%d - x: %f, y: %f, z: %f, flags: %04lx, ou: %f, ov: %f\n", i>>4, x, y, z, v->flags, v->ou, v->ov);

      v->x = x*rdp.combined[0][0] + y*rdp.combined[1][0] + z*rdp.combined[2][0] + rdp.combined[3][0];
      v->y = x*rdp.combined[0][1] + y*rdp.combined[1][1] + z*rdp.combined[2][1] + rdp.combined[3][1];
      v->z = x*rdp.combined[0][2] + y*rdp.combined[1][2] + z*rdp.combined[2][2] + rdp.combined[3][2];
      v->w = x*rdp.combined[0][3] + y*rdp.combined[1][3] + z*rdp.combined[2][3] + rdp.combined[3][3];


      if (fabs(v->w) < 0.001) v->w = 0.001f;
      v->oow = 1.0f / v->w;
      v->x_w = v->x * v->oow;
      v->y_w = v->y * v->oow;
      v->z_w = v->z * v->oow;

      v->uv_calculated = 0xFFFFFFFF;
      v->screen_translated = 0;

      v->scr_off = 0;
      if (v->x < -v->w) v->scr_off |= 1;
      if (v->x > v->w) v->scr_off |= 2;
      if (v->y < -v->w) v->scr_off |= 4;
      if (v->y > v->w) v->scr_off |= 8;
      if (v->w < 0.1f) v->scr_off |= 16;

      uint8_t *color = &gfx.RDRAM[pd_col_addr + (vertex->idx & 0xff)];

      v->a = color[0];
      CalculateFog (v);

      if (rdp.geom_mode & G_LIGHTING)
      {
         v->vec[0] = (int8_t)color[3];
         v->vec[1] = (int8_t)color[2];
         v->vec[2] = (int8_t)color[1];

         if (rdp.geom_mode & G_TEXTURE_GEN_LINEAR) 
            calc_linear(v);
         else if (rdp.geom_mode & G_TEXTURE_GEN) 
            calc_sphere(v);

         NormalizeVector (v->vec);

         calc_light (v);
      }
      else
      {
         v->r = color[3];
         v->g = color[2];
         v->b = color[1];
      }
      //FRDP ("v%d - x: %f, y: %f, z: %f, w: %f, u: %f, v: %f\n", i>>4, v->x, v->y, v->z, v->w, v->ou, v->ov);
      vertex++;
   }
}
