/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

	#include		"billboard_renderer_mem.h"

	; from ps2gl/vu1
	.include		"db_in_db_out.i"
	.include		"math.i"
	.include		"lighting.i"
	.include		"clip_cull.i"
	.include		"geometry.i"
	.include		"io.i"

kInputQPerV		.equ			1
kOutputQPerV		.equ			2

	.init_vf_all
	.init_vi_all

	.name		vsmBillboards

	--enter
	--endenter

	; ------------------------ initialization ---------------------------------

	load_vert_xfrm	vert_xform

	; we'll use this to create the billboard
	load_o2wt		obj_to_world_transpose

	init_constants
	init_clip_cnst

	--cont

	; -------------------- transform & texture loop ---------------------------

main_loop_lid:	

	; loop over the inputs
	init_i_loop

	; set up output pointer (buffer_top is output by init_i_loop)
	iaddiu		next_output, buffer_top, kOutputStart

	; set nloop and copy the giftag to the output
	lq			giftag, kGifTag(vi00)
	mtir			eop, giftag[x]
	ior			eop, eop, num_verts
	mfir.x		giftag, eop
	sqi			giftag, (next_output++)

	; make a (1, 1) for the lower-right tex coords
	maxw.xyz			lr_tex_coord, vf00, vf00
	; make a (0, 0) for the upper-left
	move.xy		ul_tex_coord, vf00
	maxw.z		ul_tex_coord, vf00, vf00

	minix.w		ul_tex_coord, vf00, vf00
	minix.w		lr_tex_coord, vf00, vf00

xform_loop_lid:
	--LoopCS		1,1

	; next_input is maintained by i_loop
	lq			center, 0(next_input)

	; make the upper-left and lower-right vertices for the sprite
	; obj_to_world_transpose[0] points right and otwt[1] points up

	; upper_left = center + (up - right) * size
	sub.xyz		up_left, obj_to_world_transpose[1], obj_to_world_transpose[0]
	mulw.xyz		up_left, up_left, center
	add.xyz		up_left, up_left, center

	; down_right = center + (right - up) * size
	sub.xyz		down_right, obj_to_world_transpose[0], obj_to_world_transpose[1]
	mulw.xyz		down_right, down_right, center
	add.xyz		down_right, down_right, center

	; xform/clip vertex

	; first the upper-left
	xform_vert	xformed_vert, vert_xform, up_left
	vert_to_gs	ul_gs_vert, xformed_vert

	clip_vert		xformed_vert

	xform_tex_stq	ul_tex_asdf, ul_tex_coord, q

	; now the lower-right
	xform_vert	xformed_vert, vert_xform, down_right
	vert_to_gs	lr_gs_vert, xformed_vert

	clip_vert		xformed_vert

	xform_tex_stq	lr_tex_asdf, lr_tex_coord, q

	; test against this vert and the last
	fcand		vi01, 0x0000fff
	iaddiu		adc, vi01, 0x7fff
	mfir.w		lr_gs_vert, adc
	mfir.w		ul_gs_vert, adc

	sqi			ul_tex_coord, (next_output++)
	sqi			ul_gs_vert, (next_output++)
	sqi			lr_tex_coord, (next_output++)
	sqi			lr_gs_vert, (next_output++)

	next_i
	loop_io		xform_loop_lid

	; -------------------- done! -------------------------------

done_lid:	

	; ---------------- kick packet to GS -----------------------

	kick_to_gs

	--cont
	
	b	main_loop_lid

.END	; for gasp
