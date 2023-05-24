/*     Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

            This file is subject to the terms and conditions of the GNU Lesser
       General Public License Version 2.1. See the file "COPYING" in the
       main directory of this archive for more details.                             */


     ;
     ;
     ; looping
     ;
     ;

     ; ---------------------------------------------------
     ; prepare to loop over the directional lights (parallel)
     ;
     ; outputs:     num_dir_lights
     ;              light_ptr      - ptr to the next light
     ;              light_ptr_ptr  - ptr to the next light ptr

     .macro         init_dlt_loop
     ; get number of dir lights
     ilw.x          num_dir_lights, kNumLights(vi00)
     iaddiu         light_ptr_ptr, vi00, kLightPointers0
     ilw.x          light_ptr, 0(light_ptr_ptr)
     .endm

     ; ---------------------------------------------------
     ; loop over the directional lights
     ;
     ; modifies:    num_dir_lights

     .macro         loop_dir_lts   loop_target
     isubiu         num_dir_lights, num_dir_lights, 1
     ibne           num_dir_lights, vi00, \loop_target
     .endm

     ; ---------------------------------------------------
     ; prepare to loop over the point lights
     ;
     ; outputs:     num_pt_lights
     ;              light_ptr      - ptr to the next light
     ;              light_ptr_ptr  - ptr to the next light ptr

     .macro         init_plt_loop
     ; get number of point lights
     ilw.y          num_pt_lights, kNumLights(vi00)
     iaddiu         light_ptr_ptr, vi00, kLightPointers0
     ilw.y          light_ptr, 0(light_ptr_ptr)
     .endm

     ; ---------------------------------------------------
     ; loop over the point lights
     ;
     ; modifies:    num_pt_lights

     .macro         loop_pt_lts    loop_target
     isubiu         num_pt_lights, num_pt_lights, 1
     ibne           num_pt_lights, vi00, \loop_target
     .endm

     ; ---------------------------------------------------
     ; increment the next light pointer
     ;
     ; modifies:    light_ptr, light_ptr_ptr

     .macro         next_dir_light
     iaddiu         light_ptr_ptr, light_ptr_ptr, 1
     ilw.x          light_ptr, 0(light_ptr_ptr)
     .endm

     .macro         next_pt_light
     iaddiu         light_ptr_ptr, light_ptr_ptr, 1
     ilw.y          light_ptr, 0(light_ptr_ptr)
     .endm

     ;
     ;
     ; loading
     ;
     ;

     ; ---------------------------------------------------
     ; load macros - pretty self-explanatory
     ;
     ; for colors, the value "max" is either 128.0 or 255.0 because
     ; of the way textures are handled (when texturing is enabled
     ; and modulated by the vertex color, 128.0 is unity and 255.0 is
     ; around 2.0).

     ; vertex-to-viewpoint vector
     .macro         load_vert_eye  vec
     lq.xyz         \vec, kFixedVertToEye(vi00)
     .endm

     ; global ambient color (rgb, 0.0-1.0)
     .macro         load_glob_amb  amb
     lq.xyz         \amb, kGlobalAmbient(vi00)
     .endm

     ; per-vertex color
     .macro         load_pvcolor   color
     lq.xyz         \color, 3(next_input)
     .endm

     ; material colors

     ; material ambient (rgb, 0.0-1.0)
     .macro         load_mat_amb   amb
     lq.xyz         \amb, kMaterialAmbient(vi00)
     .endm
     ; material diffuse (rgb, 0.0-1.0)
     .macro         load_mat_diff  diff, mask=xyz
     lq.\mask       \diff, kMaterialDiffuse(vi00)
     .endm
     ; material specular (rgb, 0.0-1.0)
     .macro         load_mat_spec  spec
     lq.xyz         \spec, kMaterialSpecular(vi00)
     .endm
     ; material emmisive (rgb, 0.0-max)
     .macro         load_mat_emm   emm, mask=xyz
     lq.\mask       \emm, kMaterialEmission(vi00)
     .endm

     ; light attributes

     ; light ambient (rgb, 0.0-max)
     .macro         load_lt_amb    amb
     lq.xyz         \amb, kLightAmbientOffset(light_ptr)
     .endm
     ; light diffuse (rgb, 0.0-max)
     .macro         load_lt_diff   diff
     lq.xyz         \diff, kLightDiffuseOffset(light_ptr)
     .endm
     ; light specular (rgb, 0.0-max)
     .macro         load_lt_spec   spec
     lq.xyz         \spec, kLightSpecularOffset(light_ptr)
     .endm
     ; light position (xyz)
     .macro         load_lt_pos    pos
     lq.xyz         \pos, kLightPosOffset(light_ptr)
     .endm
     ; light attenuation coefficients (constant, linear, quadratic)
     .macro         load_lt_atten  coeff
     lq.xyz         \coeff, kLightAttenCoeffOffset(light_ptr)
     .endm

     ; load transpose of object to world transform
     .macro         load_o2wt      xform
     lq.xyz         \xform[0], kObjToWorldXfrmTrans+0(vi00)
     lq.xyz         \xform[1], kObjToWorldXfrmTrans+1(vi00)
     lq.xyz         \xform[2], kObjToWorldXfrmTrans+2(vi00)
     .endm

     ; load the world to object space transform
     .macro         load_w2o       xform
     lq.xyz        \xform[0], kWorldToObjXfrm+0(vi00)
     lq.xyz        \xform[1], kWorldToObjXfrm+1(vi00)
     lq.xyz        \xform[2], kWorldToObjXfrm+2(vi00)
     lq.xyz        \xform[3], kWorldToObjXfrm+3(vi00)
     .endm
