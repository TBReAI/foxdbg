/***************************************************************
**
** TBReAI Header File
**
** File         :  foxdbg_concurrent.h
** Module       :  foxdbg
** Author       :  SH
** Created      :  2025-04-14 (YYYY-MM-DD)
** License      :  MIT
** Description  :  Foxglove Debug Server
**
***************************************************************/

#ifndef FOXDBG_CONCURRENT_H
#define FOXDBG_CONCURRENT_H

#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************
** MARK: INCLUDES
***************************************************************/

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <stdint.h>
#endif

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

#ifdef _WIN32
    typedef volatile LONG foxdbg_flag_t;

    #define foxdbg_flag_set(pflag) \
        InterlockedExchange((LONG*)(pflag), 1)

    #define foxdbg_flag_clear(pflag) \
        InterlockedExchange((LONG*)(pflag), 0)

    #define foxdbg_flag_get(pflag) \
        InterlockedCompareExchange((LONG*)(pflag), 0, 0)

#else
    typedef volatile int foxdbg_flag_t;

    #define foxdbg_flag_set(pflag) \
        __atomic_store_n((pflag), 1, __ATOMIC_SEQ_CST)

    #define foxdbg_flag_clear(pflag) \
        __atomic_store_n((pflag), 0, __ATOMIC_SEQ_CST)

    #define foxdbg_flag_get(pflag) \
        __atomic_load_n((pflag), __ATOMIC_SEQ_CST)
#endif

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: FUNCTION DEFS
***************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* FOXDBG_CONCURRENT_H */