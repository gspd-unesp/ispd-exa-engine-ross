#ifndef ISPD_DEBUG_HPP
#define ISPD_DEBUG_HPP

#define DEBUG_ON

#ifdef DEBUG_ON
# define DEBUG(CODE) CODE
#else
# define DEBUG(CODE)
#endif // DEBUG_ON

#endif // ISPD_DEBUG_HPP
