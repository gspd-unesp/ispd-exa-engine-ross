#ifndef ISPD_CORE_H
#define ISPD_CORE_H


#ifdef DEBUG_ON
#	define DEBUG_BLOCK(CODE) CODE
#else
#	define DEBUG_BLOCK(CODE)
#endif // DEBUG_ON

#ifdef __GNUC__
#	define likely(expr)   __builtin_expect(!!(expr), 1)
#	define unlikely(expr) __builtin_expect((expr), 0)
#else
#	define likely(expr)   (expr)
#	define unlikely(expr) (expr)
#endif // __GNUC__

#define ENGINE_USE_INLINE
#define ENGINE_FORCE_INLINE

#ifdef ENGINE_USE_INLINE
#	if defined(__GNUC__) && defined(ENGINE_FORCE_INLINE)
#		define ENGINE_INLINE inline __attribute__((always_inline))
#	else
#		define ENGINE_INLINE inline
#	endif // ENGINE_FORCE_INLINE
#else
#	define ENGINE_INLINE
#endif // ENGINE_USE_INLINE

#endif // ISPD_CORE_H
