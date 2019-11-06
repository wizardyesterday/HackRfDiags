/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#ifndef __PFKMATH_H__
#define __PFKMATH_H__

#ifdef __cplusplus
extern "C" {
#endif

enum m_math_retvals {
	M_MATH_OK = 0,
	M_MATH_SYNTAX,
	M_MATH_NOFLAGS,
	M_MATH_STACKFULL,
	M_MATH_TOOMANYARGS,
	M_MATH_EMPTYSTACK,
	M_MATH_STACKNOTONE,
	M_MATH_REGERR,
	M_MATH_TOOFEWARGS,
	M_MATH_OVERFLOW,
	M_MATH_DIVIDE_BY_ZERO,
	M_MATH_PARSEVALUEERR,
	M_MATH_OP1ERR,
	M_MATH_OP2ERR
};

#define M_INT64  unsigned long long

/* when this returns something other than M_MATH_OK,
   "result" is actually a pointer to a descriptive error string.
   also, if you specify "flags" as null, it will not allow flags
   as arguments and will return M_MATH_NOFLAGS if you specify
   flag arguments but flags==NULL.
*/

enum m_math_retvals m_do_math( int argc, char ** argv,
                               M_INT64 *result, int *flags );

/* this returns a static global pointer, so it does not have
   to be freed, however it doesn't survive two successive calls. */
char * m_dump_number( M_INT64 num, int base );

/* this parses the string in the given base and either returns
   zero with a valid number, or returns -1 with no valid number.
   0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz  */
int    m_parse_number( M_INT64 * result, char * string, int len, int base );

#ifdef __cplusplus
};
#endif

#endif /* __PFKMATH_H__ */
