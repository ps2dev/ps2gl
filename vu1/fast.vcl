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

kInputQPerV         .equ           3
kOutputQPerV        .equ           3

     .init_vf_all
     .init_vi_all

     .name          vsmFast

     --enter
     --endenter

     ; ------------------------ initialization ---------------------------------

     ; there should be from 0 to 3 directional lights, no other types

     load_vert_xfrm vert_xform

     ; build a 3x3 matrix with light directions as the rows and a 3x3 matrix with
     ; the 3 cols the light diffuse colors
     ; find the constant color terms (sum of emissive and ambient terms)

     load_mat_emm   constant_color

     ; initialize the direction and diffuse color matrices
     move.xyz       light_dirs[0], vf00
     move.xyz       light_dirs[1], vf00
     move.xyz       light_dirs[2], vf00
     move.xyz       light_colors[0], vf00
     move.xyz       light_colors[1], vf00
     move.xyz       light_colors[2], vf00

     init_dlt_loop
     load_mat_amb   material_amb
     ibeq           num_dir_lights, vi00, finish_init_lid

     load_mat_diff  material_diff
     load_o2wt      obj_to_world_transpose

     load_lt_pos    light_dirs[0]
     mul_vec_mat_33 light_dirs[0], obj_to_world_transpose, light_dirs[0]
     load_lt_diff   light_colors[0]
     mul.xyz        light_colors[0], light_colors[0], material_diff
     load_lt_amb    light_amb
     mul.xyz        local_amb, material_amb, light_amb
     add.xyz        constant_color, constant_color, local_amb

     isubiu         num_dir_lights, num_dir_lights, 1
     ibeq           num_dir_lights, vi00, finish_init_lid
     next_dir_light
     load_lt_pos    light_dirs[1]
     mul_vec_mat_33 light_dirs[1], obj_to_world_transpose, light_dirs[1]
     load_lt_diff   light_colors[1]
     mul.xyz        light_colors[1], light_colors[1], material_diff
     load_lt_amb    light_amb
     mul.xyz        local_amb, material_amb, light_amb
     add.xyz        constant_color, constant_color, local_amb

     isubiu         num_dir_lights, num_dir_lights, 1
     ibeq           num_dir_lights, vi00, finish_init_lid
     next_dir_light
     load_lt_pos    light_dirs[2]
     mul_vec_mat_33 light_dirs[2], obj_to_world_transpose, light_dirs[2]
     load_lt_diff   light_colors[2]
     mul.xyz        light_colors[2], light_colors[2], material_diff
     load_lt_amb    light_amb
     mul.xyz        local_amb, material_amb, light_amb
     add.xyz        constant_color, constant_color, local_amb

finish_init_lid:

     load_glob_amb  global_amb
     mul.xyz        local_amb, material_amb, global_amb
     add.xyz        constant_color, constant_color, local_amb

     transpose_33ip light_dirs

     ; the alpha of a vert is the material diffuse alpha
     load_mat_diff  material_diff, w
     loi            128.0
     muli.w         color, material_diff, i
     loi            255.0
     minii.w        color, color, i

     ; use constant_color add to do an ftoi0 on the color
     loi            12582912.0
     addi.xyz       constant_color, constant_color, i

     ; convert alpha to 0-bit fixed point
     addi.w         color, color, i

     init_constants
     init_clip_cnst

     ; make translation matrix
     sub            trans[0], vf00, vf00
     sub            trans[1], vf00, vf00
     sub            trans[2], vf00, vf00
     maxw.x         trans[0], trans[0], vf00
     maxw.y         trans[1], trans[1], vf00
     maxw.z         trans[2], trans[2], vf00
     move.xyz       trans[3], gs_offsets
     move.w         trans[3], vf00

     ; new xform

     mul_vec_mat_44 new_xform[0], trans, vert_xform[0]
     mul_vec_mat_44 new_xform[1], trans, vert_xform[1]
     mul_vec_mat_44 new_xform[2], trans, vert_xform[2]
     mul_vec_mat_44 new_xform[3], trans, vert_xform[3]

     --cont

     ; -------------------- transform & texture loop ---------------------------

main_loop_lid:

     init_io_loop
     init_out_buf

     set_strip_adcs

xform_loop_lid:
     --LoopCS       1,3

     ; xform/clip vertex

     load_vert      vert

     xform_vert     xformed_vert, new_xform, vert
     ftoi4.xyz      gs_vert, xformed_vert

     load_strip_adc strip_adc
     set_adc_s      gs_vert, strip_adc

     store_xyzf     gs_vert

     ; lighting

     load_normal    normal
     mul_vec_mat_33 cosines, light_dirs, normal
     max.xyz        cosines, cosines, vf00
     mul_vec_mat_33 color, light_colors, cosines
     add.xyz        color, color, constant_color
     miniw.xyz      color, color, gs_offsets[w]
     store_rgba     color

     ; texture coords

     load_stq       tex_stq
     xform_tex_stq  tex_stq, tex_stq, q ; q is from normalize_3
     store_stq      tex_stq

     next_io
     loop_io        xform_loop_lid

     ; -------------------- done! -------------------------------

done_lid:

     ; ---------------- kick packet to GS -----------------------

     kick_to_gs

     --cont

     b    main_loop_lid

.END ; for gasp
