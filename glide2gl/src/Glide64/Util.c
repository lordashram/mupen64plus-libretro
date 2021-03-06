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

#include <math.h>
#include "Gfx_1.3.h"
#include "Util.h"
#include "Combine.h"
#include "3dmath.h"
#include "TexCache.h"
#include "DepthBufferRender.h"
#include "GBI.h"

extern int dzdx;
extern int deltaZ;
extern VERTEX **org_vtx;

typedef struct
{
   float d;
   float x;
   float y;
} LineEquationType;

typedef struct
{
   unsigned int	c2_m2b:2;
   unsigned int	c1_m2b:2;
   unsigned int	c2_m2a:2;
   unsigned int	c1_m2a:2;
   unsigned int	c2_m1b:2;
   unsigned int	c1_m1b:2;
   unsigned int	c2_m1a:2;
   unsigned int	c1_m1a:2;
} rdp_blender_setting;

#define interp2p(a, b, r)  (a + (b - a) * r)
#define interp3p(a, b, c, r1, r2) ((a)+(((b)+((c)-(b))*(r2))-(a))*(r1))
#define EvaLine(li, x, y) ((li->x) * (x) + (li->y) * (y) + (li->d))

//
// util_init - initialize data for the functions in this file
//

void util_init(void)
{
}

float ScaleZ(float z)
{
   if (settings.n64_z_scale)
   {
      int iz = (int)(z*8.0f+0.5f);
      if (iz < 0)
         iz = 0;
      else if (iz >= ZLUT_SIZE)
         iz = ZLUT_SIZE - 1;
      return (float)zLUT[iz];
   }
   if (z  < 0.0f)
      return 0.0f;
   z *= 1.9f;
   if (z > 65535.0f)
      return 65535.0f;
   return z;
}

void apply_shade_mods (VERTEX *v)
{
   float col[4];
   uint32_t mod;
   memcpy (col, rdp.col, 16);

   if (rdp.cmb_flags)
   {
      if (v->shade_mod == 0)
         v->color_backup = *(uint32_t*)(&(v->b));
      else
         *(uint32_t*)(&(v->b)) = v->color_backup;
      mod = rdp.cmb_flags;
      if (mod & CMB_SET)
      {
         if (col[0] > 1.0f) col[0] = 1.0f;
         if (col[1] > 1.0f) col[1] = 1.0f;
         if (col[2] > 1.0f) col[2] = 1.0f;
         if (col[0] < 0.0f) col[0] = 0.0f;
         if (col[1] < 0.0f) col[1] = 0.0f;
         if (col[2] < 0.0f) col[2] = 0.0f;
         v->r = (uint8_t)(255.0f * col[0]);
         v->g = (uint8_t)(255.0f * col[1]);
         v->b = (uint8_t)(255.0f * col[2]);
      }
      if (mod & CMB_A_SET)
      {
         if (col[3] > 1.0f) col[3] = 1.0f;
         if (col[3] < 0.0f) col[3] = 0.0f;
         v->a = (uint8_t)(255.0f * col[3]);
      }
      if (mod & CMB_SETSHADE_SHADEALPHA)
      {
         v->r = v->g = v->b = v->a;
      }
      if (mod & CMB_MULT_OWN_ALPHA)
      {
         float percent = v->a / 255.0f;
         v->r = (uint8_t)(v->r * percent);
         v->g = (uint8_t)(v->g * percent);
         v->b = (uint8_t)(v->b * percent);
      }
      if (mod & CMB_MULT)
      {
         if (col[0] > 1.0f) col[0] = 1.0f;
         if (col[1] > 1.0f) col[1] = 1.0f;
         if (col[2] > 1.0f) col[2] = 1.0f;
         if (col[0] < 0.0f) col[0] = 0.0f;
         if (col[1] < 0.0f) col[1] = 0.0f;
         if (col[2] < 0.0f) col[2] = 0.0f;
         v->r = (uint8_t)(v->r * col[0]);
         v->g = (uint8_t)(v->g * col[1]);
         v->b = (uint8_t)(v->b * col[2]);
      }
      if (mod & CMB_A_MULT)
      {
         if (col[3] > 1.0f) col[3] = 1.0f;
         if (col[3] < 0.0f) col[3] = 0.0f;
         v->a = (uint8_t)(v->a * col[3]);
      }
      if (mod & CMB_SUB)
      {
         int r = v->r - (int)(255.0f * rdp.coladd[0]);
         int g = v->g - (int)(255.0f * rdp.coladd[1]);
         int b = v->b - (int)(255.0f * rdp.coladd[2]);
         if (r < 0) r = 0;
         if (g < 0) g = 0;
         if (b < 0) b = 0;
         v->r = (uint8_t)r;
         v->g = (uint8_t)g;
         v->b = (uint8_t)b;
      }
      if (mod & CMB_A_SUB)
      {
         int a = v->a - (int)(255.0f * rdp.coladd[3]);
         if (a < 0) a = 0;
         v->a = (uint8_t)a;
      }
      if (mod & CMB_ADD)
      {
         int r = v->r + (int)(255.0f * rdp.coladd[0]);
         int g = v->g + (int)(255.0f * rdp.coladd[1]);
         int b = v->b + (int)(255.0f * rdp.coladd[2]);
         if (r > 255) r = 255;
         if (g > 255) g = 255;
         if (b > 255) b = 255;
         v->r = (uint8_t)r;
         v->g = (uint8_t)g;
         v->b = (uint8_t)b;
      }
      if (mod & CMB_A_ADD)
      {
         int a = v->a + (int)(255.0f * rdp.coladd[3]);
         if (a > 255) a = 255;
         v->a = (uint8_t)a;
      }
      if (mod & CMB_COL_SUB_OWN)
      {
         int r = (uint8_t)(255.0f * rdp.coladd[0]) - v->r;
         int g = (uint8_t)(255.0f * rdp.coladd[1]) - v->g;
         int b = (uint8_t)(255.0f * rdp.coladd[2]) - v->b;
         if (r < 0) r = 0;
         if (g < 0) g = 0;
         if (b < 0) b = 0;
         v->r = (uint8_t)r;
         v->g = (uint8_t)g;
         v->b = (uint8_t)b;
      }
      v->shade_mod = cmb.shade_mod_hash;
   }

   if (rdp.cmb_flags_2 & CMB_INTER)
   {
      v->r = (uint8_t)(rdp.col_2[0] * rdp.shade_factor * 255.0f + v->r * (1.0f - rdp.shade_factor));
      v->g = (uint8_t)(rdp.col_2[1] * rdp.shade_factor * 255.0f + v->g * (1.0f - rdp.shade_factor));
      v->b = (uint8_t)(rdp.col_2[2] * rdp.shade_factor * 255.0f + v->b * (1.0f - rdp.shade_factor));
      v->shade_mod = cmb.shade_mod_hash;
   }
}


static void Create1LineEq(LineEquationType *l, VERTEX *v1, VERTEX *v2, VERTEX *v3)
{
   float x, y;
   // Line between (x1,y1) to (x2,y2)
   l->x = v2->sy-v1->sy;
   l->y = v1->sx-v2->sx;
   l->d = -(l->x * v2->sx+ (l->y) * v2->sy);
   x = v3->sx;
   y = v3->sy;
   if (EvaLine(l,x,y) * v3->oow < 0)
   {
      l->x = -l->x;
      l->y = -l->y;
      l->d = -l->d;
   }
}

static void InterpolateColors3(VERTEX *v1, VERTEX *v2, VERTEX *v3, VERTEX *out)
{
   LineEquationType line;
   float aDot, bDot, scale1, tx, ty, s1, s2, den, w;
   Create1LineEq(&line, v2, v3, v1);

   aDot = (out->x * line.x + out->y * line.y);
   bDot = (v1->sx * line.x + v1->sy * line.y);
   scale1 = ( -line.d - aDot) / ( bDot - aDot );
   tx = out->x + scale1 * (v1->sx - out->x);
   ty = out->y + scale1 * (v1->sy - out->y);
   s1 = 101.0;
   s2 = 101.0;
   den = tx - v1->sx;

   if (fabs(den) > 1.0)
      s1 = (out->x-v1->sx)/den;
   if (s1 > 100.0f)
      s1 = (out->y-v1->sy)/(ty-v1->sy);

   den = v3->sx - v2->sx;
   if (fabs(den) > 1.0)
      s2 = (tx-v2->sx)/den;
   if (s2 > 100.0f)
      s2 =(ty-v2->sy)/(v3->sy-v2->sy);

   w = 1.0/interp3p(v1->oow,v2->oow,v3->oow,s1,s2);

   out->r = interp3p(v1->r*v1->oow,v2->r*v2->oow,v3->r*v3->oow,s1,s2)*w;
   out->g = interp3p(v1->g*v1->oow,v2->g*v2->oow,v3->g*v3->oow,s1,s2)*w;
   out->b = interp3p(v1->b*v1->oow,v2->b*v2->oow,v3->b*v3->oow,s1,s2)*w;
   out->a = interp3p(v1->a*v1->oow,v2->a*v2->oow,v3->a*v3->oow,s1,s2)*w;
   out->f = (float)(interp3p(v1->f*v1->oow,v2->f*v2->oow,v3->f*v3->oow,s1,s2)*w);
   /*
      out->u0 = interp3p(v1->u0_w*v1->oow,v2->u0_w*v2->oow,v3->u0_w*v3->oow,s1,s2)/oow;
      out->v0 = interp3p(v1->v0_w*v1->oow,v2->v0_w*v2->oow,v3->v0_w*v3->oow,s1,s2)/oow;
      out->u1 = interp3p(v1->u1_w*v1->oow,v2->u1_w*v2->oow,v3->u1_w*v3->oow,s1,s2)/oow;
      out->v1 = interp3p(v1->v1_w*v1->oow,v2->v1_w*v2->oow,v3->v1_w*v3->oow,s1,s2)/oow;
      */
}

//*/
//
// clip_w - clips aint the z-axis
//
//
static void clip_w (int interpolate_colors)
{
   int i, j, index, n;
   float percent;
   VERTEX *tmp;
   
   n = rdp.n_global;
   // Swap vertex buffers
   tmp = (VERTEX*)rdp.vtxbuf2;
   rdp.vtxbuf2 = rdp.vtxbuf;
   rdp.vtxbuf = tmp;
   rdp.vtx_buffer ^= 1;
   index = 0;

   // Check the vertices for clipping
   for (i=0; i<n; i++)
   {
      VERTEX *first, *second;
      j = i+1;
      if (j == 3)
         j = 0;
      first = (VERTEX*)&rdp.vtxbuf2[i];
      second = (VERTEX*)&rdp.vtxbuf2[j];

      if (first->w >= 0.01f)
      {
         if (second->w >= 0.01f)    // Both are in, save the last one
         {
            rdp.vtxbuf[index] = rdp.vtxbuf2[j];
            rdp.vtxbuf[index++].not_zclipped = 1;
         }
         else      // First is in, second is out, save intersection
         {
            percent = (-first->w) / (second->w - first->w);
            rdp.vtxbuf[index].not_zclipped = 0;
            rdp.vtxbuf[index].x = first->x + (second->x - first->x) * percent;
            rdp.vtxbuf[index].y = first->y + (second->y - first->y) * percent;
            rdp.vtxbuf[index].z = first->z + (second->z - first->z) * percent;
            rdp.vtxbuf[index].w = settings.depth_bias * 0.01f;
            rdp.vtxbuf[index].u0 = first->u0 + (second->u0 - first->u0) * percent;
            rdp.vtxbuf[index].v0 = first->v0 + (second->v0 - first->v0) * percent;
            rdp.vtxbuf[index].u1 = first->u1 + (second->u1 - first->u1) * percent;
            rdp.vtxbuf[index].v1 = first->v1 + (second->v1 - first->v1) * percent;
            if (interpolate_colors)
               gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, first, second);
            else
               rdp.vtxbuf[index++].number = first->number | second->number;
         }
      }
      else
      {
         if (second->w >= 0.01f)  // First is out, second is in, save intersection & in point
         {
            percent = (-second->w) / (first->w - second->w);
            rdp.vtxbuf[index].not_zclipped = 0;
            rdp.vtxbuf[index].x = second->x + (first->x - second->x) * percent;
            rdp.vtxbuf[index].y = second->y + (first->y - second->y) * percent;
            rdp.vtxbuf[index].z = second->z + (first->z - second->z) * percent;
            rdp.vtxbuf[index].w = settings.depth_bias * 0.01f;
            rdp.vtxbuf[index].u0 = second->u0 + (first->u0 - second->u0) * percent;
            rdp.vtxbuf[index].v0 = second->v0 + (first->v0 - second->v0) * percent;
            rdp.vtxbuf[index].u1 = second->u1 + (first->u1 - second->u1) * percent;
            rdp.vtxbuf[index].v1 = second->v1 + (first->v1 - second->v1) * percent;
            if (interpolate_colors)
               gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, second, first);
            else
               rdp.vtxbuf[index++].number = first->number | second->number;

            // Save the in point
            rdp.vtxbuf[index] = rdp.vtxbuf2[j];
            rdp.vtxbuf[index++].not_zclipped = 1;
         }
      }
   }
   rdp.n_global = index;
}


static void InterpolateColors2(VERTEX *va, VERTEX *vb, VERTEX *res, float percent)
{
   float w, ba, bb, ga, gb, ra, rb, aa, ab, fa, fb;
   w = 1.0f/(va->oow + (vb->oow-va->oow) * percent);
   //   res->oow = va->oow + (vb->oow-va->oow) * percent;
   //   res->q = res->oow;
   ba = va->b * va->oow;
   bb = vb->b * vb->oow;
   res->b = interp2p(ba, bb, percent) * w;
   ga = va->g * va->oow;
   gb = vb->g * vb->oow;
   res->g = interp2p(ga, gb, percent) * w;
   ra = va->r * va->oow;
   rb = vb->r * vb->oow;
   res->r = interp2p(ra, rb, percent) * w;
   aa = va->a * va->oow;
   ab = vb->a * vb->oow;
   res->a = interp2p(aa, ab, percent) * w;
   fa = va->f * va->oow;
   fb = vb->f * vb->oow;
   res->f = interp2p(fa, fb, percent) * w;
}

static void CalculateLODValues(VERTEX *v, int32_t i, int32_t j, float *lodFactor, float s_scale, float t_scale)
{
   float deltaS, deltaT, deltaTexels, deltaPixels, deltaX, deltaY;
   deltaS = (v[j].u0/v[j].q - v[i].u0/v[i].q) * s_scale;
   deltaT = (v[j].v0/v[j].q - v[i].v0/v[i].q) * t_scale;
   deltaTexels = squareRoot( deltaS * deltaS + deltaT * deltaT );

   deltaX = (v[j].x - v[i].x)/rdp.scale_x;
   deltaY = (v[j].y - v[i].y)/rdp.scale_y;
   deltaPixels = squareRoot( deltaX * deltaX + deltaY * deltaY );

   *lodFactor += deltaTexels / deltaPixels;
}

static void CalculateLOD(VERTEX *v, int n, uint32_t lodmode)
{
   float lodFactor, intptr, s_scale, t_scale, lod_fraction, detailmax;
   int i, j, ilod, lod_tile;
   s_scale = rdp.tiles[rdp.cur_tile].width / 255.0f;
   t_scale = rdp.tiles[rdp.cur_tile].height / 255.0f;
   lodFactor = 0;

   switch (lodmode)
   {
      case G_TL_TILE:
         for (i = 0; i < n; i++)
            CalculateLODValues(v, i, (i < n-1) ? i + 1 : 0, &lodFactor, s_scale, t_scale);
         // Divide by n (n edges) to find average
         lodFactor = lodFactor / n;
         break;
      case G_TL_LOD:
         CalculateLODValues(v, 0, 1, &lodFactor, s_scale, t_scale);
         break;
   }

   ilod = (int)lodFactor;
   lod_tile = min((int)(log10f((float)ilod)/log10f(2.0f)), rdp.cur_tile + rdp.mipmap_level);
   lod_fraction = 1.0f;

   if (lod_tile < rdp.cur_tile + rdp.mipmap_level)
      lod_fraction = max((float)modff(lodFactor / glide64_pow(2.,lod_tile),&intptr), rdp.prim_lodmin / 255.0f);

   if (cmb.dc0_detailmax < 0.5f)
      detailmax = lod_fraction;
   else
      detailmax = 1.0f - lod_fraction;

   grTexDetailControl (GR_TMU0, cmb.dc0_lodbias, cmb.dc0_detailscale, detailmax);
   grTexDetailControl (GR_TMU1, cmb.dc1_lodbias, cmb.dc1_detailscale, detailmax);
   //FRDP("CalculateLOD factor: %f, tile: %d, lod_fraction: %f\n", (float)lodFactor, lod_tile, lod_fraction);
}

static void DepthBuffer(VERTEX * vtx, int n)
{
   int i;
   if (fb_depth_render_enabled && dzdx && (rdp.flags & ZBUF_UPDATE))
   {
      struct vertexi v[12];
      if (rdp.u_cull_mode == 1) //cull front
      {
         for (i = 0; i < n; i++)
         {
            v[i].x = (int)((vtx[n-i-1].x-rdp.offset_x) / rdp.scale_x * 65536.0);
            v[i].y = (int)((vtx[n-i-1].y-rdp.offset_y) / rdp.scale_y * 65536.0);
            v[i].z = (int)(vtx[n-i-1].z * 65536.0);
         }
      }
      else
      {
         for (i = 0; i < n; i++)
         {
            v[i].x = (int)((vtx[i].x-rdp.offset_x) / rdp.scale_x * 65536.0);
            v[i].y = (int)((vtx[i].y-rdp.offset_y) / rdp.scale_y * 65536.0);
            v[i].z = (int)(vtx[i].z * 65536.0);
         }
      }
      Rasterize(v, n, dzdx);
   }

   for (i = 0; i < n; i++)
      vtx[i].z = ScaleZ(vtx[i].z);
}

static void clip_tri(int interpolate_colors)
{
   int i,j,index,n=rdp.n_global;
   float percent;

   // Check which clipping is needed
   if (rdp.clip & CLIP_XMAX) // right of the screen
   {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         VERTEX *first, *second;
         j = i+1;
         if (j == n)
            j = 0;
         first = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->x <= rdp.clip_max_x)
         {
            if (second->x <= rdp.clip_max_x)   // Both are in, save the last one
            {
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
            else      // First is in, second is out, save intersection
            {
               percent = (rdp.clip_max_x - first->x) / (second->x - first->x);
               rdp.vtxbuf[index].x = rdp.clip_max_x;
               rdp.vtxbuf[index].y = first->y + (second->y - first->y) * percent;
               rdp.vtxbuf[index].z = first->z + (second->z - first->z) * percent;
               rdp.vtxbuf[index].q = first->q + (second->q - first->q) * percent;
               rdp.vtxbuf[index].u0 = first->u0 + (second->u0 - first->u0) * percent;
               rdp.vtxbuf[index].v0 = first->v0 + (second->v0 - first->v0) * percent;
               rdp.vtxbuf[index].u1 = first->u1 + (second->u1 - first->u1) * percent;
               rdp.vtxbuf[index].v1 = first->v1 + (second->v1 - first->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, first, second);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number | 8;
            }
         }
         else
         {
            if (second->x <= rdp.clip_max_x) // First is out, second is in, save intersection & in point
            {
               percent = (rdp.clip_max_x - second->x) / (first->x - second->x);
               rdp.vtxbuf[index].x = rdp.clip_max_x;
               rdp.vtxbuf[index].y = second->y + (first->y - second->y) * percent;
               rdp.vtxbuf[index].z = second->z + (first->z - second->z) * percent;
               rdp.vtxbuf[index].q = second->q + (first->q - second->q) * percent;
               rdp.vtxbuf[index].u0 = second->u0 + (first->u0 - second->u0) * percent;
               rdp.vtxbuf[index].v0 = second->v0 + (first->v0 - second->v0) * percent;
               rdp.vtxbuf[index].u1 = second->u1 + (first->u1 - second->u1) * percent;
               rdp.vtxbuf[index].v1 = second->v1 + (first->v1 - second->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, second, first);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number | 8;

               // Save the in point
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
         }
      }
      n = index;
   }

   if (rdp.clip & CLIP_XMIN) // left of the screen
   {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         VERTEX *first, *second;
         j = i+1;
         if (j == n)
            j = 0;
         first = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->x >= rdp.clip_min_x)
         {
            if (second->x >= rdp.clip_min_x)   // Both are in, save the last one
            {
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
            else      // First is in, second is out, save intersection
            {
               percent = (rdp.clip_min_x - first->x) / (second->x - first->x);
               rdp.vtxbuf[index].x = rdp.clip_min_x;
               rdp.vtxbuf[index].y = first->y + (second->y - first->y) * percent;
               rdp.vtxbuf[index].z = first->z + (second->z - first->z) * percent;
               rdp.vtxbuf[index].q = first->q + (second->q - first->q) * percent;
               rdp.vtxbuf[index].u0 = first->u0 + (second->u0 - first->u0) * percent;
               rdp.vtxbuf[index].v0 = first->v0 + (second->v0 - first->v0) * percent;
               rdp.vtxbuf[index].u1 = first->u1 + (second->u1 - first->u1) * percent;
               rdp.vtxbuf[index].v1 = first->v1 + (second->v1 - first->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, first, second);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number | 8;
            }
         }
         else
         {
            if (rdp.vtxbuf2[j].x >= rdp.clip_min_x) // First is out, second is in, save intersection & in point
            {
               percent = (rdp.clip_min_x - second->x) / (first->x - second->x);
               rdp.vtxbuf[index].x = rdp.clip_min_x;
               rdp.vtxbuf[index].y = second->y + (first->y - second->y) * percent;
               rdp.vtxbuf[index].z = second->z + (first->z - second->z) * percent;
               rdp.vtxbuf[index].q = second->q + (first->q - second->q) * percent;
               rdp.vtxbuf[index].u0 = second->u0 + (first->u0 - second->u0) * percent;
               rdp.vtxbuf[index].v0 = second->v0 + (first->v0 - second->v0) * percent;
               rdp.vtxbuf[index].u1 = second->u1 + (first->u1 - second->u1) * percent;
               rdp.vtxbuf[index].v1 = second->v1 + (first->v1 - second->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, second, first);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number | 8;

               // Save the in point
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
         }
      }
      n = index;
   }

   if (rdp.clip & CLIP_YMAX) // top of the screen
   {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         VERTEX *first, *second;
         j = i+1;
         if (j == n)
            j = 0;
         first = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->y <= rdp.clip_max_y)
         {
            if (second->y <= rdp.clip_max_y)   // Both are in, save the last one
            {
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
            else      // First is in, second is out, save intersection
            {
               percent = (rdp.clip_max_y - first->y) / (second->y - first->y);
               rdp.vtxbuf[index].x = first->x + (second->x - first->x) * percent;
               rdp.vtxbuf[index].y = rdp.clip_max_y;
               rdp.vtxbuf[index].z = first->z + (second->z - first->z) * percent;
               rdp.vtxbuf[index].q = first->q + (second->q - first->q) * percent;
               rdp.vtxbuf[index].u0 = first->u0 + (second->u0 - first->u0) * percent;
               rdp.vtxbuf[index].v0 = first->v0 + (second->v0 - first->v0) * percent;
               rdp.vtxbuf[index].u1 = first->u1 + (second->u1 - first->u1) * percent;
               rdp.vtxbuf[index].v1 = first->v1 + (second->v1 - first->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, first, second);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number | 16;
            }
         }
         else
         {
            if (second->y <= rdp.clip_max_y) // First is out, second is in, save intersection & in point
            {
               percent = (rdp.clip_max_y - second->y) / (first->y - second->y);
               rdp.vtxbuf[index].x = rdp.vtxbuf2[j].x + (rdp.vtxbuf2[i].x - second->x) * percent;
               rdp.vtxbuf[index].y = rdp.clip_max_y;
               rdp.vtxbuf[index].z = second->z + (first->z - second->z) * percent;
               rdp.vtxbuf[index].q = second->q + (first->q - second->q) * percent;
               rdp.vtxbuf[index].u0 = second->u0 + (first->u0 - second->u0) * percent;
               rdp.vtxbuf[index].v0 = second->v0 + (first->v0 - second->v0) * percent;
               rdp.vtxbuf[index].u1 = second->u1 + (first->u1 - second->u1) * percent;
               rdp.vtxbuf[index].v1 = second->v1 + (first->v1 - second->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, second, first);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number | 16;

               // Save the in point
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
         }
      }
      n = index;
   }

   if (rdp.clip & CLIP_YMIN) // bottom of the screen
   {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         VERTEX *first, *second;
         j = i+1;
         if (j == n)
            j = 0;
         first = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->y >= rdp.clip_min_y)
         {
            if (second->y >= rdp.clip_min_y)   // Both are in, save the last one
            {
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
            else      // First is in, second is out, save intersection
            {
               percent = (rdp.clip_min_y - first->y) / (second->y - first->y);
               rdp.vtxbuf[index].x = first->x + (second->x - first->x) * percent;
               rdp.vtxbuf[index].y = rdp.clip_min_y;
               rdp.vtxbuf[index].z = first->z + (second->z - first->z) * percent;
               rdp.vtxbuf[index].q = first->q + (second->q - first->q) * percent;
               rdp.vtxbuf[index].u0 = first->u0 + (second->u0 - first->u0) * percent;
               rdp.vtxbuf[index].v0 = first->v0 + (second->v0 - first->v0) * percent;
               rdp.vtxbuf[index].u1 = first->u1 + (second->u1 - first->u1) * percent;
               rdp.vtxbuf[index].v1 = first->v1 + (second->v1 - first->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, first, second);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number | 16;
            }
         }
         else
         {
            if (second->y >= rdp.clip_min_y) // First is out, second is in, save intersection & in point
            {
               percent = (rdp.clip_min_y - second->y) / (first->y - second->y);
               rdp.vtxbuf[index].x = second->x + (first->x - second->x) * percent;
               rdp.vtxbuf[index].y = rdp.clip_min_y;
               rdp.vtxbuf[index].z = second->z + (first->z - second->z) * percent;
               rdp.vtxbuf[index].q = second->q + (first->q - second->q) * percent;
               rdp.vtxbuf[index].u0 = second->u0 + (first->u0 - second->u0) * percent;
               rdp.vtxbuf[index].v0 = second->v0 + (first->v0 - second->v0) * percent;
               rdp.vtxbuf[index].u1 = second->u1 + (first->u1 - second->u1) * percent;
               rdp.vtxbuf[index].v1 = second->v1 + (first->v1 - second->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, second, first);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number | 16;

               // Save the in point
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
         }
      }
      n = index;
   }

   if (rdp.clip & CLIP_ZMAX) // far plane
   {
      // Swap vertex buffers
      VERTEX *tmp;
      tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;
      float maxZ = rdp.view_trans[2] + rdp.view_scale[2];

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         VERTEX *first, *second;
         j = i+1;
         if (j == n)
            j = 0;
         first = (VERTEX*)&rdp.vtxbuf2[i];
         second = (VERTEX*)&rdp.vtxbuf2[j];

         if (first->z < maxZ)
         {
            if (second->z < maxZ)   // Both are in, save the last one
            {
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
            else      // First is in, second is out, save intersection
            {
               percent = (maxZ - first->z) / (second->z - first->z);
               rdp.vtxbuf[index].x = first->x + (second->x - first->x) * percent;
               rdp.vtxbuf[index].y = first->y + (second->y - first->y) * percent;
               rdp.vtxbuf[index].z = maxZ - 0.001f;
               rdp.vtxbuf[index].q = first->q + (second->q - first->q) * percent;
               rdp.vtxbuf[index].u0 = first->u0 + (second->u0 - first->u0) * percent;
               rdp.vtxbuf[index].v0 = first->v0 + (second->v0 - first->v0) * percent;
               rdp.vtxbuf[index].u1 = first->u1 + (second->u1 - first->u1) * percent;
               rdp.vtxbuf[index].v1 = first->v1 + (second->v1 - first->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, first, second);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number;
            }
         }
         else
         {
            if (second->z < maxZ) // First is out, second is in, save intersection & in point
            {
               percent = (maxZ - second->z) / (first->z - second->z);
               rdp.vtxbuf[index].x = second->x + (first->x - second->x) * percent;
               rdp.vtxbuf[index].y = second->y + (first->y - second->y) * percent;
               rdp.vtxbuf[index].z = maxZ - 0.001f;;
               rdp.vtxbuf[index].q = second->q + (first->q - second->q) * percent;
               rdp.vtxbuf[index].u0 = second->u0 + (first->u0 - second->u0) * percent;
               rdp.vtxbuf[index].v0 = second->v0 + (first->v0 - second->v0) * percent;
               rdp.vtxbuf[index].u1 = second->u1 + (first->u1 - second->u1) * percent;
               rdp.vtxbuf[index].v1 = second->v1 + (first->v1 - second->v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, second, first);
               else
                  rdp.vtxbuf[index++].number = first->number | second->number;

               // Save the in point
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
         }
      }
      n = index;
   }

#if 0
   if (rdp.clip & CLIP_ZMIN) // near Z
   {
      // Swap vertex buffers
      VERTEX *tmp = rdp.vtxbuf2;
      rdp.vtxbuf2 = rdp.vtxbuf;
      rdp.vtxbuf = tmp;
      rdp.vtx_buffer ^= 1;
      index = 0;

      // Check the vertices for clipping
      for (i=0; i<n; i++)
      {
         j = i+1;
         if (j == n) j = 0;

         if (rdp.vtxbuf2[i].z >= 0.0f)
         {
            if (rdp.vtxbuf2[j].z >= 0.0f)   // Both are in, save the last one
            {
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
            else      // First is in, second is out, save intersection
            {
               percent = (-rdp.vtxbuf2[i].z) / (rdp.vtxbuf2[j].z - rdp.vtxbuf2[i].z);
               rdp.vtxbuf[index].x = rdp.vtxbuf2[i].x + (rdp.vtxbuf2[j].x - rdp.vtxbuf2[i].x) * percent;
               rdp.vtxbuf[index].y = rdp.vtxbuf2[i].y + (rdp.vtxbuf2[j].y - rdp.vtxbuf2[i].y) * percent;
               rdp.vtxbuf[index].z = 0.0f;
               rdp.vtxbuf[index].q = rdp.vtxbuf2[i].q + (rdp.vtxbuf2[j].q - rdp.vtxbuf2[i].q) * percent;
               rdp.vtxbuf[index].u0 = rdp.vtxbuf2[i].u0 + (rdp.vtxbuf2[j].u0 - rdp.vtxbuf2[i].u0) * percent;
               rdp.vtxbuf[index].v0 = rdp.vtxbuf2[i].v0 + (rdp.vtxbuf2[j].v0 - rdp.vtxbuf2[i].v0) * percent;
               rdp.vtxbuf[index].u1 = rdp.vtxbuf2[i].u1 + (rdp.vtxbuf2[j].u1 - rdp.vtxbuf2[i].u1) * percent;
               rdp.vtxbuf[index].v1 = rdp.vtxbuf2[i].v1 + (rdp.vtxbuf2[j].v1 - rdp.vtxbuf2[i].v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, &rdp.vtxbuf2[i], &rdp.vtxbuf2[j]);
               else
                  rdp.vtxbuf[index++].number = rdp.vtxbuf2[i].number | rdp.vtxbuf2[j].number;
            }
         }
         else
         {
            //if (rdp.vtxbuf2[j].z < 0.0f)  // Both are out, save nothing
            if (rdp.vtxbuf2[j].z >= 0.0f) // First is out, second is in, save intersection & in point
            {
               percent = (-rdp.vtxbuf2[j].z) / (rdp.vtxbuf2[i].z - rdp.vtxbuf2[j].z);
               rdp.vtxbuf[index].x = rdp.vtxbuf2[j].x + (rdp.vtxbuf2[i].x - rdp.vtxbuf2[j].x) * percent;
               rdp.vtxbuf[index].y = rdp.vtxbuf2[j].y + (rdp.vtxbuf2[i].y - rdp.vtxbuf2[j].y) * percent;
               rdp.vtxbuf[index].z = 0.0f;;
               rdp.vtxbuf[index].q = rdp.vtxbuf2[j].q + (rdp.vtxbuf2[i].q - rdp.vtxbuf2[j].q) * percent;
               rdp.vtxbuf[index].u0 = rdp.vtxbuf2[j].u0 + (rdp.vtxbuf2[i].u0 - rdp.vtxbuf2[j].u0) * percent;
               rdp.vtxbuf[index].v0 = rdp.vtxbuf2[j].v0 + (rdp.vtxbuf2[i].v0 - rdp.vtxbuf2[j].v0) * percent;
               rdp.vtxbuf[index].u1 = rdp.vtxbuf2[j].u1 + (rdp.vtxbuf2[i].u1 - rdp.vtxbuf2[j].u1) * percent;
               rdp.vtxbuf[index].v1 = rdp.vtxbuf2[j].v1 + (rdp.vtxbuf2[i].v1 - rdp.vtxbuf2[j].v1) * percent;
               if (interpolate_colors)
                  gSPInterpolateVertex(&rdp.vtxbuf[index++], percent, &rdp.vtxbuf2[j], &rdp.vtxbuf2[i]);
               else
                  rdp.vtxbuf[index++].number = rdp.vtxbuf2[i].number | rdp.vtxbuf2[j].number;

               // Save the in point
               rdp.vtxbuf[index++] = rdp.vtxbuf2[j];
            }
         }
      }
      n = index;
   }
#endif

   rdp.n_global = n;
}

static void render_tri (uint16_t linew, int old_interpolate)
{
   int i, j, n;
   if (rdp.clip)
      clip_tri(old_interpolate);
   n = rdp.n_global;

   if (n < 3)
   {
      FRDP (" * render_tri: n < 3\n");
      return;
   }

   //*
   if ((rdp.clip & CLIP_ZMIN) && (rdp.othermode_l & G_OBJLT_TLUT))
   {

      int to_render = false;
      for (i = 0; i < n; i++)
      {
         if (rdp.vtxbuf[i].z >= 0.0f)
         {
            to_render = true;
            break;
         }
      }
      if (!to_render) //all z < 0
      {
         FRDP (" * render_tri: all z < 0\n");
         return;
      }
   }
   //*/
   if (rdp.clip && !old_interpolate)
   {
      for (i = 0; i < n; i++)
      {
         float percent = 101.0f;
         VERTEX * v1 = 0,  * v2 = 0;
         switch (rdp.vtxbuf[i].number&7)
         {
            case 1:
            case 2:
            case 4:
               continue;
               break;
            case 3:
               v1 = org_vtx[0];
               v2 = org_vtx[1];
               break;
            case 5:
               v1 = org_vtx[0];
               v2 = org_vtx[2];
               break;
            case 6:
               v1 = org_vtx[1];
               v2 = org_vtx[2];
               break;
            case 7:
               InterpolateColors3(org_vtx[0], org_vtx[1], org_vtx[2], &rdp.vtxbuf[i]);
               continue;
               break;
         }
         switch (rdp.vtxbuf[i].number&24)
         {
            case 8:
               percent = (rdp.vtxbuf[i].x-v1->sx)/(v2->sx-v1->sx);
               break;
            case 16:
               percent = (rdp.vtxbuf[i].y-v1->sy)/(v2->sy-v1->sy);
               break;
            default:
               {
                  float d = (v2->sx-v1->sx);
                  if (fabs(d) > 1.0)
                     percent = (rdp.vtxbuf[i].x-v1->sx)/d;
                  if (percent > 100.0f)
                     percent = (rdp.vtxbuf[i].y-v1->sy)/(v2->sy-v1->sy);
               }
         }
         InterpolateColors2(v1, v2, &rdp.vtxbuf[i], percent);
      }
   }

   ConvertCoordsConvert (rdp.vtxbuf, n);

   float fog;

   switch (rdp.fog_mode)
   {
      case FOG_MODE_ENABLED:
         for (i = 0; i < n; i++)
            rdp.vtxbuf[i].f = 1.0f/max(4.0f, rdp.vtxbuf[i].f);
         break;
      case FOG_MODE_BLEND:
         fog = 1.0f/max(1, rdp.fog_color&0xFF);
         for (i = 0; i < n; i++)
            rdp.vtxbuf[i].f = fog;
         break;
      case FOG_MODE_BLEND_INVERSE:
         fog = 1.0f/max(1, (~rdp.fog_color)&0xFF);
         for (i = 0; i < n; i++)
            rdp.vtxbuf[i].f = fog;
         break;
   }

   if (settings.lodmode && rdp.cur_tile < rdp.mipmap_level)
      CalculateLOD(rdp.vtxbuf, n, settings.lodmode);

   cmb.cmb_ext_use = cmb.tex_cmb_ext_use = 0;

   /*
      if (rdp.tbuff_tex)
      {
      for (int k = 0; k < 3; k++)
      {
      FRDP("v%d %f->%f, width: %d. height: %d, tex_width: %d, tex_height: %d, lr_u: %f, lr_v: %f\n", k, vv0[k], pv[k]->v1, rdp.tbuff_tex->width, rdp.tbuff_tex->height, rdp.tbuff_tex->tex_width, rdp.tbuff_tex->tex_height, rdp.tbuff_tex->lr_u, rdp.tbuff_tex->lr_v);
      }
      }
      */
   {

      //      VERTEX ** pv = rdp.vtx_buffer? &rdp.vtx2 : &rdp.vtx1;
      //      for (int k = 0; k < n; k ++)
      //			FRDP ("DRAW[%d]: v.x = %f, v.y = %f, v.z = %f, v.u = %f, v.v = %f\n", k, pv[k]->x, pv[k]->y, pv[k]->z, pv[k]->coord[rdp.t0<<1], pv[k]->coord[(rdp.t0<<1)+1]);
      //        pv[k]->y = settings.res_y - pv[k]->y;

      if (linew > 0)
      {
         VERTEX *V0 = &rdp.vtxbuf[0];
         VERTEX *V1 = &rdp.vtxbuf[1];
         if (fabs(V0->x - V1->x) < 0.01 && fabs(V0->y - V1->y) < 0.01)
            V1 = &rdp.vtxbuf[2];
         V0->z = ScaleZ(V0->z);
         V1->z = ScaleZ(V1->z);
         VERTEX v[4];
         v[0] = *V0;
         v[1] = *V0;
         v[2] = *V1;
         v[3] = *V1;
         float width = linew * 0.25f;
         if (fabs(V0->y - V1->y) < 0.0001)
         {
            v[0].x = v[1].x = V0->x;
            v[2].x = v[3].x = V1->x;

            width *= rdp.scale_y;
            v[0].y = v[2].y = V0->y - width;
            v[1].y = v[3].y = V0->y + width;
         }
         else if (fabs(V0->x - V1->x) < 0.0001)
         {
            v[0].y = v[1].y = V0->y;
            v[2].y = v[3].y = V1->y;

            width *= rdp.scale_x;
            v[0].x = v[2].x = V0->x - width;
            v[1].x = v[3].x = V0->x + width;
         }
         else
         {
            float dx = V1->x - V0->x;
            float dy = V1->y - V0->y;
            float len = squareRoot(dx*dx + dy*dy);
            float wx = dy * width * rdp.scale_x / len;
            float wy = dx * width * rdp.scale_y / len;
            v[0].x = V0->x + wx;
            v[0].y = V0->y - wy;
            v[1].x = V0->x - wx;
            v[1].y = V0->y + wy;
            v[2].x = V1->x + wx;
            v[2].y = V1->y - wy;
            v[3].x = V1->x - wx;
            v[3].y = V1->y + wy;
         }
         grDrawTriangle(&v[0], &v[1], &v[2]);
         grDrawTriangle(&v[1], &v[2], &v[3]);
      }
      else
      {
         DepthBuffer(rdp.vtxbuf, n);
         if ((rdp.rm & 0xC10) == 0xC10)
            grDepthBiasLevel (-deltaZ);
         grDrawVertexArray (GR_TRIANGLE_FAN, n, rdp.vtx_buffer? &rdp.vtx2 : &rdp.vtx1);
      }
   }
}

void do_triangle_stuff (uint16_t linew, int old_interpolate) // what else?? do the triangle stuff :P (to keep from writing code twice)
{
   int i;

   if (rdp.clip & CLIP_WMIN)
      clip_w (old_interpolate);

   float maxZ = (rdp.zsrc != 1) ? rdp.view_trans[2] + rdp.view_scale[2] : rdp.prim_depth;

   uint8_t no_clip = 2;
   for (i=0; i<rdp.n_global; i++)
   {
      if (rdp.vtxbuf[i].not_zclipped)// && rdp.zsrc != 1)
      {
         //FRDP (" * NOT ZCLIPPPED: %d\n", rdp.vtxbuf[i].number);
         rdp.vtxbuf[i].x = rdp.vtxbuf[i].sx;
         rdp.vtxbuf[i].y = rdp.vtxbuf[i].sy;
         rdp.vtxbuf[i].z = rdp.vtxbuf[i].sz;
         rdp.vtxbuf[i].q = rdp.vtxbuf[i].oow;
         rdp.vtxbuf[i].u0 = rdp.vtxbuf[i].u0_w;
         rdp.vtxbuf[i].v0 = rdp.vtxbuf[i].v0_w;
         rdp.vtxbuf[i].u1 = rdp.vtxbuf[i].u1_w;
         rdp.vtxbuf[i].v1 = rdp.vtxbuf[i].v1_w;
      }
      else
      {
         //FRDP (" * ZCLIPPED: %d\n", rdp.vtxbuf[i].number);
         rdp.vtxbuf[i].q = 1.0f / rdp.vtxbuf[i].w;
         rdp.vtxbuf[i].x = rdp.view_trans[0] + rdp.vtxbuf[i].x * rdp.vtxbuf[i].q * rdp.view_scale[0] + rdp.offset_x;
         rdp.vtxbuf[i].y = rdp.view_trans[1] + rdp.vtxbuf[i].y * rdp.vtxbuf[i].q * rdp.view_scale[1] + rdp.offset_y;
         rdp.vtxbuf[i].z = rdp.view_trans[2] + rdp.vtxbuf[i].z * rdp.vtxbuf[i].q * rdp.view_scale[2];
         if (rdp.tex >= 1)
         {
            rdp.vtxbuf[i].u0 *= rdp.vtxbuf[i].q;
            rdp.vtxbuf[i].v0 *= rdp.vtxbuf[i].q;
         }
         if (rdp.tex >= 2)
         {
            rdp.vtxbuf[i].u1 *= rdp.vtxbuf[i].q;
            rdp.vtxbuf[i].v1 *= rdp.vtxbuf[i].q;
         }
      }

      if (rdp.zsrc == 1)
         rdp.vtxbuf[i].z = rdp.prim_depth;

      glide64SPClipVertex(i);
      // Don't remove clipping, or it will freeze
      if (rdp.vtxbuf[i].z > maxZ)           rdp.clip |= CLIP_ZMAX;
      if (rdp.vtxbuf[i].z < 0.0f)           rdp.clip |= CLIP_ZMIN;
      no_clip &= rdp.vtxbuf[i].screen_translated;
   }
   if (no_clip)
      rdp.clip = 0;
   else
   {
      if (!settings.clip_zmin)
         rdp.clip &= ~CLIP_ZMIN;
      if (!settings.clip_zmax)
         rdp.clip &= ~CLIP_ZMAX;
   }
   render_tri (linew, old_interpolate);
}

void do_triangle_stuff_2 (uint16_t linew)
{
   int i;
   rdp.clip = 0;

   for (i = 0; i < rdp.n_global; i++)
      glide64SPClipVertex(i);

   render_tri (linew, true);
}

void add_tri(VERTEX *v, int n, int type)
{
   //FRDP ("ENTER (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n", v[0].x, v[0].y, v[0].w,
   //  v[1].x, v[1].y, v[1].w, v[2].x, v[2].y, v[2].w);
}

void update_scissor(void)
{
   if (!(rdp.update & UPDATE_SCISSOR))
      return;

   // KILL the floating point error with 0.01f
   rdp.scissor.ul_x = (uint32_t)max(min((rdp.scissor_o.ul_x * rdp.scale_x + rdp.offset_x + 0.01f),settings.res_x),0);
   rdp.scissor.lr_x = (uint32_t)max(min((rdp.scissor_o.lr_x * rdp.scale_x + rdp.offset_x + 0.01f),settings.res_x),0);
   rdp.scissor.ul_y = (uint32_t)max(min((rdp.scissor_o.ul_y * rdp.scale_y + rdp.offset_y + 0.01f),settings.res_y),0);
   rdp.scissor.lr_y = (uint32_t)max(min((rdp.scissor_o.lr_y * rdp.scale_y + rdp.offset_y + 0.01f),settings.res_y),0);
   //grClipWindow specifies the hardware clipping window. Any pixels outside the clipping window are rejected.
   //Values are inclusive for minimum x and y values and exclusive for maximum x and y values.
   grClipWindow (rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);

   rdp.update ^= UPDATE_SCISSOR;
   //FRDP (" |- scissor - (%d, %d) -> (%d, %d)\n", rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);
}

//
// update - update states if they need it
//
void update(void)
{
   LRDP ("-+ update called\n");

   // Check for rendermode changes
   // Z buffer
   if (rdp.render_mode_changed & 0x00000C30)
   {
      FRDP (" |- render_mode_changed zbuf - decal: %s, update: %s, compare: %s\n",
            str_yn[(rdp.othermode_l & G_CULL_BACK)?1:0],
            str_yn[(rdp.othermode_l & UPDATE_BIASLEVEL)?1:0],
            str_yn[(rdp.othermode_l & ALPHA_COMPARE)?1:0]);

      rdp.render_mode_changed &= ~0x00000C30;
      rdp.update |= UPDATE_ZBUF_ENABLED;

      // Update?
      if ((rdp.othermode_l & 0x00000020))
         rdp.flags |= ZBUF_UPDATE;
      else
         rdp.flags &= ~ZBUF_UPDATE;

      // Compare?
      if (rdp.othermode_l & ALPHA_COMPARE)
         rdp.flags |= ZBUF_COMPARE;
      else
         rdp.flags &= ~ZBUF_COMPARE;
   }

   // Alpha compare
   if (rdp.render_mode_changed & CULL_FRONT)
   {
      FRDP (" |- render_mode_changed alpha compare - on: %s\n",
            str_yn[(rdp.othermode_l & CULL_FRONT)?1:0]);
      rdp.render_mode_changed &= ~CULL_FRONT;
      rdp.update |= UPDATE_ALPHA_COMPARE;

      if (rdp.othermode_l & CULL_FRONT)
         rdp.flags |= ALPHA_COMPARE;
      else
         rdp.flags &= ~ALPHA_COMPARE;
   }

   if (rdp.render_mode_changed & CULL_BACK) // alpha cvg sel
   {
      FRDP (" |- render_mode_changed alpha cvg sel - on: %s\n",
            str_yn[(rdp.othermode_l & CULL_BACK)?1:0]);
      rdp.render_mode_changed &= ~CULL_BACK;
      rdp.update |= UPDATE_COMBINE;
      rdp.update |= UPDATE_ALPHA_COMPARE;
   }

   // Force blend
   if (rdp.render_mode_changed & 0xFFFF0000)
   {
      FRDP (" |- render_mode_changed force_blend - %08lx\n", rdp.othermode_l&0xFFFF0000);
      rdp.render_mode_changed &= 0x0000FFFF;

      rdp.fbl_a0 = (uint8_t)((rdp.othermode_l>>30)&0x3);
      rdp.fbl_b0 = (uint8_t)((rdp.othermode_l>>26)&0x3);
      rdp.fbl_c0 = (uint8_t)((rdp.othermode_l>>22)&0x3);
      rdp.fbl_d0 = (uint8_t)((rdp.othermode_l>>18)&0x3);
      rdp.fbl_a1 = (uint8_t)((rdp.othermode_l>>28)&0x3);
      rdp.fbl_b1 = (uint8_t)((rdp.othermode_l>>24)&0x3);
      rdp.fbl_c1 = (uint8_t)((rdp.othermode_l>>20)&0x3);
      rdp.fbl_d1 = (uint8_t)((rdp.othermode_l>>16)&0x3);

      rdp.update |= UPDATE_COMBINE;
   }

   // Combine MUST go before texture
   if ((rdp.update & UPDATE_COMBINE) && rdp.allow_combine)
   {
#ifdef HAVE_HWFBE
      TBUFF_COLOR_IMAGE * aTBuff[2] = {0, 0};
      if (rdp.aTBuffTex[0])
         aTBuff[rdp.aTBuffTex[0]->tile] = rdp.aTBuffTex[0];
      if (rdp.aTBuffTex[1])
         aTBuff[rdp.aTBuffTex[1]->tile] = rdp.aTBuffTex[1];
      rdp.aTBuffTex[0] = aTBuff[0];
      rdp.aTBuffTex[1] = aTBuff[1];
#endif

      LRDP (" |-+ update_combine\n");
      Combine ();
   }

   if (rdp.update & UPDATE_TEXTURE)  // note: UPDATE_TEXTURE and UPDATE_COMBINE are the same
   {
      rdp.tex_ctr ++;
      if (rdp.tex_ctr == 0xFFFFFFFF)
         rdp.tex_ctr = 0;

      TexCache ();
      if (rdp.noise == NOISE_MODE_NONE)
         rdp.update ^= UPDATE_TEXTURE;
   }

   {
      // Z buffer
      if (rdp.update & UPDATE_ZBUF_ENABLED)
      {
         rdp.update ^= UPDATE_ZBUF_ENABLED;

         if (((rdp.flags & ZBUF_ENABLED) || rdp.zsrc == 1) && rdp.cycle_mode < G_CYC_COPY)
         {
            if (rdp.flags & ZBUF_COMPARE)
            {
               switch ((rdp.rm & ZMODE_DECAL) >> 10)
               {
                  case 0:
                     grDepthBiasLevel(0);
                     grDepthBufferFunction (settings.zmode_compare_less ? GR_CMP_LESS : GR_CMP_LEQUAL);
                     break;
                  case 1:
                     grDepthBiasLevel(-4);
                     grDepthBufferFunction (settings.zmode_compare_less ? GR_CMP_LESS : GR_CMP_LEQUAL);
                     break;
                  case 2:
                     grDepthBiasLevel(settings.ucode == 7 ? -4 : 0);
                     grDepthBufferFunction (GR_CMP_LESS);
                     break;
                  case 3:
                     // will be set dynamically per polygon
                     //grDepthBiasLevel(-deltaZ);
                     grDepthBufferFunction (GR_CMP_LEQUAL);
                     break;
               }
            }
            else
            {
               grDepthBiasLevel(0);
               grDepthBufferFunction (GR_CMP_ALWAYS);
            }

            if (rdp.flags & ZBUF_UPDATE)
               grDepthMask (FXTRUE);
            else
               grDepthMask (FXFALSE);
         }
         else
         {
            grDepthBiasLevel(0);
            grDepthBufferFunction (GR_CMP_ALWAYS);
            grDepthMask (FXFALSE);
         }
      }

      // Alpha compare
      if (rdp.update & UPDATE_ALPHA_COMPARE)
      {
         rdp.update ^= UPDATE_ALPHA_COMPARE;

         //	  if (rdp.acmp == 1 && !(rdp.othermode_l & CULL_BACK) && !force_full_alpha)
         //      if (rdp.acmp == 1 && !(rdp.othermode_l & CULL_BACK) && (rdp.blend_color&0xFF))
         if (rdp.acmp == 1 && !(rdp.othermode_l & CULL_BACK) && (!(rdp.othermode_l & G_CULL_BACK) || (rdp.blend_color&0xFF)))
         {
            uint8_t reference = (uint8_t)(rdp.blend_color&0xFF);
            grAlphaTestFunction (reference ? GR_CMP_GEQUAL : GR_CMP_GREATER);
            grAlphaTestReferenceValue (reference);
            FRDP (" |- alpha compare: blend: %02lx\n", reference);
         }
         else
         {
            if (rdp.flags & ALPHA_COMPARE)
            {
               if ((rdp.othermode_l & 0x5000) != 0x5000)
               {
                  grAlphaTestFunction (GR_CMP_GEQUAL);
                  grAlphaTestReferenceValue (0x20);//0xA0);
                  LRDP (" |- alpha compare: 0x20\n");
               }
               else
               {
                  grAlphaTestFunction (GR_CMP_GREATER);
                  if (rdp.acmp == 3)
                  {
                     grAlphaTestReferenceValue ((uint8_t)(rdp.blend_color&0xFF));
                     FRDP (" |- alpha compare: blend: %02lx\n", rdp.blend_color&0xFF);
                  }
                  else
                  {
                     grAlphaTestReferenceValue (0x00);
                     LRDP (" |- alpha compare: 0x00\n");
                  }
               }
            }
            else
            {
               grAlphaTestFunction (GR_CMP_ALWAYS);
               LRDP (" |- alpha compare: none\n");
            }
         }
         if (rdp.acmp == 3 && rdp.cycle_mode < G_CYC_COPY)
         {
            if (settings.old_style_adither || rdp.alpha_dither_mode != 3)
            {
               LRDP (" |- alpha compare: dither\n");
               grStippleMode(settings.stipple_mode);
            }
            else
               grStippleMode(GR_STIPPLE_DISABLE);
         }
         else
         {
            //LRDP (" |- alpha compare: dither disabled\n");
            grStippleMode(GR_STIPPLE_DISABLE);
         }
      }

      // Cull mode (leave this in for z-clipped triangles)
      if (rdp.update & UPDATE_CULL_MODE)
      {
         rdp.update ^= UPDATE_CULL_MODE;
         uint32_t mode = (rdp.flags & CULLMASK) >> CULLSHIFT;
         FRDP (" |- cull_mode - mode: %s\n", str_cull[mode]);
         switch (mode)
         {
            case 0: // cull none
            case 3: // cull both
               grCullMode(GR_CULL_DISABLE);
               break;
            case 1: // cull front
               //        grCullMode(GR_CULL_POSITIVE);
               grCullMode(GR_CULL_NEGATIVE);
               break;
            case 2: // cull back
               //        grCullMode (GR_CULL_NEGATIVE);
               grCullMode (GR_CULL_POSITIVE);
               break;
         }
      }

      //Added by Gonetz.
      if (settings.fog && (rdp.update & UPDATE_FOG_ENABLED))
      {
         rdp.update ^= UPDATE_FOG_ENABLED;

         uint16_t blender = (uint16_t)(rdp.othermode_l >> 16);
         if (rdp.flags & FOG_ENABLED)
         {
            rdp_blender_setting *bl = (rdp_blender_setting*)(&blender);
            if((rdp.fog_multiplier > 0) && (bl->c1_m1a==3 || bl->c1_m2a == 3 || bl->c2_m1a == 3 || bl->c2_m2a == 3))
            {
               grFogColorValue(rdp.fog_color);
               grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
               rdp.fog_mode = FOG_MODE_ENABLED;
               LRDP("fog enabled \n");
            }
            else
            {
               LRDP("fog disabled in blender\n");
               rdp.fog_mode = FOG_MODE_DISABLED;
               grFogMode (GR_FOG_DISABLE);
            }
         }
         else if (blender == 0xc410 || blender == 0xc411 || blender == 0xf500)
         {
            grFogColorValue(rdp.fog_color);
            grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
            rdp.fog_mode = FOG_MODE_BLEND;
            LRDP("fog blend \n");
         }
         else if (blender == 0x04d1)
         {
            grFogColorValue(rdp.fog_color);
            grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
            rdp.fog_mode = FOG_MODE_BLEND_INVERSE;
            LRDP("fog blend \n");
         }
         else
         {
            LRDP("fog disabled\n");
            rdp.fog_mode = FOG_MODE_DISABLED;
            grFogMode (GR_FOG_DISABLE);
         }
      }
   }

   if (rdp.update & UPDATE_VIEWPORT)
   {
      rdp.update ^= UPDATE_VIEWPORT;
      {
         float scale_x = (float)fabs(rdp.view_scale[0]);
         float scale_y = (float)fabs(rdp.view_scale[1]);

         rdp.clip_min_x = max((rdp.view_trans[0] - scale_x + rdp.offset_x) / rdp.clip_ratio, 0.0f);
         rdp.clip_min_y = max((rdp.view_trans[1] - scale_y + rdp.offset_y) / rdp.clip_ratio, 0.0f);
         rdp.clip_max_x = min((rdp.view_trans[0] + scale_x + rdp.offset_x) * rdp.clip_ratio, settings.res_x);
         rdp.clip_max_y = min((rdp.view_trans[1] + scale_y + rdp.offset_y) * rdp.clip_ratio, settings.res_y);

         FRDP (" |- viewport - (%d, %d, %d, %d)\n", (uint32_t)rdp.clip_min_x, (uint32_t)rdp.clip_min_y, (uint32_t)rdp.clip_max_x, (uint32_t)rdp.clip_max_y);
         if (!rdp.scissor_set)
         {
            rdp.scissor.ul_x = (uint32_t)rdp.clip_min_x;
            rdp.scissor.lr_x = (uint32_t)rdp.clip_max_x;
            rdp.scissor.ul_y = (uint32_t)rdp.clip_min_y;
            rdp.scissor.lr_y = (uint32_t)rdp.clip_max_y;
            grClipWindow (rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);
         }
      }
   }

   update_scissor ();

   LRDP (" + update end\n");
}
