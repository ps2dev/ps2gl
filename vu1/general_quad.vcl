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

     .name          vsmGeneralQuad

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

     init_bfc

     get_cnst_color const_color

     iaddiu         adc_bit, vi00, 0x7fff
     iaddiu         adc_bit, adc_bit, 1

xform_loop_lid:          --LoopCS 1,3

     ; the first two are never drawn (always have adc bit set)
     do_bfc_vert    0, 0, xformed_vert_1, gs_vert_1
     clip_vert      xformed_vert_1
     mfir.w         gs_vert_1, adc_bit
     store_xyzf     gs_vert_1, 0

     do_bfc_vert    kInputQPerV, kOutputQPerV, xformed_vert_2, gs_vert_2
     clip_vert      xformed_vert_2
     mfir.w         gs_vert_2, adc_bit
     store_xyzf     gs_vert_2, kOutputQPerV

     ; We are loading the 4th vertex of the quad, as the 3rd vertex of the triangle strip
     do_bfc_vert    kInputQPerV+kInputQPerV+kInputQPerV, kOutputQPerV+kOutputQPerV, xformed_vert_3, gs_vert_3
     clip_vert      xformed_vert_3

     ; We are loading the 3rd vertex of the quad, as the 4th vertex of the triangle strip
     do_bfc_vert    kInputQPerV+kInputQPerV, kOutputQPerV+kOutputQPerV+kOutputQPerV, xformed_vert_4, gs_vert_4
     clip_vert      xformed_vert_4


     ; backface/frontface cull
     bfc_tri        xformed_vert_1, xformed_vert_2, xformed_vert_4

     ; clip test last four vertices
     fcand          vi01, 0x0ffffff
     iand           vi01, vi01, do_clipping

     ; draw this quad?
     ior            new_adc_bit, vi01, z_sign
     iaddiu         new_adc_bit, new_adc_bit, 0x7fff

     mfir.w         gs_vert_3, new_adc_bit
     store_xyzf     gs_vert_3, kOutputQPerV+kOutputQPerV

     mfir.w         gs_vert_4, new_adc_bit
     store_xyzf     gs_vert_4, kOutputQPerV+kOutputQPerV+kOutputQPerV

     ; Swap normals of vertex 3 and 4
     lq.xyz         tmp3, 1+kInputQPerV+kInputQPerV(next_input)
     lq.xyz         tmp4, 1+kInputQPerV+kInputQPerV+kInputQPerV(next_input)
     sq.xyz         tmp3, 1+kInputQPerV+kInputQPerV+kInputQPerV(next_input)
     sq.xyz         tmp4, 1+kInputQPerV+kInputQPerV(next_input)

     next_io        4

     loop_io        xform_loop_lid

     ; -------------------- lighting -------------------------------

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

     init_io_loop

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
