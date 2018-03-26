/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_vu1code_h
#define ps2gl_vu1code_h

#define VU_FUNCTIONS(name)        \
    void vsm##name##_CodeStart(); \
    void vsm##name##_CodeEnd()

#define mVsmAddr(name) ((void*)vsm##name##_CodeStart)
#define mVsmSize(name) ((u8*)vsm##name##_CodeEnd - (u8*)vsm##name##_CodeStart)

extern "C" {
VU_FUNCTIONS(General);
VU_FUNCTIONS(GeneralTri);
VU_FUNCTIONS(GeneralQuad);

VU_FUNCTIONS(GeneralNoSpec);
VU_FUNCTIONS(GeneralNoSpecTri);
VU_FUNCTIONS(GeneralNoSpecQuad);

VU_FUNCTIONS(GeneralPVDiff);
VU_FUNCTIONS(GeneralPVDiffTri);
VU_FUNCTIONS(GeneralPVDiffQuad);

VU_FUNCTIONS(SCEI);
VU_FUNCTIONS(Fast);
VU_FUNCTIONS(FastNoLights);

VU_FUNCTIONS(Indexed);
}

#endif // ps2gl_vu1code_h
