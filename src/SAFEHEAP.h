#pragma once
// SAFEHEAP.cpp — Safe heap allocator with magic-tag integrity checking

void* SafeHeap_Alloc(const char* pszFile, int nLine, int nSize);
void  SafeHeap_Free(const char* pszFile, int nLine, void* pPtr);

// Convenience macros matching original call-site usage
#define SAFE_ALLOC(size)  SafeHeap_Alloc(__FILE__, __LINE__, (size))
#define SAFE_FREE(ptr)    SafeHeap_Free(__FILE__, __LINE__, (ptr))
