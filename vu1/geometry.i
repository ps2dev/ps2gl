/*     Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

            This file is subject to the terms and conditions of the GNU Lesser
       General Public License Version 2.1. See the file "COPYING" in the
       main directory of this archive for more details.                             */

     ; ---------------------------------------------------
     ; initializes some constants for transforming vertices and
     ; clamping colors (FIXME)

     .macro         init_constants
     ; gs offsets to center xformed vertex in gs coord space, also color clamping constant
     loi            2047.5
     addi.xy        gs_offsets, vf00, i
     lq.w           temp, kClipToGsDepthOffset(vi00)
     mr32.z         gs_offsets, temp
     loi            12583167.0 ; clamp colors to 255 (255 + 12582912)
     maxi.w         gs_offsets, vf00, i

     fcset          0x000
     .endm

     ; ---------------------------------------------------
     ; load/store
     ; These next macros load and store the next normal, tex coord, etc.
     ; and are pretty self explanatory.
     ;
     ; params:      offset         - the offset in quads to add to the
     ;                               pointer before loading.  Default is 0.

     .macro         load_vert      vert, offset=0
     lq.xyz         \vert, \offset(next_input)
     .endm
     .macro         load_normal    normal, offset=0
     lq.xyz         \normal, 1+\offset(next_input)
     .endm
     .macro         load_stq       coords, offset=0
     lq.xyz         \coords, 2+\offset(next_input)
     .endm
     .macro         load_rgba      rgba, offset=0
     lq             \rgba, 1+\offset(next_output)
     .endm
     .macro         load_rgb       rgb, offset=0
     lq.xyz         \rgb, 1+\offset(next_output)
     .endm

     .macro         store_xyz      xyz, offset=0
     sq.xyz         \xyz, 2+\offset(next_output)
     .endm
     .macro         store_xyzf     xyzf, offset=0
     sq             \xyzf, 2+\offset(next_output)
     .endm
     .macro         store_stq      tex, offset=0
     sq.xyz         \tex, 0+\offset(next_output)
     .endm
     .macro         store_rgb      rgb, offset=0
     sq.xyz         \rgb, 1+\offset(next_output)
     .endm
     .macro         store_rgba     rgba, offset=0
     sq             \rgba, 1+\offset(next_output)
     .endm

     ; ---------------------------------------------------
     ; loads the vertex transform from memory
     ;
     ; params:      xfrm           - the base name of the matrix (will be xfrm[0-3])

     .macro         load_vert_xfrm xfrm
     lq             \xfrm[0], kVertexXfrm+0(vi00)
     lq             \xfrm[1], kVertexXfrm+1(vi00)
     lq             \xfrm[2], kVertexXfrm+2(vi00)
     lq             \xfrm[3], kVertexXfrm+3(vi00)
     .endm

     ; ---------------------------------------------------
     ; load the adc value that was set by "set_strip_adcs"
     ;
     ; params       strip_adc      - the name of the register to assign the value to

     .macro         load_strip_adc strip_adc
     ilw.w          \strip_adc, 0(next_input)
     .endm

     ;
     ;
     ; transforming
     ;
     ;

     ; ---------------------------------------------------
     ; apply the given perspective transform to the given vertex.
     ;
     ; params:      xformed_vert   - where to put result
     ;              vert_xform     - name of xform, where columns are vert_xform[0-3]
     ;              vert           - the vertex to transform (NOTE: w field is forced to 1.0)
     ; modifies:    q

     .macro         xform_vert     xformed_vert, vert_xform, vert
     mul_pt_mat_44  \xformed_vert, \vert_xform, \vert
     div            q, vf00w, \xformed_vert[w]
     mulq.xyz       \xformed_vert, \xformed_vert, q
     ; FIXME: visible vertices are now in range (+-320, +-112, +-2^24-1)
     .endm

     ; ---------------------------------------------------
     ; convert a transformed vertex to gs coords and format (add offset
     ; and convert to fixed-point).
     ;
     ; inputs:      gs_offsets[xyz] - translation to gs coord space
     ; params:      gs_vert        - the result
     ;              vert           - the vertex to convert

     .macro         vert_to_gs     gs_vert, vert
     ; add screen offsets to xformed vertex
     add.xyz        \gs_vert, \vert, gs_offsets
     ; convert to 4-bit fixed-point
     ftoi4.xyz      \gs_vert, \gs_vert
     .endm

     ; ---------------------------------------------------
     ; Do the perspective multiply on the texture coordinates for perspective-
     ; correct textures.
     ;
     ; params:      output         - the result
     ;              tex_stq        - the texture stq.  q (the z field) must be 1.0.
     ;              q              - the recip of the w field of the transformed vertex
     ;                               (q in the case of "xform_vert")

     .macro         xform_tex_stq  output, tex_stq, q
     ; normalize  stq
     mulq.xyz       \output, \tex_stq, \q
     .endm
