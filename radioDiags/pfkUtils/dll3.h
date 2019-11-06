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

#ifndef __DLL3_H__
#define __DLL3_H__

#include <iostream>
#include <vector>
#include <pthread.h>
#include <inttypes.h>
#include <iostream>

#include "LockWait.h"
#include "BackTrace.h"

#ifdef __GNUC__
# if __GNUC__ >= 6
#  define ALLOW_THROWS noexcept(false)
# else
#  define ALLOW_THROWS
# endif
#else
# define ALLOW_THROWS
#endif

/** all dynamic linked list version 3 templates under this namespace */
namespace DLL3 {

/** a basic catchable error type for all DLL3 related errors. */
struct ListError : BackTraceUtil::BackTrace {
    /** a list of all the possible errors that can happen */
    enum ListErrValue {
        ITEM_NOT_VALID,   //!< a referenced item is missing MAGIC (corrupt?)
        ALREADY_ON_LIST,  //!< the item's list links are already on another list
        STILL_ON_A_LIST,  //!< can't delete an item when it's still on a list
        LIST_NOT_EMPTY,   //!< can't delete a list if it's not empty
        LIST_NOT_LOCKED,  //!< you forgot to lock a list before using it
        NOT_ON_THIS_LIST, //!< the referenced item isn't on this list
        __NUMERRS
    } err;
    static const std::string errStrings[__NUMERRS];
    ListError(ListErrValue _e) : err(_e) {
        std::cerr << "throwing ListError: " << Format() << std::endl;
    }
    /** return a descriptive string matching the error */
    /*virtual*/ const std::string _Format(void) const;
};
// internal: shorthand for throwing a list error.
#define __DLL3_LISTERR(e) throw ListError(ListError::e)

// internal: shorthand for all the template boilerplates
#define __DLL3_LIST List<T,uniqueIdentifier,lockWarn,validate>
#define __DLL3_LIST_TEMPL class T, int uniqueIdentifier, \
                           bool lockWarn, bool validate

/** a doubly-linked list head-tail container.
 * \param T the data type that will be stored in this list; note type T
 *      must derive from a matching List<...>::Links.
 * \param uniqueIdentifier a unique integer distinguishing this list from
 *      the other lists an item might be on simultaneously.
 * \param lockWarn enable diagnostic code that warns you if you forgot to
 *      lock the list (defaults on).
 * \param validate enable diagnostic code that validates magic numbers to
 *      detect races with deletions and bogus pointers. */
template <class T, int uniqueIdentifier,
          bool lockWarn=true, bool validate=true>
class List : public WaitUtil::Lockable {
public:
    /** any item that goes on this list must derive from this type.
     * \throw may throw ListError on delete */
    class Links {
        friend class __DLL3_LIST;
        static const int MAGIC = 0x5e061ed;
        int magic;
        Links * next;
        Links * prev;
        __DLL3_LIST * lst;
    public:
        Links(void);
        ~Links(void) ALLOW_THROWS;
        void checkvalid(__DLL3_LIST * _lst);
    };
private:
    Links * head;
    Links * tail;
    int cnt;
    void lockwarn(void) const;
    void _remove(Links * item);
    void _add_head(Links * item);
public:
    List(void);
    ~List(void) ALLOW_THROWS;
    /** add the item to the head of this list.
     * \param item  the item to add.
     * \throw may throw ListError */
    void add_head(Links * item);
    void add_tail(Links * item);
    void add_before(Links * item, Links * existing);
// add_after ?
    T * get_head(void);
    T * get_tail(void);
    T * get_next(Links * item);
    T * get_prev(Links * item);
    T * dequeue_head(void);
    T * dequeue_tail(void);
    // if using a List as a Least Recently Used,
    // all items should be added to head and oldest
    // removed from tail; and this promote method will
    // remove from the middle and add to head.
    void promote(T * item);
    T * get_oldest(void) { return get_tail(); }
    void remove(Links * item);
    const bool onlist(Links * item) const;
    const bool onthislist(Links * item) const;
    const int get_cnt(void) const { return cnt; }
};

#define __DLL3_HASH Hash<T,KeyT,HashT,uniqueIdentifier,lockWarn,validate>
#define __DLL3_HASH_TEMPL class T, class KeyT, class HashT, int uniqueIdentifier, bool lockWarn, bool validate

static const int dll3_num_hash_primes = 16;
extern const int dll3_hash_primes[dll3_num_hash_primes];

// HashT must look like this:
//     class HashT {
//     public:
//        static uint32_t obj2hash  (const T &)               { /*stuff*/ }
//        static uint32_t key2hash  (const KeyT &)            { /*stuff*/ }
//        static bool     hashMatch (const T &, const KeyT &) { /*stuff*/ }
//     }
// (where T and KeyT are replaced by the real T and KeyT types.

template <class T, class KeyT, class HashT, int uniqueIdentifier,
          bool lockWarn=true, bool validate=true>
class Hash : public WaitUtil::Lockable {
    typedef List<T,uniqueIdentifier,false,true> theHash;
    std::vector<theHash> * hash;
    int hashorder;
    int hashsize;
    int cnt;
    void lockwarn(void) const;
    void _rehash(int newOrder);
    void rehash(void);
public:
    class Links : public List<T,uniqueIdentifier,false,true>::Links {
        friend class __DLL3_HASH;
        static const int MAGIC = 0x68ddd8d;
        int magic;
        __DLL3_HASH * hsh;
        uint32_t h;
        void checkvalid(__DLL3_HASH * _hsh);
    public:
        Links(void);
        virtual ~Links(void) ALLOW_THROWS;
    };
    Hash(void);
    ~Hash(void) ALLOW_THROWS;
private:
    void _add(Links * item);
public:
    void add(Links * item);
    void remove(Links * item);
    T * find(const KeyT &key) const;
    const bool onlist(Links * item) const;
    const bool onthislist(Links * item) const;
    const int get_cnt(void) const { return cnt; }
};

#define __DLL3_HASHLRU HashLRU<T,KeyT,HashT,uniqueIdentifier1, \
                               uniqueIdentifier2,lockWarn,validate>
#define __DLL3_HASHLRU_TEMPL class T, class KeyT, class HashT, \
                     int uniqueIdentifier1, int uniqueIdentifier2, \
                     bool lockWarn, bool validate

template <class T, class KeyT, class HashT,
          int uniqueIdentifier1, int uniqueIdentifier2,
          bool lockWarn=true, bool validate=true>
class HashLRU : public WaitUtil::Lockable {
    List<T,uniqueIdentifier1,false,true> list;
    Hash<T,KeyT,HashT,uniqueIdentifier2,false,true> hash;
    void lockwarn(void);
public:
    class Links : public List<T,uniqueIdentifier1,false,true>::Links,
                  public Hash<T,KeyT,HashT,uniqueIdentifier2,
                              false,true>::Links {
        friend class __DLL3_HASHLRU;
        static const int MAGIC = 0x2cbee2a;
        int magic;
        __DLL3_HASHLRU * hlru;
        void checkvalid(__DLL3_HASHLRU * _hlru);
    public:
        Links(void);
        virtual ~Links(void) ALLOW_THROWS;
    };
    HashLRU(void);
    ~HashLRU(void) ALLOW_THROWS;
    void add(Links * item);
    void remove(Links * item);
    void promote(Links * item);
    T * find(const KeyT &key);
    T * get_oldest(void);
};

#include "dll3.tcc"

#undef  __DLL3_HASH
#undef  __DLL3_HASH_TEMPL
#undef  __DLL3_LIST
#undef  __DLL3_LIST_TEMPL
#undef  __DLL3_LISTERR

}; // namespace DLL3

/** \mainpage DLL3 

This is a simple template library for doing doubly-linked lists.

There is a data type DLL3::List for a simple list head/tail object.

\code
class myItem;
typedef DLL3::List<myItem,1> myItemList1_t;
typedef DLL3::List<myItem,2> myItemList2_t;
\endcode

There's a data type DLL3::Hash for doing a simple hash based on a 
lookup key. You must declare a data type for the lookup key with 
well known signatures.

\code
class myItemHashCompa {
public:
    static uint32_t obj2hash(const myItem &item)
    { return (uint32_t) item.a; }
    static uint32_t key2hash(const int &key)
    { return (uint32_t) key; }
    static bool hashMatch(const myItem &item, const int &key)
    { return (item.a == key); }
};
typedef DLL3::Hash<myItem,int,myItemHashCompa,3> myItemHash3_t;

class myItemHashCompz {
public:
    static uint32_t obj2hash(const myItem &item)
    { return (uint32_t) item.z; }
    static uint32_t key2hash(const int &key)
    { return (uint32_t) key; }
    static bool hashMatch(const myItem &item, const int &key)
    { return (item.z == key); }
};
typedef DLL3::Hash<myItem,int,myItemHashCompz,4> myItemHash4_t;
\endcode

Any item which goes any of these lists or hashes must be derived from
DLL3::List::Links or DLL3::Hash::Links.  An item can exist on multiple
lists at the same time as long as the uniqueIdentifer is different and
there's multiple inheritance.

\code
class myItem : public someBase,
               public myItemList1_t::Links,
               public myItemList2_t::Links,
               public myItemHash3_t::Links,
               public myItemHash4_t::Links
{
public:
    myItem(int _a, int _z) : someBase(_z), a(_a) { }
    int a;
};
\endcode

here is some sample code which adds and removes items from lists
and hashes and keeps items on multiple lists at once:

\code
class someBase {
public:
    int z;
    someBase(int _z) : z(_z) { }
};
std::ostream& operator<< (std::ostream &ostr, const someBase *it)
{
    ostr << "z=" << it->z;
    return ostr;
}

std::ostream& operator<< (std::ostream &ostr, const myItem *it)
{
    ostr << "a=" << it->a << ", " << (const someBase *)it;
    return ostr;
}

int
main()
{
    myItemList1_t  lst1;
    myItemList2_t  lst2;
    myItemHash3_t  hash3;
    myItemHash4_t  hash4;
    myItem * i, * ni;

    {
        WaitUtil::Lock lck1(&lst1);
        WaitUtil::Lock lck2(&lst2);
        WaitUtil::Lock lck3(&hash3);
        WaitUtil::Lock lck4(&hash4);

        i = new myItem(4,1);
        lst1.add_tail(i);
        lst2.add_head(i);
        hash3.add(i);
        hash4.add(i);

        i = new myItem(5,2);
        lst1.add_tail(i);
        lst2.add_head(i);
        hash3.add(i);
        hash4.add(i);

        i = new myItem(6,3);
        lst1.add_tail(i);
        lst2.add_head(i);
        hash3.add(i);
        hash4.add(i);
    }

    {
        WaitUtil::Lock lck3(&hash3);
        i = hash3.find(6);
        if (i)
            std::cout << "search for a=6: " << i << std::endl;

        i = hash3.find(4);
        if (i)
            std::cout << "search for a=4: " << i << std::endl;
    }

    {
        WaitUtil::Lock lck4(&hash4);
        i = hash4.find(2);
        if (i)
            std::cout << "search for z=2: " << i << std::endl;

        i = hash4.find(3);
        if (i)
            std::cout << "search for z=3: " << i << std::endl;
    }

    {
        WaitUtil::Lock lck1(&lst1);
        for (i = lst1.get_head(); i; i = ni)
        {
            ni = lst1.get_next(i);
            std::cout << "lst1 item : " << i << std::endl;
            lst1.remove(i);
            WaitUtil::Lock lck3(&hash3);
            hash3.remove(i);
        }
    }

    {
        WaitUtil::Lock lck2(&lst2);
        while ((i = lst2.dequeue_head()) != NULL)
        {
            std::cout << "lst2 item : " << i << std::endl;
            WaitUtil::Lock lck4(&hash4);
            hash4.remove(i);
            delete i;
        }
    }

    return 0;
}
\endcode



 */

#endif /* __DLL3_H__ */
