/*     Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

            This file is subject to the terms and conditions of the GNU Lesser
       General Public License Version 2.1. See the file "COPYING" in the
       main directory of this archive for more details.                             */


     .macro         mul_vec_mat_44 output, xform, input
     mulax          acc, \xform[0], \input
     madday         acc, \xform[1], \input
     maddaz         acc, \xform[2], \input
     maddw          \output, \xform[3], \input
     .endm

     .macro         mul_vec_mat_43 output, xform, input
     mulax          acc, \xform[0], \input
     madday         acc, \xform[1], \input
     maddz          \output, \xform[2], \input
     .endm

     .macro         mul_vec_mat_33 output, xform, input
     mulax.xyz      acc, \xform[0], \input
     madday.xyz     acc, \xform[1], \input
     maddz.xyz      \output, \xform[2], \input
     .endm

     .macro         mul_pt_mat_44  output, xform, input
     mulax          acc, \xform[0], \input
     madday         acc, \xform[1], \input
     maddaz         acc, \xform[2], \input
     maddw          \output, \xform[3], vf00
     .endm

     .macro         mul_pt_mat_34  output, xform, input
     mulax.xyz      acc, \xform[0], \input
     madday.xyz     acc, \xform[1], \input
     maddaz.xyz     acc, \xform[2], \input
     maddw.xyz      \output, \xform[3], vf00
     .endm

     .macro         normalize_3    output, input, scalar
     div            q, vf00w, \scalar
     mulq.xyz       \output, \input, q
     .endm

     .macro         get_ones_vec   ones
     maxw           ones, vf00, vf00
     .endm

     .macro         dot3_to_x      output, vec1, vec2, x_one
     mul.xyz        \output, \vec1, \vec2
     adday.x        acc, \output, \output
     maddz.x        \output, \x_one, \output
     .endm
     .macro         dot3_to_z      output, vec1, vec2, z_one
     mul.xyz        \output, \vec1, \vec2
     adday.z        acc, \output, \output
     maddx.z        \output, \z_one, \output
     .endm
     .macro         dot3_to_w      output, vec1, vec2
     mul.xyz        temp\@, \vec1, \vec2
     mulax.w        acc, vf00, temp\@
     madday.w       acc, vf00, temp\@
     maddz.w        \output, vf00, temp\@
     .endm
     ; this is slower than above, but uses a lower inst in place of
     ; one upper
     .macro         dot3_to_w_slow output, vec1, vec2
     mul.xyz        temp\@, \vec1, \vec2
     mr32.xyw       temp\@, temp\@
     addax.w        acc, temp\@, temp\@
     maddy.w        \output, vf00, temp\@
     .endm

     .macro         transpose_33ip mat
     addy.x         temp[1]\@, vf00, \mat[0]
     addx.y         \mat[0], vf00, \mat[1]
     add.x          \mat[1], vf00, temp[1]\@

     addz.x         temp[2]\@, vf00, \mat[0]
     addx.z         \mat[0], vf00, \mat[2]
     add.x          \mat[2], vf00, temp[2]\@

     mr32.x         temp1\@, \mat[2]
     mr32.w         temp1\@, temp1\@
     mr32.y         \mat[2], \mat[1]
     mr32.z         \mat[1], temp1\@
     .endm
