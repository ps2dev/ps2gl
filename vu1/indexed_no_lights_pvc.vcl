/*  Indexed, per-vertex color, no lights  */

     #include       "vu1_mem_indexed.h"

     .include       "db_in_sb_out.i"
     .include       "math.i"
     .include       "lighting.i"
     .include       "clip_cull.i"
     .include       "geometry.i"
     .include       "io.i"
     .include       "general.i"

kInputQPerV         .equ           4
kOutputQPerV        .equ           3

     .init_vf_all
     .init_vi_all

     .name          vsmIndexedPVC

     --enter
     --endenter

     ; ------------------------ init ---------------------------------

     load_vert_xfrm vert_xform
     init_constants
     init_clip_cnst

main_loop_lid:

     ; -------------------- set up decompression ----------------------

     xtop           buffer_top
     iaddiu         next_output, vi00, kOutputGeomStart
     iaddiu         input_start, buffer_top, kInputGeomStart

     ; num indices / vertex count
     ilw.y          num_indices_d2, kNumIndicesD2(buffer_top)
     ilw.z          num_indices,    kNumIndices(buffer_top)
     ilw.x          num_vertices,   kNumVertices(buffer_top)

     ; VI mask for low 8 bits of first index (needed by iand)
     iaddiu         first_index_mask, vi00, 0xff

    ; stride constants for index unpack (kInputQPerV == 4 here)
    loi            253.0
    maxi.w         index_constants, vf00, i      ; keep w=253.0 (used in the 2nd-index trick)
    loi            4.0
    maxi.z         index_constants, vf00, i      ; z=4 → offsets are 4 * index
    loi            255.0
    maxi.y         index_constants, vf00, i      ; y=255 (8-bit mask for second index path)

     ; decompression pointers
     iaddiu         next_index, vi00, kInputGeomStart
     iadd           next_index, next_index, buffer_top
     iadd           last_index, next_index, num_indices_d2

     ; giftag
     lq             gif_tag, kGifTag(vi00)
     mtir           eop, gif_tagx
     ior            eop, eop, num_indices
     mfir.x         gif_tag, eop
     mfir.w         gif_tag, next_output
     sq             gif_tag, kOutputBufStart(vi00)

    ; -------- figure out where the color lane lives --------
    ; Layout for this kernel = [ V(0) | N(1) | STQ(2) | PVC(3) ]
    ; PVC is lane 3 → base is +3 * num_vertices

    iadd           color_start, input_start, num_vertices   ; +1
    iadd           color_start, color_start, num_vertices   ; +2
    iadd           color_start, color_start, num_vertices   ; +3


     ; alpha policy: use material diffuse alpha (matches constant-color path)
     loi            128.0
     load_mat_diff  vert_color, w
     muli.w         vert_color, vert_color, i
     loi            255.0
     minii.w        vert_color, vert_color, i
     ftoi0.w        vert_color, vert_color
     maxi.w         max_color_val, vf00, i

     ; wait for other buffers
     iaddiu         zero_giftag, vi00, kGifTag
     xgkick         zero_giftag

xform_loop_lid:          --LoopCS 1,3

    ; first index
    ilw.w          first_index, 0(next_index)
    iand           first_index, first_index, first_index_mask

    ; first_offset = first_index * 4   (was *3)
    iadd           first_offset, first_index, first_index    ; *2
    iadd           first_offset, first_offset, first_offset  ; *4


     ; second index in packed word
     lqi.w          indices, (next_index++)
     addy.w         second_ind, indices, index_constants[y]
     mtir           second_index, second_ind[w]
     mulz.w         second_off, indices, index_constants[z]
     add.w          second_off, second_off, index_constants[w]
     mtir           second_offset, second_off[w]

     .macro         do_vert
         ; vertex
         iadd           next_input, first_offset, input_start
         load_vert      vert
         xform_vert     xformed_vert, vert_xform, vert
         vert_to_gs     gs_vert, xformed_vert
         clip_vert      xformed_vert
         fcand          vi01, 0x003ffff
         iand           vi01, vi01, do_clipping
         set_adc_fs     gs_vert, vi00
         store_xyzf     gs_vert

         ; color (PVC 0..1 → 0..255, alpha from mat diffuse)
         iadd           next_color, first_index, color_start
         lq             vert_color, (next_color)
         loi            255.0
         muli.xyz       vert_color, vert_color, i
         miniw.xyz      vert_color, vert_color, max_color_val[w]
         ftoi0.xyz      vert_color, vert_color
         store_rgba     vert_color

         ; texcoords (if enabled, STQ lane is present)
         load_stq       tex_stq
         xform_tex_stq  tex_stq, tex_stq, q
         store_stq      tex_stq
     .endm

     ; first vertex
     do_vert

     ; second vertex (just swap to second_* values)
     iadd           first_offset, second_offset, vi00
     iadd           first_index,  second_index,  vi00
     iaddiu         next_output,  next_output,   kOutputQPerV
     do_vert

     ; end loop
     next_o
     ibne           next_index, last_index, xform_loop_lid

     kick_to_gs
     --cont
     b              main_loop_lid

.END
