/*     Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

            This file is subject to the terms and conditions of the GNU Lesser
       General Public License Version 2.1. See the file "COPYING" in the
       main directory of this archive for more details.                             */

     ;
     ;
     ; clipping
     ;
     ;

     ; ---------------------------------------------------
     ; initialize various clipping constants and clip_mask
     ;
     ; outputs:     clip_scales
     ; modifies:    i

     .macro         init_clip_cnst
     ; clip scales and value to clip against (2048 - arbitrary)
     lq.xyz         clip_scales, kClipInfo(vi00)
     loi            2048.0
     maxi.w         clip_scales, vf00, i

     ilw.w          do_clipping, kClipInfo(vi00)
     .endm

     ; ---------------------------------------------------
     ; do a clipping test on vertex.
     ;
     ; params:      vert           - the vertex
     ; outputs:     vi01           - contains clip result (1 or 0)

     .macro         clip_vert      vert
     mul.xyz        clip_vert\@, \vert, clip_scales
     clipw.xyz      clip_vert\@, clip_scales[w]
     .endm

     ;
     ;
     ; face culling
     ;
     ;

     ; ---------------------------------------------------
     ; init backface culling.
     ;
     ; outputs:     z_sign_mask
     ;              bfc_multiplier - 1.0 to cull back-facing polys, else -1.0

     .macro         init_bfc
     ilw.w          z_sign_mask, kBackFaceCullMult(vi00)
     lq.w           bfc_multiplier, kBackFaceCullMult(vi00)
     .endm

     ; ---------------------------------------------------
     ; backface cull a triangle
     ;
     ; inputs:      bfc_multiplier, z_sign_mask
     ; params:      xformed_vert_? - the transformed vertices of the triangle
     ; outputs:     z_sign         - the sign of the tri normal's z component

     .macro         bfc_tri        xformed_vert_1, xformed_vert_2, xformed_vert_3
     ; this screen triangle's normal
     sub.xyz        delta_1\@, \xformed_vert_1, \xformed_vert_2
     sub.xyz        delta_2\@, \xformed_vert_3, \xformed_vert_2
     ; bfc_multiplier is 1 to cull back-facing polys, -1 for front
     mulw.xyz       delta_1\@, delta_1\@, bfc_multiplier
     opmula.xyz     acc, delta_1\@, delta_2\@
     opmsub.xyz     bfc_normal\@, delta_2\@, delta_1\@

     ; get sign of normal
     fmand          z_sign, z_sign_mask
     .endm

     ; ---------------------------------------------------
     ; init backface culling for strips (where each loop iteration relies on the
     ; previous iteration).
     ;
     ; outputs:     old_delta      - really only assigns these two values
     ;              old_vert         to make vcl happy
     ;              z_sign_switch  - a switch to flip the sense of the bfc test
     ;                               for each tri in a strip
     ;              <"init_bfc" called>

     .macro         init_bfc_strip
     addx.xyz       old_delta, vf00, vf00[x]
     addx.xyz       old_vert, vf00, vf00[x]
     iaddiu         z_sign_switch, vi00, 0x20

     init_bfc
     .endm

     ; ---------------------------------------------------
     ; backface cull a strip vertex.  This is different from a triangle
     ; because the backfacing direction flips at each vertex, and
     ; because one vertex is processed per loop iteration.
     ;
     ; inputs:      old_vert, old_delta, bfc_multiplier,
     ;              z_sign_mask, z_sign_switch
     ; params:      xformed_vert   - the current transformed vertex..duh
     ;              strip_adc      - set by "set_strip_adcs" and loaded by "load_strip_adc"
     ; outputs:     z_sign

     .macro         bfc_strip_vert xformed_vert, strip_adc
     ; this screen triangle's normal
     sub.xyz        delta\@, old_vert, \xformed_vert
     opmula.xyz     acc, delta\@, old_delta
     opmsub.xyz     bfc_normal\@, old_delta, delta\@

     ; get sign of normal
     fmand          z_sign, z_sign_mask
     ; flip the sign every other time we do this
     isub           z_sign, z_sign, z_sign_switch
     iand           z_sign, z_sign, z_sign_mask
     ; next time flip the other way
     isub           z_sign_switch, z_sign_mask, z_sign_switch
     ; reset if starting new strip
     iand           strip_flip\@, \strip_adc, z_sign_mask
     ior            z_sign_switch, z_sign_switch, strip_flip\@

     ; bfc_multiplier is 1 to cull back-facing polys, -1 for front
     mulw.xyz       old_delta, delta\@, bfc_multiplier
     mulw.xyz       old_vert, \xformed_vert, vf00
     .endm

     ;
     ;
     ; setting the adc bit
     ;
     ;

     ; ---------------------------------------------------
     ; Set the adc bits at the beginnings of strips in this buffer.
     ; Multiple strips can be packed in each vu memory buffer, so this
     ; routine is called to set the adc bits of vertices on strip boundaries
     ; so that the strips don't all run together.
     ;
     ; inputs:      buffer_top, kInputStart, kStripADCs

     ; internal; used below
     .macro         _set_adcs      field
     mtir           offset, strip_boundaries[\field]
     iand           stop_bit, offset, stop_bit_mask
     ibeq           stop_bit, stop_bit_mask, adcLoop_done_lid
     iand           second_adc, offset, second_adc_mask
     iand           offset, offset, offsetMask
     iadd           offset, offset, firstVert

     isw.w          first_adc, 0(offset)
     isw.w          second_adc, kInputQPerV(offset)
     .endm

     ; the real macro

     .macro         set_strip_adcs
     iaddiu         firstVert, buffer_top, kInputStart
     iaddiu         adcsPtr, buffer_top, kStripADCs
     iaddiu         lastAdcsPtr, adcsPtr, 4

     ; see CGeneralRenderer::XferBufferHeader for format
     iaddiu         offsetMask, vi00, 0x3ff
     iaddiu         second_adc_mask, vi00, 0x800
     iaddiu         stop_bit_mask, vi00, 0x400

     iaddiu         first_adc, vi00, 0x20

     adcLoop_lid:

     lq             strip_boundaries, 0(adcsPtr)
     ; get the offsets
     ftoi0          strip_boundaries, strip_boundaries

     _set_adcs      x
     _set_adcs      y
     _set_adcs      z
     _set_adcs      w

     ; loop
     iaddiu         adcsPtr, adcsPtr, 1
     ibne           adcsPtr, lastAdcsPtr, adcLoop_lid

     adcLoop_done_lid:
     .endm

     ; ---------------------------------------------------
     ; set the adc bit based on (f)rustum cull, (b)ackface cull,
     ; and (s)trip boundaries (which implies that this macro is intended
     ; for strips).
     ;
     ; inputs:      vi01           - clip results
     ;              z_sign         - sign of face normal[z]
     ; params:      vert           - the vertex in question
     ;              strip_adc      - set by "set_strip_adcs" and loaded by "load_strip_adc"

     .macro         set_adc_fbs    vert, strip_adc
     ; clip and face culling flags
     ior            new_adc\@, vi01, z_sign
     ior            new_adc\@, new_adc\@, \strip_adc
     iaddiu         new_adc\@, new_adc\@, 0x7fff
     mfir.w         \vert, new_adc\@
     .endm

     ; ---------------------------------------------------
     ; set the adc bit based on (f)rustum cull and (s)trip boundaries.
     ;
     ; inputs:      vi01           - clip results
     ; params:      vert           - the vertex in question
     ;              strip_adc      - set by "set_strip_adcs" and loaded by "load_strip_adc"

     .macro         set_adc_fs     vert, strip_adc
     ; clip and face culling flags
     ior            new_adc\@, vi01, \strip_adc
     iaddiu         new_adc\@, new_adc\@, 0x7fff
     mfir.w         \vert, new_adc\@
     .endm

     ; ---------------------------------------------------
     ; set the adc bit based on (f)rustum and (b)ackface cull.
     ;
     ; inputs:      vi01           - clip results
     ;              z_sign         - sign of face normal[z]
     ; params:      vert           - the vertex in question

     .macro         set_adc_fb     vert
     ; clip and face culling flags
     ior            new_adc\@, vi01, z_sign
     iaddiu         new_adc\@, new_adc\@, 0x7fff
     mfir.w         \vert, new_adc\@
     .endm

     ; ---------------------------------------------------
     ; set the adc bit based on (s)trip boundaries.
     ;
     ; params:      vert           - the vertex in question
     ;              strip_adc      - set by "set_strip_adcs" and loaded by "load_strip_adc"

     .macro         set_adc_s      vert, strip_adc
     ; clip and face culling flags
     iaddiu         new_adc\@, \strip_adc, 0x7fff
     mfir.w         \vert, new_adc\@
     .endm
