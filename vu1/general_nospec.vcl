/*     Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

            This file is subject to the terms and conditions of the GNU Lesser
       General Public License Version 2.1. See the file "COPYING" in the
       main directory of this archive for more details.                             */

     #include       "vu1_mem_linear.h"

     .include       "db_in_db_out.i"
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

     .name          vsmGeneralNoSpec

     --enter
     --endenter

     ; ------------------------ initialization ---------------------------------

     load_vert_xfrm vert_xform

     --cont

     ; -------------------- transform & texture loop ---------------------------

main_loop_lid:

     init_constants
     init_clip_cnst

     init_io_loop
     init_out_buf

     set_strip_adcs

     get_cnst_color const_color

     init_bfc_strip

xform_loop_lid:          --LoopCS 1,3

     ; xform/clip vertex

     load_vert      vert

     xform_vert     xformed_vert, vert_xform, vert
     vert_to_gs     gs_vert, xformed_vert

     load_strip_adc strip_adc
     bfc_strip_vert xformed_vert, strip_adc
     clip_vert      xformed_vert
     fcand          vi01, 0x003ffff
     iand           vi01, vi01, do_clipping
     set_adc_fbs    gs_vert, strip_adc

     store_xyzf     gs_vert

     ; constant color

     store_rgb      const_color

     ; texture coords

     load_stq       tex_stq
     xform_tex_stq  tex_stq, tex_stq, q ; q is from normalize_3
     store_stq      tex_stq

     next_io
     loop_io        xform_loop_lid

     ; -------------------- lighting -------------------------------

lighting_lid:

     load_mat_amb   material_amb
     load_mat_diff  material_diff

     ; ---------- directional lights -----------------

     init_dlt_loop
     ibeq           num_dir_lights, vi00, pt_lights_lid
     get_ones_vec   ones

dir_light_loop_lid:

     init_io_loop

     load_lt_amb    light_amb
     load_lt_diff   light_diff

     ; transform light direction into object space
     load_lt_pos    vert_to_light
     load_o2wt      obj_to_world_transpose
     mul_vec_mat_33 vert_to_light, obj_to_world_transpose, vert_to_light

dir_light_vert_loop_lid: --LoopCS  1,3

     load_normal    normal

     get_diff_term  acc, vert_to_light, normal, light_diff, material_diff, dot3_to_z, z, ones
     add_amb_term   vert_color, light_amb, material_amb

     ; add to previous lighting calculations (other lights, global amb + emission)
     accum_rgb      vert_color, vert_color

     store_rgb      vert_color

     next_io
     loop_io        dir_light_vert_loop_lid

     ; loop over lights

     next_dir_light
     loop_dir_lts   dir_light_loop_lid

     ; ---------- point lights -----------------

pt_lights_lid:

     init_plt_loop
     ibeq           num_pt_lights, vi00, done_lid
     get_ones_vec   ones

pt_light_loop_lid:

     init_io_loop

     load_lt_amb    light_amb
     load_lt_diff   light_diff
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

     add_amb_term   vert_color, light_amb, material_amb

     atten_color    vert_color, vert_color, atten

     accum_rgb      vert_color, vert_color

     store_rgb      vert_color

     next_io
     loop_io        pt_light_vert_loop_lid

     next_pt_light
     loop_pt_lts    pt_light_loop_lid

     ; -------------------- done! -------------------------------

done_lid:

     ; clamp and convert to fixed-point
     finish_colors

     ; ---------------- kick packet to GS -----------------------

     kick_to_gs

     --cont

     b    main_loop_lid

.END ; for gasp
