/*     Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

            This file is subject to the terms and conditions of the GNU Lesser
       General Public License Version 2.1. See the file "COPYING" in the
       main directory of this archive for more details.                             */


     ; ---------------------------------------------------
     ; initialize the output buffer (where the packet is written).
     ; Needs to be called immediately after init_io or init_o

     .macro         init_out_buf
     ; fill in the nloop field in the giftag and store at
     ; top of the output buffer
     lq             gif_tag\@, kGifTag(vi00)
     mtir           eop\@, gif_tag\@x
     ior            eop\@, eop\@, num_verts
     mfir.x         gif_tag\@, eop\@
     sq             gif_tag\@, -1(next_output)
     .endm

     ; ---------------------------------------------------
     ; internal - used by init_io_loop and init_o_loop
     ; calculates the offset of the last vertex in a buffer
     ;
     ; inputs:      num_verts
     ; params:      last           - the return value is written here
     ;              first          - pointer to first vert in buffer
     ;              num_quads      - number of quads per vertex

     .macro         get_last_vert  last, first, num_quads
     iadd           \last, \first, num_verts
     .arepeat       \num_quads-1
     iadd           \last, \last, num_verts
     .aendr
     .endm

     ; ---------------------------------------------------
     ; outputs:     num_verts      - the total number of vertices

     .macro         get_num_verts
     ilw.x          num_verts, kNumVertices(buffer_top)
     .endm

     ; ---------------------------------------------------
     ; get ready to loop over inputs and write to output buffer.
     ;
     ; params:      input_addr     - a *constant* address in vu mem where
     ;                               input geom begins.  default is kInputStart
     ;              output_addr    - a *constant* address in vu mem where
     ;                               output geom begins.  default is kOutputStart+1
     ; outputs:     buffer_top     - the starting offset of this buffer
     ;              num_verts      - the total number of vertices
     ;              next_input
     ;              next_output
     ;              last_input     - a pointer to the last vertex

     .macro         init_io_loop   input_addr=kInputStart, output_addr=kOutputStart+1
     ; input/output ptrs and strides (increments to next vertex data)
     xtop           buffer_top
     iaddiu         next_input, buffer_top, \input_addr
     .aif           "\&_out_buffer" eq "_double"
     iaddiu         next_output, buffer_top, \output_addr
     .aelse
     .aif           "\&_out_buffer" eq "_single"
     iaddiu         next_output, vi00, \output_addr
     .aelse
     "error: you need to include ?_in_?_out.i"
     .aendi
     .aendi

     get_num_verts

     ; when to stop
     get_last_vert  last_input, next_input, kInputQPerV
     .endm

     ; ---------------------------------------------------
     ; get ready to loop over inputs only.
     ;
     ; inputs:
     ; outputs:     (same as above) buffer_top, next_input, last_input

     .macro         init_i_loop    input_addr=kInputStart
     ; input/output ptrs and strides (increments to next vertex data)
     xtop           buffer_top
     iaddiu         next_input, buffer_top, \input_addr

     get_num_verts

     ; when to stop
     get_last_vert  last_input, next_input, kInputQPerV
     .endm

     ; ---------------------------------------------------
     ; get ready to loop over outputs only.
     ;
     ; inputs:      num_verts
     ; outputs:     (same as above) buffer_top, next_output, last_output

     .macro         init_o_loop
     .aif           "\&_out_buffer" eq "_double"
     xtop           buffer_top
     iaddiu         next_output, buffer_top, kOutputStart+1
     .aelse
     .aif           "\&_out_buffer" eq "_single"
     iaddiu         next_output, vi00, kOutputStart+1
     .aelse
     "error: you need to include ?_in_?_out.i"
     .aendi
     .aendi

     ; when to stop
     get_last_vert  last_output, next_output, kOutputQPerV
     .endm

     ; ---------------------------------------------------
     ; increments the current input pointer
     ;
     ; params:      num            - the number of vertices to increment (**not qwords**)
     ;                             default is 1.

     .macro         next_i         num=1
     .arepeat       \num
     iaddiu         next_input, next_input, kInputQPerV
     .aendr
     .endm

     ; ---------------------------------------------------
     ; increments the current output pointer
     ;
     ; params:      num            - the number of vertices to increment (**not qwords**)
     ;                             default is 1.

     .macro         next_o         num=1
     .arepeat       \num
     iaddiu         next_output, next_output, kOutputQPerV
     .aendr
     .endm

     ; ---------------------------------------------------
     ; increments both input and output pointers
     ;
     ; params:      num            - the number of vertices to increment (**not qwords**)
     ;                             default is 1.

     .macro         next_io        num=1
     next_i         \num
     next_o         \num
     .endm

     ; ---------------------------------------------------
     ; do next iteration of loop
     ;
     ; inputs:      next_output, last_output
     ; params:      loop_target    - the branch target

     .macro         loop_io        loop_target
     ibne           next_input, last_input, \loop_target
     .endm

     .macro         loop_o         loop_target
     ibne           next_output, last_output, \loop_target
     .endm

     ; ---------------------------------------------------
     ; kicks the output packet to the gs
     ;
     ; inputs:      buffer_top, kOutputStart

     .macro         kick_to_gs
     .aif           "\&_out_buffer" eq "_double"
     iaddiu         packet_start\@, buffer_top, kOutputStart
     .aelse
     .aif           "\&_out_buffer" eq "_single"
     iaddiu         packet_start\@, vi00, kOutputStart
     .aendi
     .aendi
     xgkick         packet_start\@
     .endm
