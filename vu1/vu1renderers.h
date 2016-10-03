/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America
       	  
       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_vu1code_h
#define ps2gl_vu1code_h

extern "C" {
   void vsmStart();

   void vsmGeneral();
   void vsmGeneralQuad();
   void vsmGeneralTri();

   void vsmGeneralNoSpec();
   void vsmGeneralNoSpecTri();
   void vsmGeneralNoSpecQuad();

   void vsmGeneralPVDiff();
   void vsmGeneralPVDiffTri();
   void vsmGeneralPVDiffQuad();

   void vsmSCEI();
   void vsmFast();
   void vsmFastNoLights();

   void vsmEnd();
}

#endif // ps2gl_vu1code_h
