// SAFEHEAP.cpp — Safe heap allocator with magic-tag integrity checking
//
// Wraps malloc/free with a 4-byte magic-tag header so that SafeHeap_Free
// can assert that the pointer was originally allocated by SafeHeap_Alloc.
// This catches double-free and wild-pointer bugs at runtime.
//
// Allocation layout:
//   [ 4-byte magic tag ][ user data (size bytes) ]
//   SafeHeap_Alloc returns a pointer to the user data (header + 4).
//   SafeHeap_Free receives the user data pointer, walks back 4 bytes,
//   checks the tag, zeroes it, then calls free().
//
// Original source: C:\DevStudio\Projects\Crux\SAFEHEAP.cpp

#include "SAFEHEAP.h"
#include <string.h>

// Magic tag written at the start of every SafeHeap allocation.
// Stored at 0x004d8818 in the binary: likely "CRUX" or similar 4-char string.
extern const char g_szSafeHeapTag[];  // 004d8818

// ============================================================
//  SafeHeap_Alloc — allocate with magic-tag header
//
//  pszFile / nLine — caller's __FILE__ / __LINE__ (for debug output)
//  nSize           — requested byte count
//  Returns pointer to user data (past the 4-byte header), or NULL on OOM.
// ============================================================
void* SafeHeap_Alloc(const char* pszFile, int nLine, int nSize)
{
    extern void* Mem_Alloc(int nBytes);   // FUN_0048ac60 — raw malloc
    extern void  Str_Copy(void* pDst, const void* pSrc);  // FUN_004895e0

    char* pBlock = (char*)Mem_Alloc(nSize + 4);
    if (pBlock == NULL)
        return NULL;

    Str_Copy(pBlock, g_szSafeHeapTag);  // write 4-byte magic at header
    return pBlock + 4;                  // return user pointer
}

// ============================================================
//  SafeHeap_Free — validate tag then free
//
//  Asserts: pointer is non-NULL and tag matches g_szSafeHeapTag.
//  Zeroes the tag before calling free() to catch double-free.
// ============================================================
void SafeHeap_Free(const char* pszFile, int nLine, void* pPtr)
{
    extern void  Debug_Trace(int nLine, const char* pszFile, const char* pszMsg);
    extern void  Debug_Assert(int nLine, const char* pszFile, const char* pszMsg);
    extern void  Fmt_Sprintf(char* pszOut, const char* pszFmt, ...);  // FUN_0048a060
    extern void  Mem_Free(void* pBlock);   // FUN_0048b4a0

    char szMsg[256];

    if (pPtr == NULL)
    {
        Fmt_Sprintf(szMsg, "NULL pointer in %s %d", pszFile, nLine);
        Debug_Trace(0, "C:\\DevStudio\\Projects\\Crux\\SAFEHEAP.cpp", szMsg);
        Debug_Assert(0, "C:\\DevStudio\\Projects\\Crux\\SAFEHEAP.cpp", szMsg);
        return;
    }

    char* pBlock = (char*)pPtr - 4;
    extern const char g_szSafeHeapTag[];
    if (strcmp(pBlock, g_szSafeHeapTag) != 0)
    {
        Fmt_Sprintf(szMsg, "Invalid pointer in %s %d", pszFile, nLine);
        Debug_Trace(0, "C:\\DevStudio\\Projects\\Crux\\SAFEHEAP.cpp", szMsg);
        Debug_Assert(0, "C:\\DevStudio\\Projects\\Crux\\SAFEHEAP.cpp", szMsg);
    }

    memset(pBlock, 0, 4);  // zero tag to catch double-free
    Mem_Free(pBlock);
}
