/*     Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

            This file is subject to the terms and conditions of the GNU Lesser
       General Public License Version 2.1. See the file "COPYING" in the
       main directory of this archive for more details.                             */

     #include       "vu1_mem_indexed.h"

     .include       "db_in_sb_out.i"
     .include       "math.i"
     .include       "lighting.i"
     .include       "clip_cull.i"
     .include       "geometry.i"
     .include       "io.i"
     .include       "general.i"

kInputQPerV         .equ           3
kOutputQPerV        .equ           3

     .init_vf_all
     .init_vi_all

     .name          vsmIndexed

     --enter
     --endenter

     ; ------------------------ initialization ---------------------------------

     load_vert_xfrm vert_xform

     --cont


main_loop_lid:

     ; -------------------- lighting -------------------------------

     .macro         accum_rgb      output, rgb
     lq.xyz         total_rgb\@, 0(next_color_acc)
     add.xyz        \output, total_rgb\@, \rgb
     .endm

lighting_lid:

     load_mat_amb   material_amb
     load_mat_diff  material_diff
     load_mat_spec  material_spec

     load_vert_eye  vert_to_eye

     ; ---------- directional lights -----------------

     init_dlt_loop
     ibeq           num_dir_lights, vi00, pt_lights_lid
     get_ones_vec   ones

dir_light_loop_lid:

     init_i_loop

     ; color accumulation buffer
     iaddiu         next_color_acc, buffer_top, kTempAreaStart

     load_lt_amb    light_amb
     load_lt_diff   light_diff
     load_lt_spec   light_spec
     mul.xyz        local_spec, light_spec, material_spec

     ; transform light direction into object space
     load_lt_pos    vert_to_light
     load_o2wt      obj_to_world_transpose
     mul_vec_mat_33 vert_to_light, obj_to_world_transpose, vert_to_light

     ; in non-local viewer mode for infinite lights, the half-angle vec is fixed
     get_half_angle half_angle, vert_to_eye, vert_to_light

dir_light_vert_loop_lid: --LoopCS  1,3

     load_normal    normal

     get_diff_term  acc, vert_to_light, normal, light_diff, material_diff, dot3_to_z, z, ones
     add_spec_term  acc, half_angle, normal, local_spec, dot3_to_w_slow, w
     add_amb_term   vert_color, light_amb, material_amb

     ; add to previous lighting calculations (other lights, global amb + emission)
     accum_rgb      vert_color, vert_color

     sqi.xyz        vert_color, (next_color_acc++)

     next_i
     loop_io        dir_light_vert_loop_lid

     ; loop over lights

     next_dir_light
     loop_dir_lts   dir_light_loop_lid

     ; ---------- point lights -----------------

pt_lights_lid:

     init_plt_loop
     ibeq           num_pt_lights, vi00, done_lighting_lid
     get_ones_vec   ones

pt_light_loop_lid:

     init_i_loop

     ; color accumulation buffer
     iaddiu         next_color_acc, buffer_top, kTempAreaStart

     load_lt_amb    light_amb
     load_lt_diff   light_diff
     load_lt_spec   light_spec
     mul.xyz        local_spec, light_spec, material_spec
     load_lt_atten  atten_coeff

     ; transform light position to object space
     load_lt_pos    light_pos
     load_w2o       world_to_obj
     mul_pt_mat_34  light_pos, world_to_obj, light_pos

pt_light_vert_loop_lid:

     --LoopCS  1,3
     --LoopExtra 5

     load_normal    normal
     load_vert      vert

     get_v2l_atten  vert_to_light, light_pos, vert, atten, atten_coeff, ones
     get_diff_term  acc, vert_to_light, normal, light_diff, material_diff, dot3_to_w, w

     get_half_angle half_angle, vert_to_eye, vert_to_light
     add_spec_term  acc, half_angle, normal, local_spec, dot3_to_w, w

     add_amb_term   vert_color, light_amb, material_amb

     atten_color    vert_color, vert_color, atten

     accum_rgb      vert_color, vert_color

     sqi.xyz        vert_color, (next_color_acc++)

     next_i
     loop_io        pt_light_vert_loop_lid

     next_pt_light
     loop_pt_lts    pt_light_loop_lid

done_lighting_lid:

     ; -------------------- transform & texture loop ---------------------------

     init_constants
     init_clip_cnst

     ; init_io_loop kInputGeomStart, kTempAreaStart
     xtop           buffer_top

     iaddiu         next_output, vi00, kOutputGeomStart
     iaddiu         input_start, buffer_top, kInputGeomStart
     iaddiu         color_start, buffer_top, kTempAreaStart

     ; set up index-decompression

     iaddiu         next_index, buffer_top, kInputGeomStart
     iaddiu         first_index_mask, vi00, 0xff

     ; constant to shift right
     loi            253.0 ; = 2^8 - 3 = shift fraction part right 8 bits
     maxi.w         index_constants, vf00, i
     loi            3.0
     maxi.z         index_constants, vf00, i
     loi            255.0
     maxi.y         index_constants, vf00, i

     ; init_out_buf

     ; get num indices (stored as num/2)
     ilw.y          num_indices_d2, kNumIndicesD2(buffer_top)
     ; 2 indices per w-field
     iadd           last_index, next_index, num_indices_d2
     ; get real # of indices
     ilw.z          num_indices, kNumIndices(buffer_top)

     ; set nloop and copy giftag
     lq             gif_tag, kGifTag(vi00)
     mtir           eop, gif_tagx
     ior            eop, eop, num_indices
     mfir.x         gif_tag, eop
     mfir.w         gif_tag, next_output
     sq             gif_tag, kOutputBufStart(vi00)

     ; for the final color conversion
     ; the alpha value of a vert is the mat diffuse alpha
     loi            128.0
     load_mat_diff  vert_color, w
     muli.w         vert_color, vert_color, i
     loi            255.0
     minii.w        vert_color, vert_color, i
     ftoi0.w        vert_color, vert_color
     maxi.w         max_color_val, vf00, i

;    init_bfc_strip

     ; wait for any other buffers to finish..
     iaddiu         zero_giftag, vi00, kGifTag
     xgkick         zero_giftag

xform_loop_lid:          --LoopCS 1,3

     ; get first index
     ilw.w          first_index, 0(next_index)
     iand           first_index, first_index, first_index_mask
     iadd           first_offset, first_index, first_index
     iadd           first_offset, first_offset, first_index

     ; get second index
     ; the first and second indices are in the form 0x3f80ssff
     lqi.w          indices, (next_index++)
     addy.w         second_ind, indices, index_constants[y]
     mtir           second_index, second_ind[w]

     mulz.w         second_off, indices, index_constants[z]
     add.w          second_off, second_off, index_constants[w]
     mtir           second_offset, second_off[w]

     .macro         do_vert
     ; xform/clip vertex

     load_vert      vert

     xform_vert     xformed_vert, vert_xform, vert
     vert_to_gs     gs_vert, xformed_vert

;    load_strip_adc strip_adc
;    bfc_strip_vert xformed_vert, vi00
     clip_vert      xformed_vert
     fcand          vi01, 0x003ffff
     iand           vi01, vi01, do_clipping
;    set_adc_fbs    gs_vert, vi00
     set_adc_fs     gs_vert, vi00

     store_xyzf     gs_vert

     ; color

     lq.xyz         vert_color, (next_color)
     miniw.xyz      vert_color, vert_color, max_color_val[w]
     ftoi0.xyz      vert_color, vert_color
     store_rgba     vert_color

     ; texture coords

     load_stq       tex_stq
     xform_tex_stq  tex_stq, tex_stq, q ; q is from normalize_3
     store_stq      tex_stq
     .endm

     ; ---------- first vert -----------------

     iadd           next_input, first_offset, input_start
     iadd           next_color, first_index, color_start
     do_vert

     ; ---------- second vert -----------------

     iadd           next_input, second_offset, input_start
     iadd           next_color, second_index, color_start
     iaddiu         next_output, next_output, kOutputQPerV
     do_vert

     ;  --------- end loop ---------------

     next_o

     ibne           next_index, last_index, xform_loop_lid

     ; ---------------- kick packet to GS -----------------------

     kick_to_gs

     --cont

     b    main_loop_lid

.END ; for gasp
