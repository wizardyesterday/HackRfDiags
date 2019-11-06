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

#include "dll3.h"

using namespace DLL3;

const std::string ListError::errStrings[__NUMERRS] = {
    "item not valid",
    "item already on list",
    "item still on a list",
    "list not empty",
    "list not locked",
    "item not on this list"
};

//virtual
const std::string
ListError::_Format(void) const
{
    std::string ret = "LIST ERROR: ";
    ret += errStrings[err];
    ret += " at:\n";
    return ret;
}

const int DLL3::dll3_hash_primes[dll3_num_hash_primes] = {
    97,     251,     499,
    997,    1999,    4999,
    9973,   24989,   49999,
    99991,  249989,  499979,
    999983, 2499997, 4999999,
    9999991
};
