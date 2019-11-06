/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */
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

#ifndef __pfkbitmap_h__
#define __pfkbitmap_h__ 1

template <int numbits>
class Bitmap {
    static const int bits_per_word = sizeof(unsigned long) * 8;
    static const int digits_per_word = sizeof(unsigned long) * 2;
    static const int array_size = (numbits + bits_per_word - 1) / bits_per_word;
    static const unsigned long thebit = 1;
    unsigned long words[array_size];
public:
    Bitmap(void) { memset(words, 0, sizeof(words)); }
    bool set(int bitno) {
        if (bitno >= numbits)
            return false;
        int word = bitno / bits_per_word;
        int bit = bitno % bits_per_word;
        words[word] |= (thebit << bit);
        return true;
    }
    bool clear(int bitno) {
        if (bitno >= numbits)
            return false;
        int word = bitno / bits_per_word;
        int bit = bitno % bits_per_word;
        words[word] &= ~(thebit << bit);
        return true;
    }
    bool isset(int bitno) {
        if (bitno >= numbits)
            return false;
        int word = bitno / bits_per_word;
        int bit = bitno % bits_per_word;
        return ((words[word] & (thebit << bit)) != 0);
    }
    std::string print(void) {
        std::ostringstream str;
        str << "words: " << std::hex
            << std::setw(digits_per_word) << std::setfill('0');
        for (int word = 0; word < array_size; word++)
            str << words[word] << " ";
        str << std::endl;
        str << "bits: " << std::dec << std::setw(0);
        for (int bit = 0; bit < numbits; bit++)
            if (isset(bit))
                str << bit << " ";
        str << std::endl;
        return str.str();
    }
};

#endif /* __pfkbitmap_h__ */
