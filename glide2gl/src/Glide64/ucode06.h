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
// * Do NOT send me the whole project or file that you modified->  Take out your modified code sections, and tell me where to put them.  If people sent the whole thing, I would have many different versions, but no idea how to combine them all.
//
//****************************************************************

// STANDARD DRAWIMAGE - draws a 2d image based on the following structure

static float set_sprite_combine_mode(void)
{
  if (rdp.cycle_mode == G_CYC_COPY)
  {
    rdp.tex = 1;
    rdp.allow_combine = 0;
    // Now actually combine !
    GrCombineFunction_t color_source = GR_COMBINE_FUNCTION_LOCAL;
#ifdef HAVE_HWFBE
    if (rdp.tbuff_tex && rdp.tbuff_tex->info.format == GR_TEXFMT_ALPHA_INTENSITY_88)
		color_source = GR_COMBINE_FUNCTION_LOCAL_ALPHA;
#endif
    cmb.tmu1_func = cmb.tmu0_func = color_source;
    cmb.tmu1_fac = cmb.tmu0_fac = GR_COMBINE_FACTOR_NONE;
    cmb.tmu1_a_func = cmb.tmu0_a_func = GR_COMBINE_FUNCTION_LOCAL;
    cmb.tmu1_a_fac = cmb.tmu0_a_fac = GR_COMBINE_FACTOR_NONE;
    cmb.tmu1_invert = cmb.tmu0_invert = FXFALSE;
    cmb.tmu1_a_invert = cmb.tmu0_a_invert = FXFALSE;
  }

  rdp.update |= UPDATE_COMBINE;
  update ();

  rdp.allow_combine = 1;

  // set z buffer mode
  float Z = 0.0f;
  if ((rdp.othermode_l & 0x00000030) && rdp.cycle_mode < G_CYC_COPY)
  {
    if (rdp.zsrc == 1)
    {
      Z = rdp.prim_depth;
    }
    FRDP ("prim_depth = %d, prim_dz = %d\n", rdp.prim_depth, rdp.prim_dz);
    Z = ScaleZ(Z);

    if (rdp.othermode_l & 0x00000400)
      grDepthBiasLevel(rdp.prim_dz);
  }
  else
  {
    LRDP("z compare not used, using 0\n");
  }

  grCullMode (GR_CULL_DISABLE);
  grFogMode (GR_FOG_DISABLE);
  rdp.update |= UPDATE_CULL_MODE | UPDATE_FOG_ENABLED;

  if (rdp.cycle_mode == G_CYC_COPY)
  {
    grColorCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
      GR_COMBINE_FACTOR_ONE,
      GR_COMBINE_LOCAL_NONE,
      GR_COMBINE_OTHER_TEXTURE,
      FXFALSE);
    grAlphaCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
      GR_COMBINE_FACTOR_ONE,
      GR_COMBINE_LOCAL_NONE,
      GR_COMBINE_OTHER_TEXTURE,
      FXFALSE);
    grAlphaBlendFunction (GR_BLEND_ONE,
      GR_BLEND_ZERO,
      GR_BLEND_ZERO,
      GR_BLEND_ZERO);
    if (rdp.othermode_l & 1)
    {
      grAlphaTestFunction (GR_CMP_GEQUAL);
      grAlphaTestReferenceValue (0x80);
    }
    else
      grAlphaTestFunction (GR_CMP_ALWAYS);
    rdp.update |= UPDATE_ALPHA_COMPARE | UPDATE_COMBINE;
  }
  return Z;
}

typedef struct DRAWIMAGE_t {
  float frameX;
  float frameY;
  uint16_t frameW;
  uint16_t frameH;
  uint16_t imageX;
  uint16_t imageY;
  uint16_t imageW;
  uint16_t imageH;
  uint32_t imagePtr;
  uint8_t imageFmt;
  uint8_t imageSiz;
  uint16_t imagePal;
  uint8_t flipX;
  uint8_t flipY;
  float scaleX;
  float scaleY;
} DRAWIMAGE;

#ifdef HAVE_HWFBE
static void DrawHiresDepthImage (const DRAWIMAGE *d)
{
   int w, h;
   uint16_t * src = (uint16_t*)(gfx.RDRAM + d->imagePtr);
   uint16_t image[512*512];
   uint16_t *dst = image;
   for (h = 0; h < d->imageH; h++)
   {
      for (w = 0; w < d->imageW; w++)
         *(dst++) = src[(w + h * d->imageW) ^ 1];
      dst += (512 - d->imageW);
   }
   GrTexInfo t_info;
   t_info.format = GR_TEXFMT_RGB_565;
   t_info.data = image;
   t_info.smallLodLog2 = GR_LOD_LOG2_512;
   t_info.largeLodLog2 = GR_LOD_LOG2_512;
   t_info.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;

   grTexDownloadMipMap (rdp.texbufs[1].tmu,
         rdp.texbufs[1].begin,
         GR_MIPMAPLEVELMASK_BOTH,
         &t_info);
   grTexSource (rdp.texbufs[1].tmu,
         rdp.texbufs[1].begin,
         GR_MIPMAPLEVELMASK_BOTH,
         &t_info);
   grTexCombine( GR_TMU1,
         GR_COMBINE_FUNCTION_LOCAL,
         GR_COMBINE_FACTOR_NONE,
         GR_COMBINE_FUNCTION_LOCAL,
         GR_COMBINE_FACTOR_NONE,
         FXFALSE,
         FXFALSE );
   grTexCombine( GR_TMU0,
         GR_COMBINE_FUNCTION_SCALE_OTHER,
         GR_COMBINE_FACTOR_ONE,
         GR_COMBINE_FUNCTION_SCALE_OTHER,
         GR_COMBINE_FACTOR_ONE,
         FXFALSE,
         FXFALSE );
   grColorCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
         GR_COMBINE_FACTOR_ONE,
         GR_COMBINE_LOCAL_NONE,
         GR_COMBINE_OTHER_TEXTURE,
         FXFALSE);
   grAlphaCombine (GR_COMBINE_FUNCTION_SCALE_OTHER,
         GR_COMBINE_FACTOR_ONE,
         GR_COMBINE_LOCAL_NONE,
         GR_COMBINE_OTHER_TEXTURE,
         FXFALSE);
   grAlphaBlendFunction (GR_BLEND_ONE,
         GR_BLEND_ZERO,
         GR_BLEND_ONE,
         GR_BLEND_ZERO);
   grDepthBufferFunction (GR_CMP_ALWAYS);
   grDepthMask (FXFALSE);

   GrLOD_t LOD = GR_LOD_LOG2_1024;
   if (settings.scr_res_x > 1024)
      LOD = GR_LOD_LOG2_2048;

   float lr_x = (float)d->imageW * rdp.scale_x;
   float lr_y = (float)d->imageH * rdp.scale_y;
   float lr_u = (float)d->imageW * 0.5f;// - 0.5f;
   float lr_v = (float)d->imageH * 0.5f;// - 0.5f;
   int i;
   VERTEX v[4] = {
      { 0, 0, 1.0f, 1.0f, 0, 0, 0, 0 },
      { lr_x, 0, 1.0f, 1.0f, lr_u, 0, lr_u, 0 },
      { 0, lr_y, 1.0f, 1.0f, 0, lr_v, 0, lr_v },
      { lr_x, lr_y, 1.0f, 1.0f, lr_u, lr_v, lr_u, lr_v }
   };
   AddOffset(v, 4);
   for (i = 0; i < 4; i++)
   {
      v[i].uc(0) = v[i].uc(1) = v[i].u0;
      v[i].vc(0) = v[i].vc(1) = v[i].v0;
   }
   grTextureBufferExt( rdp.texbufs[0].tmu, rdp.texbufs[0].begin, LOD, LOD,
         GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565, GR_MIPMAPLEVELMASK_BOTH );
   grRenderBuffer( GR_BUFFER_TEXTUREBUFFER_EXT );
   grAuxBufferExt( GR_BUFFER_AUXBUFFER );
   grSstOrigin(GR_ORIGIN_UPPER_LEFT);
   grBufferClear (0, 0, 0xFFFF);
   grDrawTriangle(&v[0], &v[2], &v[1]);
   grDrawTriangle(&v[2], &v[3], &v[1]);
   grRenderBuffer( GR_BUFFER_BACKBUFFER );
   grTextureAuxBufferExt( rdp.texbufs[0].tmu, rdp.texbufs[0].begin, LOD, LOD,
         GR_ASPECT_LOG2_1x1, GR_TEXFMT_RGB_565, GR_MIPMAPLEVELMASK_BOTH );
   grAuxBufferExt( GR_BUFFER_TEXTUREAUXBUFFER_EXT );
   grDepthMask (FXTRUE);
}
#endif

static void DrawDepthImage (const DRAWIMAGE *d)
{
   float scale_x_src, scale_y_src, scale_x_dst, scale_y_dst;
   uint16_t *src, *dst;
   int32_t x, y, src_width, src_height, dst_width, dst_height;

   if (!fb_depth_render_enabled || d->imageH > d->imageW)
      return;

#ifdef HAVE_HWFBE
   if (fb_hwfbe_enabled)
   {
      DrawHiresDepthImage(d);
      return;
   }
#endif

   scale_x_dst = rdp.scale_x;
   scale_y_dst = rdp.scale_y;
   scale_x_src = 1.0f/rdp.scale_x;
   scale_y_src = 1.0f/rdp.scale_y;
   src_width = d->imageW;
   src_height = d->imageH;
   dst_width = min((int)(src_width*scale_x_dst), (int)settings.scr_res_x);
   dst_height = min((int)(src_height*scale_y_dst), (int)settings.scr_res_y);
   src = (uint16_t*)(gfx.RDRAM+d->imagePtr);
   dst = (uint16_t*)malloc(dst_width * dst_height * sizeof(uint16_t));

   for (y = 0; y < dst_height; y++)
   {
      for (x = 0; x < dst_width; x++)
         dst[x + y * dst_width] = src[((int)(x * scale_x_src) + (int)(y*scale_y_src) * src_width)^1];
   }
   grLfbWriteRegion(GR_BUFFER_AUXBUFFER,
         0,
         0,
         GR_LFB_SRC_FMT_ZA16,
         dst_width,
         dst_height,
         FXFALSE,
         dst_width<<1,
         dst);
   free(dst);

   //LRDP("Depth image write\n");
}

static void DrawImage (DRAWIMAGE *d)
{
   if (d->imageW == 0 || d->imageH == 0 || d->frameH == 0)
      return;

   int x_size, y_size, x_shift, y_shift, line;
   // choose optimum size for the format/size
   switch (d->imageSiz)
   {
      case 0:
         if (rdp.tlut_mode < 2)
         {
            y_size = 64;
            y_shift = 6;
         }
         else
         {
            y_size = 32;
            y_shift = 5;
         }
         x_size = 128;
         x_shift = 7;
         line = 8;
         break;
      case 1:
         if (rdp.tlut_mode < 2)
         {
            y_size = 64;
            y_shift = 6;
         }
         else
         {
            y_size = 32;
            y_shift = 5;
         }
         x_size = 64;
         x_shift = 6;
         line = 8;
         break;
      case 2:
         x_size = 64;
         y_size = 32;
         x_shift = 6;
         y_shift = 5;
         line = 16;
         break;
      case 3:
         x_size = 32;
         y_size = 16;
         x_shift = 4;
         y_shift = 3;
         line = 16;
         break;
      default:
         FRDP("DrawImage. unknown image size: %d\n", d->imageSiz);
         return;
   }

   if (rdp.ci_width == 512 && !no_dlist) //RE2
   {
      uint16_t width = (uint16_t)(*gfx.VI_WIDTH_REG & 0xFFF);
      d->frameH = d->imageH = (d->frameW*d->frameH)/width;
      d->frameW = d->imageW = width;
      if (rdp.zimg == rdp.cimg)
      {
         DrawDepthImage(d);
         rdp.update |= UPDATE_ZBUF_ENABLED | UPDATE_COMBINE |
            UPDATE_ALPHA_COMPARE | UPDATE_VIEWPORT;
         return;
      }
   }

   if ((settings.hacks&hack_PPL) > 0)
   {
      if (d->imageY > d->imageH)
         d->imageY = (d->imageY%d->imageH);
   }
   else if ((settings.hacks&hack_Starcraft) > 0)
   {
      if (d->imageH%2 == 1)
         d->imageH -= 1;
   }
   else
   {
      if ( (d->frameX > 0) && (d->frameW == rdp.ci_width) )
         d->frameW -= (uint16_t)(2.0f*d->frameX);
      if ( (d->frameY > 0) && (d->frameH == rdp.ci_height) )
         d->frameH -= (uint16_t)(2.0f*d->frameY);
   }

   int ul_u = (int)d->imageX;
   int ul_v = (int)d->imageY;
   int lr_u = (int)d->imageX + (int)(d->frameW * d->scaleX);
   int lr_v = (int)d->imageY + (int)(d->frameH * d->scaleY);

   float ul_x, ul_y, lr_x, lr_y;
   if (d->flipX)
   {
      ul_x = d->frameX + d->frameW;
      lr_x = d->frameX;
   }
   else
   {
      ul_x = d->frameX;
      lr_x = d->frameX + d->frameW;
   }
   if (d->flipY)
   {
      ul_y = d->frameY + d->frameH;
      lr_y = d->frameY;
   }
   else
   {
      ul_y = d->frameY;
      lr_y = d->frameY + d->frameH;
   }

   int min_wrap_u = ul_u / d->imageW;
   //int max_wrap_u = lr_u / d->wrapW;
   int min_wrap_v = ul_v / d->imageH;
   //int max_wrap_v = lr_v / d->wrapH;
   int min_256_u = ul_u >> x_shift;
   //int max_256_u = (lr_u-1) >> x_shift;
   int min_256_v = ul_v >> y_shift;
   //int max_256_v = (lr_v-1) >> y_shift;

   gDPSetTextureImage(
         d->imageFmt,                                 /* format - RGBA */
         d->imageSiz,                                 /* size   - 16-bit */
         (d->imageW%2) ? (d->imageW-1) : (d->imageW), /* width */
         d->imagePtr                                  /* address */
         );

   rdp.timg.set_by = 0;

   gDPSetTile(
         d->imageFmt,               /* format (RGBA) */
         d->imageSiz,               /* size (16-bit) */
         line,                      /* line   */
         0,                         /* tmem   */
         0,                         /* tile index no */
         (uint8_t)d->imagePal,      /* palette */
         1,                         /* clamp_t */
         1,                         /* clamp_s */
         0,                         /* mask_t */
         0,                         /* mask_s */
         0,                         /* shift_t */
         0,                         /* shift_s */
         0,                         /* mirror_t */
         0                          /* mirror_s */
         );

   rdp.tiles[0].ul_s = 0;
   rdp.tiles[0].ul_t = 0;
   rdp.tiles[0].lr_s = x_size-1;
   rdp.tiles[0].lr_t = y_size-1;

   const float Z = set_sprite_combine_mode ();
   if (rdp.cycle_mode == G_CYC_COPY)
      rdp.allow_combine = 0;

   {
      if (rdp.ci_width == 512 && !no_dlist)
         grClipWindow (0, 0, settings.scr_res_x, settings.scr_res_y);
      else if (d->scaleX == 1.0f && d->scaleY == 1.0f)
         grClipWindow (rdp.scissor.ul_x, rdp.scissor.ul_y, rdp.scissor.lr_x, rdp.scissor.lr_y);
      else
         grClipWindow (rdp.scissor.ul_x, rdp.scissor.ul_y, min(rdp.scissor.lr_x, (uint32_t)((d->frameX+d->imageW/d->scaleX+0.5f)*rdp.scale_x)), min(rdp.scissor.lr_y, (uint32_t)((d->frameY+d->imageH/d->scaleY+0.5f)*rdp.scale_y)));
      rdp.update |=  UPDATE_SCISSOR;
   }

   // Texture ()
   rdp.cur_tile = 0;

   float nul_x, nul_y, nlr_x, nlr_y;
   int nul_u, nul_v, nlr_u, nlr_v;
   float ful_u, ful_v, flr_u, flr_v;
   float ful_x, ful_y, flr_x, flr_y;

   float mx = (float)(lr_x - ul_x) / (float)(lr_u - ul_u);
   float bx = ul_x - mx * ul_u;

   float my = (float)(lr_y - ul_y) / (float)(lr_v - ul_v);
   float by = ul_y - my * ul_v;

   int cur_wrap_u, cur_wrap_v, cur_u, cur_v;
   int cb_u, cb_v;       // coordinate-base
   int tb_u, tb_v;       // texture-base

   nul_v = ul_v;
   nul_y = ul_y;

   // #162

   cur_wrap_v = min_wrap_v + 1;
   cur_v = min_256_v + 1;
   cb_v = ((cur_v-1)<<y_shift);
   while (cb_v >= d->imageH) cb_v -= d->imageH;
   tb_v = cb_v;
   rdp.bg_image_height = d->imageH;

   while (1)
   {
      cur_wrap_u = min_wrap_u + 1;
      cur_u = min_256_u + 1;

      // calculate intersection with this point
      nlr_v = min (min (cur_wrap_v*d->imageH, (cur_v<<y_shift)), lr_v);
      nlr_y = my * nlr_v + by;

      nul_u = ul_u;
      nul_x = ul_x;
      cb_u = ((cur_u-1)<<x_shift);
      while (cb_u >= d->imageW) cb_u -= d->imageW;
      tb_u = cb_u;

      while (1)
      {
         // calculate intersection with this point
         nlr_u = min (min (cur_wrap_u*d->imageW, (cur_u<<x_shift)), lr_u);
         nlr_x = mx * nlr_u + bx;

         // ** Load the texture, constant portions have been set above
         // SetTileSize ()
         rdp.tiles[0].ul_s = tb_u;
         rdp.tiles[0].ul_t = tb_v;
         rdp.tiles[0].lr_s = tb_u+x_size-1;
         rdp.tiles[0].lr_t = tb_v+y_size-1;

         // LoadTile ()
         rdp.cmd0 = ((int)rdp.tiles[0].ul_s << 14) | ((int)rdp.tiles[0].ul_t << 2);
         rdp.cmd1 = ((int)rdp.tiles[0].lr_s << 14) | ((int)rdp.tiles[0].lr_t << 2);
         gDPLoadTile(
               ((rdp.cmd1 >> 24) & 0x07), /* tile */
               ((rdp.cmd0 >> 14) & 0x03FF), /* ul_s */
               ((rdp.cmd0 >> 2 ) & 0x03FF), /*ul_t */
               ((rdp.cmd1 >> 14) & 0x03FF), /* lr_s */
               ((rdp.cmd1 >> 2 ) & 0x03FF) /* lr_t */
               );

         TexCache ();
         // **

         ful_u = (float)nul_u - cb_u;
         flr_u = (float)nlr_u - cb_u;
         ful_v = (float)nul_v - cb_v;
         flr_v = (float)nlr_v - cb_v;

         ful_u *= rdp.cur_cache[0]->c_scl_x;
         ful_v *= rdp.cur_cache[0]->c_scl_y;
         flr_u *= rdp.cur_cache[0]->c_scl_x;
         flr_v *= rdp.cur_cache[0]->c_scl_y;

         ful_x = nul_x * rdp.scale_x + rdp.offset_x;
         flr_x = nlr_x * rdp.scale_x + rdp.offset_x;
         ful_y = nul_y * rdp.scale_y + rdp.offset_y;
         flr_y = nlr_y * rdp.scale_y + rdp.offset_y;

         // Make the vertices

         if ((flr_x <= rdp.scissor.lr_x) || (ful_x < rdp.scissor.lr_x))
         {
            //not clipped
            int s;
            VERTEX v[4] = {
               { ful_x, ful_y, Z, 1.0f, ful_u, ful_v },
               { flr_x, ful_y, Z, 1.0f, flr_u, ful_v },
               { ful_x, flr_y, Z, 1.0f, ful_u, flr_v },
               { flr_x, flr_y, Z, 1.0f, flr_u, flr_v } };
            AllowShadeMods (v, 4);
            for (s = 0; s < 4; s++)
               apply_shade_mods (&(v[s]));
            ConvertCoordsConvert (v, 4);

            grDrawVertexArrayContiguous (GR_TRIANGLE_STRIP, 4, v, sizeof(VERTEX));

         }
         rdp.tri_n += 2;

         // increment whatever caused this split
         tb_u += x_size - (x_size-(nlr_u-cb_u));
         cb_u = nlr_u;
         if (nlr_u == cur_wrap_u*d->imageW) {
            cur_wrap_u ++;
            tb_u = 0;
         }
         if (nlr_u == (cur_u<<x_shift)) cur_u ++;
         if (nlr_u == lr_u) break;
         nul_u = nlr_u;
         nul_x = nlr_x;
      }

      tb_v += y_size - (y_size-(nlr_v-cb_v));
      cb_v = nlr_v;
      if (nlr_v == cur_wrap_v*d->imageH) {
         cur_wrap_v ++;
         tb_v = 0;
      }
      if (nlr_v == (cur_v<<y_shift)) cur_v ++;
      if (nlr_v == lr_v) break;
      nul_v = nlr_v;
      nul_y = nlr_y;
   }

   rdp.allow_combine = 1;
   rdp.bg_image_height = 0xFFFF;
}

#ifdef HAVE_HWFBE
void DrawHiresImage(DRAWIMAGE *d, int screensize)
{
   TBUFF_COLOR_IMAGE *tbuff_tex = (TBUFF_COLOR_IMAGE*)rdp.tbuff_tex;
   if (rdp.motionblur)
      rdp.tbuff_tex = &(rdp.texbufs[rdp.cur_tex_buf^1].images[0]);
   else if (rdp.tbuff_tex == 0)
      return;
   FRDP("DrawHiresImage. fb format=%d\n", rdp.tbuff_tex->info.format);

   setTBufTex(rdp.tbuff_tex->t_mem, rdp.tbuff_tex->width << rdp.tbuff_tex->size >> 1);

   const float Z = set_sprite_combine_mode();
   grClipWindow (0, 0, settings.res_x, settings.res_y);

   if (d->imageW%2 == 1) d->imageW -= 1;
   if (d->imageH%2 == 1) d->imageH -= 1;
   if (d->imageY > d->imageH) d->imageY = (d->imageY%d->imageH);

   if (!(settings.hacks&hack_PPL))
   {
      if ( (d->frameX > 0) && (d->frameW == rdp.ci_width) )
         d->frameW -= (uint16_t)(2.0f*d->frameX);
      if ( (d->frameY > 0) && (d->frameH == rdp.ci_height) )
         d->frameH -= (uint16_t)(2.0f*d->frameY);
   }

   int s;
   float ul_x, ul_y, ul_u, ul_v, lr_x, lr_y, lr_u, lr_v;
   if (screensize)
   {
      ul_x = 0.0f;
      ul_y = 0.0f;
      ul_u = 0.15f;
      ul_v = 0.15f;
      lr_x = rdp.tbuff_tex->scr_width;
      lr_y = rdp.tbuff_tex->scr_height;
      lr_u = rdp.tbuff_tex->lr_u;
      lr_v = rdp.tbuff_tex->lr_v;
   }
   else
   {
      ul_u = d->imageX;
      ul_v = d->imageY;
      lr_u = d->imageX + (d->frameW * d->scaleX) ;
      lr_v = d->imageY + (d->frameH * d->scaleY) ;

      ul_x = d->frameX;
      ul_y = d->frameY;

      lr_x = d->frameX + d->frameW;
      lr_y = d->frameY + d->frameH;
      ul_x *= rdp.scale_x;
      lr_x *= rdp.scale_x;
      ul_y *= rdp.scale_y;
      lr_y *= rdp.scale_y;
      ul_u *= rdp.tbuff_tex->u_scale;
      lr_u *= rdp.tbuff_tex->u_scale;
      ul_v *= rdp.tbuff_tex->v_scale;
      lr_v *= rdp.tbuff_tex->v_scale;
      ul_u = max(0.15f, ul_u);
      ul_v = max(0.15f, ul_v);
      if (lr_x > rdp.scissor.lr_x) lr_x = (float)rdp.scissor.lr_x;
      if (lr_y > rdp.scissor.lr_y) lr_y = (float)rdp.scissor.lr_y;
   }
   // Make the vertices
   VERTEX v[4] = {
      { ul_x, ul_y, Z, 1.0f, ul_u, ul_v, ul_u, ul_v },
      { lr_x, ul_y, Z, 1.0f, lr_u, ul_v, lr_u, ul_v },
      { ul_x, lr_y, Z, 1.0f, ul_u, lr_v, ul_u, lr_v },
      { lr_x, lr_y, Z, 1.0f, lr_u, lr_v, lr_u, lr_v } };
   ConvertCoordsConvert (v, 4);
   AllowShadeMods (v, 4);
   AddOffset(v, 4);
   for (s = 0; s < 4; s++)
      apply_shade_mods (&(v[s]));
   grDrawTriangle(&v[0], &v[2], &v[1]);
   grDrawTriangle(&v[2], &v[3], &v[1]);
   rdp.update |= UPDATE_ZBUF_ENABLED | UPDATE_COMBINE | UPDATE_TEXTURE | UPDATE_ALPHA_COMPARE | UPDATE_SCISSOR;
   rdp.tri_n += 2;
   rdp.tbuff_tex = tbuff_tex;
}
#endif

//****************************************************************

static void uc6_read_background_data (DRAWIMAGE *d, bool bReadScale)
{
   uint32_t addr = segoffset(rdp.cmd1) >> 1;

   d->imageX      = (((uint16_t *)gfx.RDRAM)[(addr+0)^1] >> 5);   // 0
   d->imageW      = (((uint16_t *)gfx.RDRAM)[(addr+1)^1] >> 2);   // 1
   d->frameX      = ((int16_t*)gfx.RDRAM)[(addr+2)^1] / 4.0f;       // 2
   d->frameW      = ((uint16_t *)gfx.RDRAM)[(addr+3)^1] >> 2;             // 3

   d->imageY      = (((uint16_t *)gfx.RDRAM)[(addr+4)^1] >> 5);   // 4
   d->imageH      = (((uint16_t *)gfx.RDRAM)[(addr+5)^1] >> 2);   // 5
   d->frameY      = ((int16_t*)gfx.RDRAM)[(addr+6)^1] / 4.0f;       // 6
   d->frameH      = ((uint16_t *)gfx.RDRAM)[(addr+7)^1] >> 2;             // 7

   d->imagePtr    = segoffset(((uint32_t*)gfx.RDRAM)[(addr+8)>>1]);       // 8,9
   d->imageFmt    = ((uint8_t *)gfx.RDRAM)[(((addr+11)<<1)+0)^3]; // 11
   d->imageSiz    = ((uint8_t *)gfx.RDRAM)[(((addr+11)<<1)+1)^3]; // |
   d->imagePal    = ((uint16_t *)gfx.RDRAM)[(addr+12)^1]; // 12
   uint16_t imageFlip = ((uint16_t *)gfx.RDRAM)[(addr+13)^1];    // 13;
   d->flipX       = (uint8_t)imageFlip & G_BG_FLAG_FLIPS;

   if (bReadScale)
   {
      d->scaleX      = ((int16_t*)gfx.RDRAM)[(addr+14)^1] / 1024.0f;  // 14
      d->scaleY      = ((int16_t*)gfx.RDRAM)[(addr+15)^1] / 1024.0f;  // 15
   }
   else
      d->scaleX = d->scaleY = 1.0f;

   d->flipY       = 0;
   int imageYorig= ((int *)gfx.RDRAM)[(addr+16)>>1] >> 5;
   rdp.last_bg = d->imagePtr;

   FRDP ("imagePtr: %08lx\n", d->imagePtr);
   FRDP ("frameX: %f, frameW: %d, frameY: %f, frameH: %d\n", d->frameX, d->frameW, d->frameY, d->frameH);
   FRDP ("imageX: %d, imageW: %d, imageY: %d, imageH: %d\n", d->imageX, d->imageW, d->imageY, d->imageH);
   FRDP ("imageYorig: %d, scaleX: %f, scaleY: %f\n", imageYorig, d->scaleX, d->scaleY);
   FRDP ("imageFmt: %d, imageSiz: %d, imagePal: %d, imageFlip: %d\n", d->imageFmt, d->imageSiz, d->imagePal, d->flipX);
}

static void uc6_bg (bool bg_1cyc)
{
   DRAWIMAGE d;
   bool draw_from_fb;
   if (rdp.skip_drawing)
      return;

   uc6_read_background_data(&d, bg_1cyc);

#ifdef HAVE_HWFBE
   if (fb_hwfbe_enabled && FindTextureBuffer(d.imagePtr, d.imageW))
   {
      DrawHiresImage(d, false);
      return;
   }
#endif

   draw_from_fb = !((settings.ucode == ucode_F3DEX2 || (settings.hacks&hack_PPL))
      && !((d.imagePtr != rdp.cimg) && (d.imagePtr != rdp.ocimg) && d.imagePtr));

   if (draw_from_fb)
      DrawImage (&d);
   //FRDP ("%s #%d, #%d\n", bg_1cyc ? "uc6:bg_1cyc" : "uc6:bg_copy", rdp.tri_n, rdp.tri_n+1);
}

static void uc6_bg_1cyc(uint32_t w0, uint32_t w1)
{
  uc6_bg(true);
}

static void uc6_bg_copy(uint32_t w0, uint32_t w1)
{
  uc6_bg(false);
}

static void draw_split_triangle(VERTEX **vtx)
{
  vtx[0]->not_zclipped = vtx[1]->not_zclipped = vtx[2]->not_zclipped = 1;

  int index,i,j, min_256,max_256, cur_256,left_256,right_256;
  float percent;

  min_256 = min((int)vtx[0]->u0,(int)vtx[1]->u0); // bah, don't put two mins on one line
  min_256 = min(min_256,(int)vtx[2]->u0) >> 8;  // or it will be calculated twice

  max_256 = max((int)vtx[0]->u0,(int)vtx[1]->u0); // not like it makes much difference
  max_256 = max(max_256,(int)vtx[2]->u0) >> 8;  // anyway :P

  for (cur_256=min_256; cur_256<=max_256; cur_256++)
  {
    left_256 = cur_256 << 8;
    right_256 = (cur_256+1) << 8;

    // Set vertex buffers
    rdp.vtxbuf = rdp.vtx1;  // copy from v to rdp.vtx1
    rdp.vtxbuf2 = rdp.vtx2;
    rdp.vtx_buffer = 0;
    rdp.n_global = 3;
    index = 0;

    // ** Left plane **
    for (i=0; i<3; i++)
    {
      j = i+1;
      if (j == 3) j = 0;

      VERTEX *v1 = vtx[i];
      VERTEX *v2 = vtx[j];

      if (v1->u0 >= left_256)
      {
        if (v2->u0 >= left_256)   // Both are in, save the last one
        {
          rdp.vtxbuf[index] = *v2;
          rdp.vtxbuf[index].u0 -= left_256;
          rdp.vtxbuf[index++].v0 += rdp.cur_cache[0]->c_scl_y * (cur_256 * rdp.cur_cache[0]->splitheight);
        }
        else      // First is in, second is out, save intersection
        {
          percent = (left_256 - v1->u0) / (v2->u0 - v1->u0);
          rdp.vtxbuf[index].x = v1->x + (v2->x - v1->x) * percent;
          rdp.vtxbuf[index].y = v1->y + (v2->y - v1->y) * percent;
          rdp.vtxbuf[index].z = 1;
          rdp.vtxbuf[index].q = 1;
          rdp.vtxbuf[index].u0 = 0.5f;
          rdp.vtxbuf[index].v0 = v1->v0 + (v2->v0 - v1->v0) * percent +
            rdp.cur_cache[0]->c_scl_y * cur_256 * rdp.cur_cache[0]->splitheight;
          rdp.vtxbuf[index].b = (uint8_t)(v1->b + (v2->b - v1->b) * percent);
          rdp.vtxbuf[index].g = (uint8_t)(v1->g + (v2->g - v1->g) * percent);
          rdp.vtxbuf[index].r = (uint8_t)(v1->r + (v2->r - v1->r) * percent);
          rdp.vtxbuf[index++].a = (uint8_t)(v1->a + (v2->a - v1->a) * percent);
        }
      }
      else
      {
        if (v2->u0 >= left_256) // First is out, second is in, save intersection & in point
        {
          percent = (left_256 - v2->u0) / (v1->u0 - v2->u0);
          rdp.vtxbuf[index].x = v2->x + (v1->x - v2->x) * percent;
          rdp.vtxbuf[index].y = v2->y + (v1->y - v2->y) * percent;
          rdp.vtxbuf[index].z = 1;
          rdp.vtxbuf[index].q = 1;
          rdp.vtxbuf[index].u0 = 0.5f;
          rdp.vtxbuf[index].v0 = v2->v0 + (v1->v0 - v2->v0) * percent +
            rdp.cur_cache[0]->c_scl_y * cur_256 * rdp.cur_cache[0]->splitheight;
          rdp.vtxbuf[index].b = (uint8_t)(v2->b + (v1->b - v2->b) * percent);
          rdp.vtxbuf[index].g = (uint8_t)(v2->g + (v1->g - v2->g) * percent);
          rdp.vtxbuf[index].r = (uint8_t)(v2->r + (v1->r - v2->r) * percent);
          rdp.vtxbuf[index++].a = (uint8_t)(v2->a + (v1->a - v2->a) * percent);

          // Save the in point
          rdp.vtxbuf[index] = *v2;
          rdp.vtxbuf[index].u0 -= left_256;
          rdp.vtxbuf[index++].v0 += rdp.cur_cache[0]->c_scl_y * (cur_256 * rdp.cur_cache[0]->splitheight);
        }
      }
    }
    rdp.n_global = index;

    rdp.vtxbuf = rdp.vtx2;  // now vtx1 holds the value, & vtx2 is the destination
    rdp.vtxbuf2 = rdp.vtx1;
    rdp.vtx_buffer ^= 1;
    index = 0;

    for (i=0; i<rdp.n_global; i++)
    {
      j = i+1;
      if (j == rdp.n_global) j = 0;

      VERTEX *v1 = &rdp.vtxbuf2[i];
      VERTEX *v2 = &rdp.vtxbuf2[j];

      // ** Right plane **
      if (v1->u0 <= 256.0f)
      {
        if (v2->u0 <= 256.0f)   // Both are in, save the last one
        {
          rdp.vtxbuf[index++] = *v2;
        }
        else      // First is in, second is out, save intersection
        {
          percent = (right_256 - v1->u0) / (v2->u0 - v1->u0);
          rdp.vtxbuf[index].x = v1->x + (v2->x - v1->x) * percent;
          rdp.vtxbuf[index].y = v1->y + (v2->y - v1->y) * percent;
          rdp.vtxbuf[index].z = 1;
          rdp.vtxbuf[index].q = 1;
          rdp.vtxbuf[index].u0 = 255.5f;
          rdp.vtxbuf[index].v0 = v1->v0 + (v2->v0 - v1->v0) * percent;
          rdp.vtxbuf[index].b = (uint8_t)(v1->b + (v2->b - v1->b) * percent);
          rdp.vtxbuf[index].g = (uint8_t)(v1->g + (v2->g - v1->g) * percent);
          rdp.vtxbuf[index].r = (uint8_t)(v1->r + (v2->r - v1->r) * percent);
          rdp.vtxbuf[index++].a = (uint8_t)(v1->a + (v2->a - v1->a) * percent);
        }
      }
      else
      {
        if (v2->u0 <= 256.0f) // First is out, second is in, save intersection & in point
        {
          percent = (right_256 - v2->u0) / (v1->u0 - v2->u0);
          rdp.vtxbuf[index].x = v2->x + (v1->x - v2->x) * percent;
          rdp.vtxbuf[index].y = v2->y + (v1->y - v2->y) * percent;
          rdp.vtxbuf[index].z = 1;
          rdp.vtxbuf[index].q = 1;
          rdp.vtxbuf[index].u0 = 255.5f;
          rdp.vtxbuf[index].v0 = v2->v0 + (v1->v0 - v2->v0) * percent;
          rdp.vtxbuf[index].b = (uint8_t)(v2->b + (v1->b - v2->b) * percent);
          rdp.vtxbuf[index].g = (uint8_t)(v2->g + (v1->g - v2->g) * percent);
          rdp.vtxbuf[index].r = (uint8_t)(v2->r + (v1->r - v2->r) * percent);
          rdp.vtxbuf[index++].a = (uint8_t)(v2->a + (v1->a - v2->a) * percent);

          // Save the in point
          rdp.vtxbuf[index++] = *v2;
        }
      }
    }
    rdp.n_global = index;

    do_triangle_stuff_2 (0);
  }
}

static void uc6_draw_polygons (VERTEX v[4])
{
   int s;
   AllowShadeMods (v, 4);
   for (s = 0; s < 4; s++)
      apply_shade_mods (&(v[s]));
   AddOffset(v, 4);

   // Set vertex buffers
   rdp.vtxbuf = rdp.vtx1;      // copy from v to rdp.vtx1
   rdp.vtxbuf2 = rdp.vtx2;
   rdp.vtx_buffer = 0;
   rdp.n_global = 3;
   memcpy (rdp.vtxbuf, v, sizeof(VERTEX)*3);
   do_triangle_stuff_2 (0);
   rdp.tri_n ++;

   rdp.vtxbuf = rdp.vtx1;      // copy from v to rdp.vtx1
   rdp.vtxbuf2 = rdp.vtx2;
   rdp.vtx_buffer = 0;
   rdp.n_global = 3;
   memcpy (rdp.vtxbuf, v+1, sizeof(VERTEX)*3);
   do_triangle_stuff_2 (0);
   rdp.tri_n ++;
   rdp.update |= UPDATE_ZBUF_ENABLED | UPDATE_VIEWPORT;

   if (settings.fog && (rdp.flags & FOG_ENABLED))
      grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
}

static void uc6_read_object_data (DRAWOBJECT *d)
{
   uint32_t addr = segoffset(rdp.cmd1) >> 1;

   d->objX            = ((int16_t*)gfx.RDRAM)[(addr+0)^1] / 4.0f;               // 0
   d->scaleW  = ((uint16_t *)gfx.RDRAM)[(addr+1)^1] / 1024.0f;        // 1
   d->imageW  = ((int16_t*)gfx.RDRAM)[(addr+2)^1] >> 5;                 // 2, 3 is padding
   d->objY            = ((int16_t*)gfx.RDRAM)[(addr+4)^1] / 4.0f;               // 4
   d->scaleH  = ((uint16_t *)gfx.RDRAM)[(addr+5)^1] / 1024.0f;        // 5
   d->imageH  = ((int16_t*)gfx.RDRAM)[(addr+6)^1] >> 5;                 // 6, 7 is padding

   d->imageStride = ((uint16_t *)gfx.RDRAM)[(addr+8)^1];                  // 8
   d->imageAdrs           = ((uint16_t *)gfx.RDRAM)[(addr+9)^1];                  // 9
   d->imageFmt             = ((uint8_t *)gfx.RDRAM)[(((addr+10)<<1)+0)^3]; // 10
   d->imageSiz             = ((uint8_t *)gfx.RDRAM)[(((addr+10)<<1)+1)^3]; // |
   d->imagePal             = ((uint8_t *)gfx.RDRAM)[(((addr+10)<<1)+2)^3]; // 11
   d->imageFlags   = ((uint8_t *)gfx.RDRAM)[(((addr+10)<<1)+3)^3]; // |

   if (d->imageW < 0)
      d->imageW = (int16_t)rdp.scissor_o.lr_x - (int16_t)d->objX - d->imageW;
   if (d->imageH < 0)
      d->imageH = (int16_t)rdp.scissor_o.lr_y - (int16_t)d->objY - d->imageH;

   FRDP ("#%d, #%d\n"
         "objX: %f, scaleW: %f, imageW: %d\n"
         "objY: %f, scaleH: %f, imageH: %d\n"
         "size: %d, format: %d\n", rdp.tri_n, rdp.tri_n+1,
         d->objX, d->scaleW, d->imageW, d->objY, d->scaleH, d->imageH, d->imageSiz, d->imageFmt);
}

static void uc6_init_tile(const DRAWOBJECT *d)
{
   gDPSetTile(
         d->imageFmt,         /* format - RGBA */
         d->imageSiz,         /* size - 16-bit */
         d->imageStride,      /* line */
         d->imageAdrs,        /* tmem */
         0,                   /* tile_idx */
         d->imagePal,         /* palette */
         1,                   /* cmt */
         1,                   /* cms */
         0,                   /* mask_t */
         0,                   /* mask_s */
         0,                   /* shiftt */
         0,                   /* shifts */
         0,                   /* mirrort */
         0                    /* mirrors */
         );

  gDPSetTileSize(
        0,                                      /* tile */
        0,                                      /* ul_s */
        0,                                      /* ul_t */
        (d->imageW > 0) ? d->imageW-1 : 0,      /* lr_s */
        (d->imageH > 0) ? d->imageH-1 : 0       /* lr_t */
        );
}

static void uc6_obj_rectangle(uint32_t w0, uint32_t w1)
{
   int i;
  LRDP ("uc6:obj_rectangle ");
  DRAWOBJECT d;
  uc6_read_object_data(&d);

  if (d.imageAdrs > 4096)
  {
    FRDP("tmem: %08lx is out of bounds! return\n", d.imageAdrs);
    return;
  }
  if (!rdp.s2dex_tex_loaded)
  {
    LRDP("Texture was not loaded! return\n");
    return;
  }

  uc6_init_tile(&d);

  float Z = set_sprite_combine_mode();

  float ul_x = d.objX;
  float lr_x = d.objX + d.imageW / d.scaleW;
  float ul_y = d.objY;
  float lr_y = d.objY + d.imageH / d.scaleH;
  float ul_u, ul_v;
  float lr_u = 255.0f*rdp.cur_cache[0]->scale_x;
  float lr_v = 255.0f*rdp.cur_cache[0]->scale_y;

  if (d.imageFlags & G_BG_FLAG_FLIPS) /* flipS */
  {
    ul_u = lr_u;
    lr_u = 0.5f;
  }
  else
    ul_u = 0.5f;
  if (d.imageFlags & G_BG_FLAG_FLIPT) //flipT
  {
    ul_v = lr_v;
    lr_v = 0.5f;
  }
  else
    ul_v = 0.5f;

  // Make the vertices
  VERTEX v[4] = {
    { ul_x, ul_y, Z, 1, ul_u, ul_v },
    { lr_x, ul_y, Z, 1, lr_u, ul_v },
    { ul_x, lr_y, Z, 1, ul_u, lr_v },
    { lr_x, lr_y, Z, 1, lr_u, lr_v }
  };

  for (i = 0; i < 4; i++)
  {
    v[i].x *= rdp.scale_x;
    v[i].y *= rdp.scale_y;
  }

  uc6_draw_polygons (v);
}

static void uc6_obj_sprite(uint32_t w0, uint32_t w1)
{
   gSPObjSprite();
}

static void uc6_obj_movemem(uint32_t w0, uint32_t w1)
{
  if (_SHIFTR( w0, 0, 16 ) == 0)
     gSPObjMatrix(w1);
  else if (_SHIFTR( w0, 0, 16 ) == 2)
     gSPObjSubMatrix(w1);
  //LRDP("uc6:obj_movemem\n");
}

static void uc6_select_dl(uint32_t w0, uint32_t w1)
{
  LRDP("uc6:select_dl\n");
  RDP_E ("uc6:select_dl\n");
}

static void uc6_obj_rendermode(uint32_t w0, uint32_t w1)
{
  LRDP("uc6:obj_rendermode\n");
  RDP_E ("uc6:obj_rendermode\n");
}

static uint16_t uc6_yuv_to_rgba(uint8_t y, uint8_t u, uint8_t v)
{
  float r = y + (1.370705f * (v-128));
  float g = y - (0.698001f * (v-128)) - (0.337633f * (u-128));
  float b = y + (1.732446f * (u-128));
  r *= 0.125f;
  g *= 0.125f;
  b *= 0.125f;
  //clipping the result
  if (r > 32) r = 32;
  if (g > 32) g = 32;
  if (b > 32) b = 32;
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;

  uint16_t c = (uint16_t)(((uint16_t)(r) << 11) |
    ((uint16_t)(g) << 6) |
    ((uint16_t)(b) << 1) | 1);
  return c;
}

static void uc6_DrawYUVImageToFrameBuffer(uint16_t ul_x, uint16_t ul_y, uint16_t lr_x, uint16_t lr_y)
{
   uint16_t h, w;
  FRDP ("uc6:DrawYUVImageToFrameBuffer ul_x%d, ul_y%d, lr_x%d, lr_y%d\n", ul_x, ul_y, lr_x, lr_y);
  uint32_t ci_width = rdp.ci_width;
  uint32_t ci_height = rdp.ci_lower_bound;
  if (ul_x >= ci_width)
    return;
  if (ul_y >= ci_height)
    return;
  uint32_t width = 16, height = 16;
  if (lr_x > ci_width)
    width = ci_width - ul_x;
  if (lr_y > ci_height)
    height = ci_height - ul_y;
  uint32_t * mb = (uint32_t*)(gfx.RDRAM+rdp.timg.addr); //pointer to the first macro block
  uint16_t * dst = (uint16_t*)(gfx.RDRAM+rdp.cimg);
  dst += ul_x + ul_y * ci_width;
  //yuv macro block contains 16x16 texture. we need to put it in the proper place inside cimg
  for (h = 0; h < 16; h++)
  {
    for (w = 0; w < 16; w+=2)
    {
      uint32_t t = *(mb++); //each uint32_t contains 2 pixels
      if ((h < height) && (w < width)) //clipping. texture image may be larger than color image
      {
        uint8_t y0 = (uint8_t)t&0xFF;
        uint8_t v  = (uint8_t)(t>>8)&0xFF;
        uint8_t y1 = (uint8_t)(t>>16)&0xFF;
        uint8_t u  = (uint8_t)(t>>24)&0xFF;
        *(dst++) = uc6_yuv_to_rgba(y0, u, v);
        *(dst++) = uc6_yuv_to_rgba(y1, u, v);
      }
    }
    dst += rdp.ci_width - 16;
  }
}

static void uc6_obj_rectangle_r(uint32_t w0, uint32_t w1)
{
   int i;
   LRDP ("uc6:obj_rectangle_r ");
   DRAWOBJECT d;
   uc6_read_object_data(&d);

   if (d.imageFmt == 1 && (settings.hacks&hack_Ogre64)) //Ogre Battle needs to copy YUV texture to frame buffer
   {
      float ul_x = d.objX/mat_2d.BaseScaleX + mat_2d.X;
      float lr_x = (d.objX + d.imageW / d.scaleW) / mat_2d.BaseScaleX + mat_2d.X;
      float ul_y = d.objY / mat_2d.BaseScaleY + mat_2d.Y;
      float lr_y = (d.objY + d.imageH / d.scaleH) / mat_2d.BaseScaleY + mat_2d.Y;
      uc6_DrawYUVImageToFrameBuffer((uint16_t)ul_x, (uint16_t)ul_y, (uint16_t)lr_x, (uint16_t)lr_y);
      rdp.tri_n += 2;
      return;
   }

   uc6_init_tile(&d);

   float Z = set_sprite_combine_mode();

   float ul_x = d.objX / mat_2d.BaseScaleX;
   float lr_x = (d.objX + d.imageW / d.scaleW) / mat_2d.BaseScaleX;
   float ul_y = d.objY / mat_2d.BaseScaleY;
   float lr_y = (d.objY + d.imageH / d.scaleH) / mat_2d.BaseScaleY;
   float ul_u, ul_v;
   float lr_u = 255.0f * rdp.cur_cache[0]->scale_x;
   float lr_v = 255.0f * rdp.cur_cache[0]->scale_y;

   if (d.imageFlags & G_BG_FLAG_FLIPS) //flipS
   {
      ul_u = lr_u;
      lr_u = 0.5f;
   }
   else
      ul_u = 0.5f;
   if (d.imageFlags & G_BG_FLAG_FLIPT) //flipT
   {
      ul_v = lr_v;
      lr_v = 0.5f;
   }
   else
      ul_v = 0.5f;

   // Make the vertices
   VERTEX v[4] = {
      { ul_x, ul_y, Z, 1, ul_u, ul_v },
      { lr_x, ul_y, Z, 1, lr_u, ul_v },
      { ul_x, lr_y, Z, 1, ul_u, lr_v },
      { lr_x, lr_y, Z, 1, lr_u, lr_v }
   };

   for (i = 0; i < 4; i++)
   {
      float x = v[i].x;
      float y = v[i].y;
      v[i].x = (x + mat_2d.X) * rdp.scale_x;
      v[i].y = (y + mat_2d.Y) * rdp.scale_y;
   }

   uc6_draw_polygons (v);
}

static void uc6_obj_loadtxtr(uint32_t w0, uint32_t w1)
{
   gSPObjLoadTxtr(w1);
}

static void uc6_obj_ldtx_sprite(uint32_t w0, uint32_t w1)
{
   gSPObjLoadTxSprite(w1);
}

static void uc6_obj_ldtx_rect(uint32_t w0, uint32_t w1)
{
   LRDP("uc6:obj_ldtx_rect\n");

   uint32_t addr = rdp.cmd1;
   gSPObjLoadTxtr(w1);
   rdp.cmd1 = addr + 24;
   uc6_obj_rectangle(w0, w1);
}

static void uc6_ldtx_rect_r(uint32_t w0, uint32_t w1)
{
   LRDP("uc6:ldtx_rect_r\n");

   uint32_t addr = rdp.cmd1;
   gSPObjLoadTxtr(w1);
   rdp.cmd1 = addr + 24;
   uc6_obj_rectangle_r(w0, w1);
}

static void uc6_loaducode(uint32_t w0, uint32_t w1)
{
   LRDP("uc6:load_ucode\n");
   RDP_E ("uc6:load_ucode\n");

   // copy the microcode data
   uint32_t addr = segoffset(rdp.cmd1);
   uint32_t size = (rdp.cmd0 & 0xFFFF) + 1;
   memcpy (microcode, gfx.RDRAM+addr, size);

   microcheck ();
}

static void uc6_sprite2d(uint32_t w0, uint32_t w1)
{
   int i, s;
   uint16_t stride;
   uint32_t a, cmd0, addr, tlut;
   DRAWIMAGE d;

   a = rdp.pc[rdp.pc_i] & BMASK;
   cmd0 = ((uint32_t*)gfx.RDRAM)[a>>2]; //check next command
   if ( (cmd0 >> 24) != 0xBE )
      return;

   FRDP ("uc6:uc6_sprite2d #%d, #%d\n", rdp.tri_n, rdp.tri_n+1);
   addr = segoffset(w1) >> 1;

   d.imagePtr    = segoffset(((uint32_t*)gfx.RDRAM)[(addr+0)>>1]);       // 0,1
   stride        = (((uint16_t *)gfx.RDRAM)[(addr+4)^1]);      // 4
   d.imageW      = (((uint16_t *)gfx.RDRAM)[(addr+5)^1]);        // 5
   d.imageH      = (((uint16_t *)gfx.RDRAM)[(addr+6)^1]);        // 6
   d.imageFmt    = ((uint8_t *)gfx.RDRAM)[(((addr+7)<<1)+0)^3];  // 7
   d.imageSiz    = ((uint8_t *)gfx.RDRAM)[(((addr+7)<<1)+1)^3];  // |
   d.imagePal    = 0;
   d.imageX      = (((uint16_t *)gfx.RDRAM)[(addr+8)^1]);        // 8
   d.imageY      = (((uint16_t *)gfx.RDRAM)[(addr+9)^1]);        // 9
   tlut          = ((uint32_t*)gfx.RDRAM)[(addr + 2) >> 1];      // 2, 3

   //low-level implementation of sprite2d apparently calls setothermode command to set tlut mode
   //However, description of sprite2d microcode just says that
   //TlutPointer should be Null when CI images will not be used->
   //HLE implementation sets rdp.tlut_mode=2 if TlutPointer is not null, and rdp.tlut_mode=0 otherwise
   //Alas, it is not sufficient, since WCW Nitro uses non-Null TlutPointer for rgba textures.
   //So, additional check added->
   
   if (tlut)
   {
      load_palette (segoffset(tlut), 0, 256);
      if (d.imageFmt > 0)
         rdp.tlut_mode = 2;
      else
         rdp.tlut_mode = 0;
   }
   else
      rdp.tlut_mode = 0;

   if (d.imageW == 0)
      return;//     d.imageW = stride;

   cmd0 = ((uint32_t*)gfx.RDRAM)[a>>2]; //check next command
   do
   {
      if ( (cmd0>>24) == 0xBE )
      {
         uint32_t cmd1 = ((uint32_t*)gfx.RDRAM)[(a>>2)+1];
         rdp.pc[rdp.pc_i] = (a+8) & BMASK;

         d.scaleX  = ((cmd1>>16)&0xFFFF)/1024.0f;
         d.scaleY  = (cmd1&0xFFFF)/1024.0f;
         //the code below causes wrong background height in super robot spirit, so it is disabled->
         //need to find, for which game this hack was made
         //if( (cmd1&0xFFFF) < 0x100 )
         //  d.scaleY = d.scaleX;
         d.flipX = (uint8_t)((cmd0>>8)&0xFF);
         d.flipY = (uint8_t)(cmd0&0xFF);

         a = rdp.pc[rdp.pc_i] & BMASK;
         rdp.pc[rdp.pc_i] = (a+8) & BMASK;
         cmd0 = ((uint32_t*)gfx.RDRAM)[a>>2]; //check next command
      }
      if ( (cmd0>>24) == 0xBD )
      {
         uint32_t cmd1 = ((uint32_t*)gfx.RDRAM)[(a>>2)+1];

         d.frameX  = ((int16_t)((cmd1>>16)&0xFFFF)) / 4.0f;
         d.frameY  = ((int16_t)(cmd1&0xFFFF)) / 4.0f;
         d.frameW    = (uint16_t) (d.imageW / d.scaleX);
         d.frameH    = (uint16_t) (d.imageH / d.scaleY);
         if (settings.hacks&hack_WCWnitro)
         {
            int scaleY = (int)d.scaleY;
            d.imageH        /= scaleY;
            d.imageY        /= scaleY;
            stride      *= scaleY;
            d.scaleY        = 1.0f;
         }
#ifndef NDEBUG
         FRDP ("imagePtr: %08lx\n", d.imagePtr);
         FRDP ("frameX: %f, frameW: %d, frameY: %f, frameH: %d\n", d.frameX, d.frameW, d.frameY, d.frameH);
         FRDP ("imageX: %d, imageW: %d, imageY: %d, imageH: %d\n", d.imageX, d.imageW, d.imageY, d.imageH);
         FRDP ("imageFmt: %d, imageSiz: %d, imagePal: %d, imageStride: %d\n", d.imageFmt, d.imageSiz, d.imagePal, stride);
         FRDP ("scaleX: %f, scaleY: %f\n", d.scaleX, d.scaleY);
#endif
      }
      else
         return;

      const uint32_t texsize = (d.imageW * d.imageH) << d.imageSiz >> 1;
      const uint32_t maxTexSize = rdp.tlut_mode < 2 ? 4096 : 2048;

      if (texsize > maxTexSize)
      {
         if (d.scaleX != 1)
            d.scaleX *= (float)stride / (float)d.imageW;
         d.imageW  = stride;
         d.imageH  += d.imageY;
         DrawImage (&d);
      }
      else
      {
         uint16_t line = d.imageW;
         if (line & 7)
            line += 8;  // round up
         line >>= 3;
         if (d.imageSiz == 0)
         {
            if (line%2)
               line++;
            line >>= 1;
         }
         else
            line <<= (d.imageSiz-1);

         if (line == 0)
            line = 1;

         rdp.timg.addr = d.imagePtr;
         rdp.timg.width = stride;
         rdp.tiles[7].t_mem = 0;
         rdp.tiles[7].line = line;//(d.imageW>>3);
         rdp.tiles[7].size = d.imageSiz;
         rdp.cmd0 = (d.imageX << 14) | (d.imageY << 2);
         rdp.cmd1 = 0x07000000 | ((d.imageX + d.imageW-1) << 14) | ((d.imageY + d.imageH-1) << 2);
         gDPLoadTile(
               ((rdp.cmd1 >> 24) & 0x07), /* tile */
               ((rdp.cmd0 >> 14) & 0x03FF), /* ul_s */
               ((rdp.cmd0 >> 2 ) & 0x03FF), /*ul_t */
               ((rdp.cmd1 >> 14) & 0x03FF), /* lr_s */
               ((rdp.cmd1 >> 2 ) & 0x03FF) /* lr_t */
               );

         // SetTile ()
         TILE *tile = &rdp.tiles[0];
         tile->format = d.imageFmt;
         tile->size = d.imageSiz;
         tile->line = line;//(d.imageW>>3);
         tile->t_mem = 0;
         tile->palette = 0;
         tile->clamp_t = 1;
         tile->mirror_t = 0;
         tile->mask_t = 0;
         tile->shift_t = 0;
         tile->clamp_s = 1;
         tile->mirror_s = 0;
         tile->mask_s = 0;
         tile->shift_s = 0;

         // SetTileSize ()
         rdp.tiles[0].ul_s = d.imageX;
         rdp.tiles[0].ul_t = d.imageY;
         rdp.tiles[0].lr_s = d.imageX + d.imageW-1;
         rdp.tiles[0].lr_t = d.imageY + d.imageH-1;

         float Z = set_sprite_combine_mode ();

         float ul_x, ul_y, lr_x, lr_y;
         if (d.flipX)
         {
            ul_x = d.frameX + d.frameW;
            lr_x = d.frameX;
         }
         else
         {
            ul_x = d.frameX;
            lr_x = d.frameX + d.frameW;
         }
         if (d.flipY)
         {
            ul_y = d.frameY + d.frameH;
            lr_y = d.frameY;
         }
         else
         {
            ul_y = d.frameY;
            lr_y = d.frameY + d.frameH;
         }

         float lr_u = 255.0f*rdp.cur_cache[0]->scale_x;
         float lr_v = 255.0f*rdp.cur_cache[0]->scale_y;

         // Make the vertices
         VERTEX v[4] = {
            {ul_x, ul_y, Z, 1, 0.5f, 0.5f },
            {lr_x, ul_y, Z, 1, lr_u, 0.5f },
            {ul_x, lr_y, Z, 1, 0.5f, lr_v },
            {lr_x, lr_y, Z, 1, lr_u, lr_v }
         };

         for (i = 0; i < 4; i++)
         {
            v[i].x *= rdp.scale_x;
            v[i].y *= rdp.scale_y;
         }

         //      ConvertCoordsConvert (v, 4);
         AllowShadeMods (v, 4);
         for (s = 0; s < 4; s++)
            apply_shade_mods (&(v[s]));
         AddOffset(v, 4);

         // Set vertex buffers
         rdp.vtxbuf = rdp.vtx1;        // copy from v to rdp.vtx1
         rdp.vtxbuf2 = rdp.vtx2;
         rdp.vtx_buffer = 0;
         rdp.n_global = 3;
         memcpy (rdp.vtxbuf, v, sizeof(VERTEX)*3);
         do_triangle_stuff_2 (0);
         rdp.tri_n ++;

         rdp.vtxbuf = rdp.vtx1;        // copy from v to rdp.vtx1
         rdp.vtxbuf2 = rdp.vtx2;
         rdp.vtx_buffer = 0;
         rdp.n_global = 3;
         memcpy (rdp.vtxbuf, v+1, sizeof(VERTEX)*3);
         do_triangle_stuff_2 (0);
         rdp.tri_n ++;
         rdp.update |= UPDATE_ZBUF_ENABLED | UPDATE_VIEWPORT;

         if (settings.fog && (rdp.flags & FOG_ENABLED))
            grFogMode (GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT);

      }
      a = rdp.pc[rdp.pc_i] & BMASK;
      cmd0 = ((uint32_t*)gfx.RDRAM)[a>>2]; //check next command
      if (( (cmd0>>24) == 0xBD ) || ( (cmd0>>24) == 0xBE ))
         rdp.pc[rdp.pc_i] = (a+8) & BMASK;
      else
         return;
   }while(1);
}
