/*	  Copyright (C) 2000,2001,2002  Sony Computer Entertainment America

       	  This file is subject to the terms and conditions of the GNU Lesser
	  General Public License Version 2.1. See the file "COPYING" in the
	  main directory of this archive for more details.                             */

#ifndef ps2gl_dlist_h
#define ps2gl_dlist_h

#include <string.h>

#include "GL/gl.h"
#include "ps2gl/debug.h"
#include "ps2s/packet.h"

/********************************************
 * display list commands
 */

class CDListCmd {
public:
    CDListCmd() {}
    virtual ~CDListCmd() {}

    virtual CDListCmd* Play() = 0;

    template <class CmdType>
    static inline int SizeOf()
    {
        // SEE FUNCTION below if you change anything here..
        int size = sizeof(CmdType);
        // round up to nearest quad.. this is temporary so that I
        // can use the vector classes for a while
        if (size & 0xf)
            size = (size & ~0xf) + 16;
        return size;
    }

    template <class CmdType>
    static CDListCmd* GetNextCmd(CmdType* cmd)
    {
        // for some reason gcc doesn't find the above method when called
        // from here... go figure.
        int size = sizeof(CmdType);
        // round up to nearest quad.. this is temporary so that I
        // can use the vector classes for a while
        if (size & 0xf)
            size = (size & ~0xf) + 16;

        return reinterpret_cast<CDListCmd*>((unsigned int)cmd + size);
    }
};

class CEmptyListCmd : public CDListCmd {
public:
    CDListCmd* Play()
    {
        mError("Trying to play an empty list!");
        return NULL;
    }

    // round (size of this class) up to nearest quad.. this is temporary so that I
    // can use the vector classes for a while
    int filler[3];
};

class CEndListCmd : public CDListCmd {
public:
    CDListCmd* Play() { return NULL; }
};

class CDListCmdBlock;
class CNextBlockCmd : public CDListCmd {
    CDListCmdBlock* NextBlock;

public:
    CNextBlockCmd(CDListCmdBlock* nextBlock)
        : NextBlock(nextBlock)
    {
    }
    CDListCmd* Play(); // defined after CDListCmdBlock
};

class CEnableCmd : public CDListCmd {
    GLenum Property;

public:
    CEnableCmd(GLenum prop)
        : Property(prop)
    {
    }
    CDListCmd* Play()
    {
        glEnable(Property);
        return CDListCmd::GetNextCmd(this);
    }
};

/********************************************
 * CDListCmdBlock
 */

class CDListCmdBlock {
    static const int ByteSize = 2048;
    char Memory[ByteSize];
    char* MemCursor;
    int BytesLeft;
    CDListCmdBlock* NextBlock;

public:
    CDListCmdBlock()
        : MemCursor(Memory)
        , BytesLeft(ByteSize)
        , NextBlock(NULL)
    {
        CEmptyListCmd empty;
        memcpy(MemCursor, &empty, CDListCmd::SizeOf<CEmptyListCmd>());
    }
    ~CDListCmdBlock()
    {
        if (NextBlock)
            delete NextBlock;
    }

    template <class CmdType>
    bool CanFit(CmdType cmd)
    {
        return (CDListCmd::SizeOf<CmdType>()
            <= BytesLeft - CDListCmd::SizeOf<CNextBlockCmd>());
    }

    template <class CmdType>
    void operator+=(CmdType cmd)
    {
        memcpy(MemCursor, &cmd, sizeof(CmdType)); // this should be the usual sizeof()
        MemCursor += CDListCmd::SizeOf<CmdType>();
        BytesLeft -= CDListCmd::SizeOf<CmdType>();
    }

    CDListCmd* GetFirstCmd() { return reinterpret_cast<CDListCmd*>(&Memory[0]); }

    void SetNextBlock(CDListCmdBlock* next) { NextBlock = next; }
    CDListCmdBlock* GetNextBlock() const { return NextBlock; }
};

inline CDListCmd*
CNextBlockCmd::Play()
{
    return NextBlock->GetFirstCmd();
}

/********************************************
 * CDList
 */

class CDList {
    static const int kBufferMaxQwordLength = 16 * 1024; // 256kb
    CDmaPacket *VertexBuf, *NormalBuf, *TexCoordBuf, *ColorBuf;
    CDListCmdBlock *FirstCmdBlock, *CurCmdBlock;
    static const int kMaxNumRenderPackets = 512;
    int NumRenderPackets;
    CVifSCDmaPacket* RenderPackets[kMaxNumRenderPackets];

public:
    CDList();
    ~CDList();

    // utility
    template <class CmdType>
    CDListCmd* GetNext(CmdType* cmd)
    {
        return 0;
    }

    template <class CmdType>
    void operator+=(CmdType cmd)
    {
        if (!CurCmdBlock->CanFit(cmd)) {
            // not enough space left in current cmd block, so terminate it
            // and start a new one..
            CDListCmdBlock* newBlock = new CDListCmdBlock;
            CurCmdBlock->SetNextBlock(newBlock);
            *CurCmdBlock += CNextBlockCmd(newBlock);
            CurCmdBlock = newBlock;
        }
        *CurCmdBlock += cmd;
    }

    void Play()
    {
        CDListCmd* nextCmd = FirstCmdBlock->GetFirstCmd();
        while (nextCmd)
            nextCmd = nextCmd->Play();
    }

    void Begin();
    void End()
    {
        CEndListCmd endList;
        *this += endList;
    }

    CDmaPacket& GetVertexBuf();
    CDmaPacket& GetNormalBuf();
    CDmaPacket& GetTexCoordBuf();
    CDmaPacket& GetColorBuf();

    void RegisterNewPacket(CVifSCDmaPacket* packet)
    {
        RenderPackets[NumRenderPackets++] = packet;
    }
};

/********************************************
 * CDListManager
 */

class CDListManager {
    static const int kMaxListID = 4096;
    int NextFreeListID;
    CDList* Lists[kMaxListID];
    unsigned int OpenListID;
    CDList* OpenList;

    bool ListsAreFree(int firstListID, int numLists);

    static const int kMaxBuffersToBeFreed = 1024;
    CDList* ListsToBeFreed[2][kMaxBuffersToBeFreed];
    int NumListsToBeFreed[2];
    int CurBuffer;

    inline void AddListToBeFreed(CDList* dlist)
    {
        mAssert(NumListsToBeFreed[CurBuffer] < kMaxBuffersToBeFreed);
        ListsToBeFreed[CurBuffer][NumListsToBeFreed[CurBuffer]++] = dlist;
    }

public:
    CDListManager()
        : NextFreeListID(1)
        , OpenListID(0)
        , OpenList(NULL)
        , CurBuffer(0)
    {
        for (int i           = 0; i < kMaxListID; i++)
            Lists[i]         = NULL;
        NumListsToBeFreed[0] = NumListsToBeFreed[1] = 0;
    }
    ~CDListManager()
    {
        for (int i = 0; i < 10; i++)
            if (Lists[i])
                delete Lists[i];
    }

    void SwapBuffers();

    unsigned int GenLists(int numLists);
    void DeleteLists(unsigned int firstListID, int numLists);
    void NewList(unsigned int listID, GLenum mode);
    void EndList();
    void CallList(unsigned int listID);

    CDList& GetOpenDList() const { return *OpenList; }
};

#endif // ps2gl_dlist_h
