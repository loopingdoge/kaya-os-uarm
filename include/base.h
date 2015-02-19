/**
 * @file base.h
 * @brief utility types
 */

/**************************** Utility types *********************************/

/* IMPORTANT: the following definitions (and thus the whole OS) assume that:

  sizeof(int) == 4, both signed and unsigned
	sizeof(short int) == 2, both signed and unsigned
	sizeof(char) == 1, both signed and unsigned

  CHAR_BITS is 8 (1 byte is 8 bits)
	size of a word == size of a memory address == sizeof(unsigned int) == 4 bytes

  The next 5 definitions state this explicitly.
*/

#ifndef BASE_INCLUDED
#define BASE_INCLUDED

typedef unsigned int U32;
typedef signed int S32;
typedef unsigned char U8;
typedef signed char S8;

typedef unsigned int memaddr;

#endif

