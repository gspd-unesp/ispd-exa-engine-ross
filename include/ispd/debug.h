#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_ON

#ifdef DEBUG_ON
#define DEBUG_BLOCK(CODE) CODE
#else
#define DEBUG_BLOCKC(CODE)
#endif // DEBUG_ON

#endif // DEBUG_H
