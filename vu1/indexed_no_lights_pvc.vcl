    #include       "vu1_mem_indexed.h"

    .include       "db_in_sb_out.i"
    .include       "math.i"
    .include       "lighting.i"
    .include       "clip_cull.i"
    .include       "geometry.i"
    .include       "io.i"
    .include       "general.i"

kInputQPerV    .equ   4
kOutputQPerV   .equ   3

    .init_vf_all
    .init_vi_all

    .name          vsmIndexedPVC

    --enter
    --endenter

    load_vert_xfrm vert_xform

    --cont

main_loop_lid:
    init_constants
    init_clip_cnst

    xtop           buffer_top
    iaddiu         next_output,  vi00,       kOutputGeomStart
    iaddiu         input_start,  buffer_top, kInputGeomStart

    iaddiu         next_index,       buffer_top, kInputGeomStart
    iaddiu         first_index_mask, vi00,       0xff
    loi            253.0
    maxi.w         index_constants,  vf00, i
    loi            4.0
    maxi.z         index_constants,  vf00, i
    loi            255.0
    maxi.y         index_constants,  vf00, i

    ilw.y          num_indices_d2, kNumIndicesD2(buffer_top)
    iadd           last_index,     next_index,   num_indices_d2
    ilw.z          num_indices,    kNumIndices(buffer_top)

    lq             gif_tag,   kGifTag(vi00)
    mtir           eop,       gif_tagx
    ior            eop,       eop, num_indices
    mfir.x         gif_tag,   eop
    mfir.w         gif_tag,   next_output
    sq             gif_tag,   kOutputBufStart(vi00)

    iaddiu         zero_giftag, vi00, kGifTag
    xgkick         zero_giftag

xform_loop_lid:      --LoopCS 1,3
    ilw.w          first_index, 0(next_index)
    iand           first_index, first_index, first_index_mask
    iadd           first_offset, first_index, first_index
    iadd           first_offset, first_offset, first_offset

    lqi.w          indices, (next_index++)
    addy.w         second_ind, indices, index_constants[y]
    mtir           second_index,  second_ind[w]
    mulz.w         second_off,    indices, index_constants[z]
    add.w          second_off,    second_off, index_constants[w]
    mtir           second_offset, second_off[w]

    .macro do_vert
        load_vert      vert
        xform_vert     xformed_vert, vert_xform, vert
        vert_to_gs     gs_vert, xformed_vert
        clip_vert      xformed_vert
        fcand          vi01, 0x003ffff
        iand           vi01, vi01, do_clipping
        set_adc_fs     gs_vert, vi00
        store_xyzf     gs_vert

        iaddiu         color_qw, next_input, kColorQwOff ;TODO: this probably as a macro, but just emphatic for now...
        lq             vert_color, 0(color_qw)
        loi            255.0
        muli           vert_color, vert_color, i
        max            vert_color, vert_color, vf00
        ftoi0          vert_color, vert_color
        store_rgba     vert_color

        load_stq       tex_stq
        xform_tex_stq  tex_stq, tex_stq, q
        store_stq      tex_stq
    .endm

    iadd           next_input, first_offset,  input_start
    do_vert

    iadd           next_input, second_offset, input_start
    iaddiu         next_output, next_output,  kOutputQPerV
    do_vert

    next_o
    ibne           next_index, last_index, xform_loop_lid
    kick_to_gs

--cont
    b              main_loop_lid
.END
