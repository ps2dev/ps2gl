/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#include "GL/ps2gl.h"

#include "ps2s/gsmem.h"

/**
 * @file gsmemory.cpp
 * pgl interface to gs memory manager
 */

static bool GsMemInitted = false;

/**
 * @addtogroup pgl_api
 * @{
 */

// gs mem

/**
 * @defgroup gs_mem gs memory management
 * Functions to initialize, allocate, and free gs memory.
 * ps2gl manages gs memory through the use of memory <i>slots</i> and
 * memory <i>areas</i>.  The basic idea is that the app partitions gs
 * memory into slots (probably once at the beginning of a level) which are
 * then used to allocate frame buffers, depth buffers, textures, etc.
 *
 * See the memory slot and memory area "modules" for more details.
 * @{
 */

/**
 * prints the current gs memory allocation to stdout
 */
void pglPrintGsMemAllocation()
{
    GS::CMemManager& mm = GS::CMemArea::GetMemManager();
    mm.PrintAllocation();
}

/**
 * returns whether gs memory has been initialized (by adding one or more slots).
 * @return 1 if true, 0 if false
 */
int pglHasGsMemBeenInitted()
{
    return GsMemInitted;
}

// slots

/**
 * @defgroup gs_mem_slots GS memory 'slots'
 *
 * API for managing the "slots" that partition GS memory in ps2gl.
 * As mentioned in the documentation for "GS memory management," ps2gl
 * asks the user to partition GS memory into slots.  A slot is defined
 * by a starting page number in GS ram, a page length, and a preferred
 * pixel format.
 *
 * Note that the pixel format of a slot is its <i>preferred</i> format.
 * If there are no slots of the requested pixel type large enough, ps2gl
 * will try to find a big enough slot of a "higher" pixel type.  For example,
 * a 64x64, 32-bit slot (2 pages) might be allocated to hold a 128x128, 8-bit
 * image if there were no 8-bit slots large enough.
 *
 * The idea is that if a level has a lot of 64x64, 8-bit textures, you would
 * allocate a lot of 64x64, 8-bit slots.
 *
 * For example, say that our app uses a 640x448 interlaced display with
 * a 24-bit depth buffer.  We would create 3 slots, one for the depth
 * buffer and two for a double-buffered frame buffer.
 *
 * First, we need the
 * size and starting address of each slot.  The size is calculated from
 * the dimensions of a page for the pixel format in question.  Here are
 * the dimensions of one GS "page" for the various pixel formats (from
 * section 8.3 of the GS manual):
 *
 *   - 64x32:   PSMCT32/24, PSMZ32/24
 *   - 64x64:   PSMCT16/16S
 *   - 128x64:  PSMT8
 *   - 128x128: PSMT4
 *
 * Each frame buffer is 640x224 (interlaced) at 32-bits.  This means we need
 * a slot 10 pages by 7 pages = 70 pages.  The 24-bit depth buffer pages
 * are the same size, so we need another 70 pages for the depth buffer.
 *
 * Now that we have the sizes of each slot (70 pages), we just need the
 * starting addresses.  It doesn't really matter much where they begin,
 * except of course when existing code assumes to find them in certain places.
 * So let's make them contiguous starting at page 0:
 * <p>
 *    pglAddGsMemSlot( 0, 70, SCE_GS_PSMCT32 );<br>
 *    pglAddGsMemSlot( 70, 70, SCE_GS_PSMCT32 );<br>
 *    pglAddGsMemSlot( 140, 70, SCE_GS_PSMZ32 );<br>
 * <p>
 *
 * Take a look at ps2glut.cpp for a more complete example (a better tutorial
 * is on the way).
 *
 * @{
 */

/**
 * Adds a memory slot to the list of free slots.
 * @param startingPage the first page this slot occupies
 * @param pageLength the number of pages this slot occupies (NOT the last page)
 * @param pixelMode the preferred pixel format of this slot
 * @return a handle to the new slot
 */
pgl_slot_handle_t
pglAddGsMemSlot(int startingPage, int pageLength, unsigned int pixelMode)
{
    GS::CMemManager& mm   = GS::CMemArea::GetMemManager();
    GS::CMemSlot* newSlot = mm.AddSlot(startingPage, pageLength, (GS::tPSM)pixelMode);

    GsMemInitted = true;

    return reinterpret_cast<pgl_slot_handle_t>(newSlot);
}

/**
 * Prevents a slot from being allocated or freed automatically.
 * The slot can still be bound to an area by the user.
 */
void pglLockGsMemSlot(pgl_slot_handle_t slot_handle)
{
    GS::CMemSlot* slot = reinterpret_cast<GS::CMemSlot*>(slot_handle);
    slot->Lock();
}

/**
 * Lets the memory manager automatically allocate/free a slot that
 * was previously locked.
 */
void pglUnlockGsMemSlot(pgl_slot_handle_t slot_handle)
{
    GS::CMemSlot* slot = reinterpret_cast<GS::CMemSlot*>(slot_handle);
    slot->Unlock();
}

/**
 * Removes all gs memory slots.
 */
void pglRemoveAllGsMemSlots()
{
    GS::CMemManager& mm = GS::CMemArea::GetMemManager();
    mm.RemoveAllSlots();
}

/** @} */ // gs_mem_slots

// mem area operations

/**
 * @defgroup gs_mem_areas GS memory 'areas'
 *
 * API for working with GS memory "areas" that are used to allocate
 * GS ram.
 * An area is defined by a width and height (in pixels) and a pixel
 * format.  The area can then be given an address in GS ram by the
 * application or be bound to a memory slot (either automatically or by
 * the user) which will set the address in GS ram.
 *
 * An allocated memory area (one that has an address) can be used as
 * a texture, drawn to, or displayed.
 *
 * @{
 */

/**
 * Create a memory area.
 * @param width width in pixels
 * @param height height in pixels
 * @param pix_format pixel format (SCE_GS_PS*)
 * @return a handle to the newly created area
 */
pgl_area_handle_t
pglCreateGsMemArea(int width, int height, unsigned int pix_format)
{
    GS::CMemArea* newArea = new GS::CMemArea(width, height, (GS::tPSM)pix_format);
    return reinterpret_cast<pgl_area_handle_t>(newArea);
}

/**
 * Destroy a memory area (free the memory it occupies).
 */
void pglDestroyGsMemArea(pgl_area_handle_t mem_area)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    delete area;
}

/**
 * Allocate GS ram by binding to a slot.  This operation will
 * first find a list of slots which are closest in dimension and
 * pixel format to the memory area in question, then bind either
 * to a free slot or, if none are available, to the least-recently-used
 * slot after first freeing it.
 *
 * Note that this will always succeed.  If no suitable, unlocked slots
 * exist the memory manager will panic and fail an assertion.
 */
void pglAllocGsMemArea(pgl_area_handle_t mem_area)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    area->Alloc();
}

/**
 * Free a memory area.  Note that this does <b>not</b> free any
 * main ram; the user must still call pglDestroyGsMemArea();
 */
void pglFreeGsMemArea(pgl_area_handle_t mem_area)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    area->Free();
}

/**
 * Manually set the starting GS ram word address of this area (mainly for
 * compatibility with existing code).
 * @param addr the word address in GS ram (byte addr / 4)
 */
void pglSetGsMemAreaWordAddr(pgl_area_handle_t mem_area, unsigned int addr)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    area->SetWordAddr(addr);
}

/**
 * This is the manual equivalent of pglAllocGsMemArea() (except that the slot
 * does not have to be unlocked).
 */
void pglBindGsMemAreaToSlot(pgl_area_handle_t mem_area, pgl_slot_handle_t mem_slot)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    GS::CMemSlot* slot = reinterpret_cast<GS::CMemSlot*>(mem_slot);
    slot->Bind(*area, 0); // FIXME: current frame number
}

/**
 * Release the slot bound to this area.
 */
void pglUnbindGsMemArea(pgl_area_handle_t mem_area)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    area->Unbind();
}

/**
 * Prevent this area from being allocated or freed automatically by the memory
 * manager (it may still be operated on manually).
 */
void pglLockGsMemArea(pgl_area_handle_t mem_area)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    area->Lock();
}
/**
 * Let the memory manager affect the allocation of this area.
 */
void pglUnlockGsMemArea(pgl_area_handle_t mem_area)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    area->Unlock();
}
/**
 * @return 1 if allocated, 0 if not.
 */
int pglGsMemAreaIsAllocated(pgl_area_handle_t mem_area)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    return (int)area->IsAllocated();
}
/**
 * @return the starting word address in GS ram of this area
 */
unsigned int
pglGetGsMemAreaWordAddr(pgl_area_handle_t mem_area)
{
    GS::CMemArea* area = reinterpret_cast<GS::CMemArea*>(mem_area);
    return area->GetWordAddr();
}

/** @} */ // gs_mem_areas

/** @} */ // gs_mem

/** @} */ // pgl_api
