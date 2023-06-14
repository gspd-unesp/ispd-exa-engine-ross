#ifndef ISPD_CORE_H
#define ISPD_CORE_H

#define DEBUG_ON

#ifdef DEBUG_ON
#define DEBUG_BLOCK(CODE) CODE
#else
#define DEBUG_BLOCK(CODE)
#endif // DEBUG_ON

#ifdef __GNUC__
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect((expr), 0)
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif // __GNUC__

#endif // ISPD_CORE_H
