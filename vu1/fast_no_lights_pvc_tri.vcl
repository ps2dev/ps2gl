/*  Per-vertex color, no lights, triangles  -- based off of fast_no_lights */

     #include       "vu1_mem_linear.h"

     .include       "db_in_db_out.i"
     .include       "math.i"
     .include       "lighting.i"
     .include       "clip_cull.i"
     .include       "geometry.i"
     .include       "io.i"
     .include       "general.i"

kInputQPerV         .equ        4
kOutputQPerV        .equ        3

    .init_vf_all
    .init_vi_all

    .name               vsmFastNoLightsPVCTri

    --enter
    --endenter

    ; ------------------------ initialization ---------------------------------

    load_vert_xfrm      vert_xform

    init_constants

    sub                 trans[0], vf00, vf00
    sub                 trans[1], vf00, vf00
    sub                 trans[2], vf00, vf00
    maxw.x              trans[0], trans[0], vf00
    maxw.y              trans[1], trans[1], vf00
    maxw.z              trans[2], trans[2], vf00
    move.xyz            trans[3], gs_offsets
    move.w              trans[3], vf00

    mul_vec_mat_44      new_xform[0], trans, vert_xform[0]
    mul_vec_mat_44      new_xform[1], trans, vert_xform[1]
    mul_vec_mat_44      new_xform[2], trans, vert_xform[2]
    mul_vec_mat_44      new_xform[3], trans, vert_xform[3]

    --cont

    ; -------------------- transform & texture loop ---------------------------

main_loop_lid:

        init_io_loop
        init_out_buf

        set_strip_adcs

xform_loop_lid:
        --LoopCS 1,3

        load_vert           vert
        xform_vert          xformed_vert, new_xform, vert
        ftoi4.xyz           gs_vert, xformed_vert

        load_strip_adc      strip_adc
        set_adc_s           gs_vert, strip_adc
        store_xyzf          gs_vert

        load_pvcolor        vtx_color
        loi                 255.0
        muli.xyz            vtx_color, vtx_color, i
        addi.w              vtx_color, vf00, i
        max                 vtx_color, vtx_color, vf00
        ftoi0               vtx_color, vtx_color
        store_rgba          vtx_color

        load_stq            tex_stq
        xform_tex_stq       tex_stq, tex_stq, Q
        store_stq           tex_stq

        next_io
        loop_io             xform_loop_lid

    ; -------------------- done! -------------------------------

done_lid:
        kick_to_gs
        --cont
        b    main_loop_lid

.END
