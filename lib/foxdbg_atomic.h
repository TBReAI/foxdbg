/***************************************************************
**
** TBReAI Header File
**
** File         :  foxdbg_atomic.h
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-24 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
**
***************************************************************/

#ifndef FOXDBG_ATOMIC_H
#define FOXDBG_ATOMIC_H

/***************************************************************
** MARK: INCLUDES
***************************************************************/


#if defined(_MSC_VER)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <intrin.h>
    #pragma intrinsic(_mm_mfence)
#elif defined(__GNUC__) || defined(__clang__)
    #include <sched.h>
#else
    #error "Unsupported compiler - implement atomic operations for your compiler"
#endif


/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#if defined(_MSC_VER)
    #define YIELD_CPU() Sleep(0)
    #define ATOMIC_READ_INT(ptr) (_mm_mfence(), InterlockedCompareExchange((volatile LONG *)(ptr), 0, 0))
    #define ATOMIC_WRITE_INT(ptr, val) (_mm_mfence(), InterlockedExchange((volatile LONG *)(ptr), (val)), _mm_mfence())
#elif defined(__GNUC__) || defined(__clang__)
    #define YIELD_CPU() sched_yield()
    #define ATOMIC_READ_INT(ptr) __atomic_load_n((ptr), __ATOMIC_SEQ_CST)
    #define ATOMIC_WRITE_INT(ptr, val) __atomic_store_n((ptr), (val), __ATOMIC_SEQ_CST)
#else
    #error "Unsupported compiler - implement atomic operations for your compiler"
#endif

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/


/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/



#endif /* FOXDBG_ATOMIC_H */