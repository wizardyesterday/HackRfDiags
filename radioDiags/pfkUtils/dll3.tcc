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

template <__DLL3_LIST_TEMPL>
__DLL3_LIST::Links::Links(void)
{
    next = prev = NULL;
    lst = NULL;
    magic = MAGIC;
}

template <__DLL3_LIST_TEMPL>
__DLL3_LIST::Links::~Links(void) ALLOW_THROWS
{
    if (validate && lst != NULL)
        __DLL3_LISTERR(STILL_ON_A_LIST);
    magic = 0;
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::Links::checkvalid(__DLL3_LIST * _lst)
{
    if (!validate)
        return;
    if (magic != MAGIC)
        __DLL3_LISTERR(ITEM_NOT_VALID);
    if (_lst == NULL && lst != NULL)
        __DLL3_LISTERR(ALREADY_ON_LIST);
    else if (lst != _lst)
        __DLL3_LISTERR(NOT_ON_THIS_LIST);
}

template <__DLL3_LIST_TEMPL>
__DLL3_LIST::List(void)
{
    head = tail = NULL;
    cnt = 0;
}

template <__DLL3_LIST_TEMPL>
__DLL3_LIST::~List(void) ALLOW_THROWS
{
    if (validate && head != NULL)
        __DLL3_LISTERR(LIST_NOT_EMPTY);
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::lockwarn(void) const
{
    if (lockWarn == true && isLocked() == false)
        __DLL3_LISTERR(LIST_NOT_LOCKED);
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::_add_head(Links * item)
{
    item->next = head;
    item->prev = NULL;
    if (head)
        head->prev = item;
    else
        tail = item;
    head = item;
    cnt++;
    item->lst = this;
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::add_head(Links * item)
{
    lockwarn();
    item->checkvalid(NULL);
    _add_head(item);
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::add_tail(Links * item)
{
    lockwarn();
    item->checkvalid(NULL);
    item->next = NULL;
    item->prev = tail;
    if (tail)
        tail->next = item;
    else
        head = item;
    tail = item;
    cnt++;
    item->lst = this;
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::add_before(Links * item, Links * existing)
{
    lockwarn();
    item->checkvalid(NULL);
    existing->checkvalid(this);
    item->prev = existing->prev;
    item->next = existing;
    existing->prev = item;
    if (item->prev)
        item->prev->next = item;
    else
        head = item;
    cnt++;
    item->lst = this;
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::get_head(void)
{
    lockwarn();
    return static_cast<T*>(head);
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::get_tail(void)
{
    lockwarn();
    return static_cast<T*>(tail);
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::get_next(Links * item)
{
    lockwarn();
    item->checkvalid(this);
    return static_cast<T*>(item->next);
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::get_prev(Links * item)
{
    lockwarn();
    item->checkvalid(this);
    return static_cast<T*>(item->prev);
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::_remove(Links * item)
{
    if (item->next)
        item->next->prev = item->prev;
    else
        tail = item->prev;
    if (item->prev)
        item->prev->next = item->next;
    else
        head = item->next;
    item->next = item->prev = NULL;
    cnt--;
    item->lst = NULL;
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::remove(Links * item)
{
    lockwarn();
    item->checkvalid(this);
    _remove(item);
}

template <__DLL3_LIST_TEMPL>
const bool __DLL3_LIST::onlist(Links * item) const
{
    lockwarn();
    if (validate)
    {
        if (item->magic != Links::MAGIC)
            __DLL3_LISTERR(ITEM_NOT_VALID);
    }
    if (item->lst != NULL)
        return true;
    return false;
}

template <__DLL3_LIST_TEMPL>
const bool __DLL3_LIST::onthislist(Links * item) const
{
    lockwarn();
    if (validate)
    {
        if (item->magic != Links::MAGIC)
            __DLL3_LISTERR(ITEM_NOT_VALID);
    }
    if (item->lst == this)
        return true;
    return false;
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::dequeue_head(void)
{
    lockwarn();
    Links * item = head;
    if (item)
        _remove(item);
    return static_cast<T*>(item);
}

template <__DLL3_LIST_TEMPL>
T * __DLL3_LIST::dequeue_tail(void)
{
    lockwarn();
    Links * item = head;
    if (item)
        _remove(item);
    return static_cast<T*>(item);
}

template <__DLL3_LIST_TEMPL>
void __DLL3_LIST::promote(T * item)
{
    lockwarn();
    item->checkvalid(this);
    _remove(item);
    _add_head(item);
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH::lockwarn(void) const
{
    if (lockWarn == true && isLocked() == false)
        __DLL3_LISTERR(LIST_NOT_LOCKED);
}

template <__DLL3_HASH_TEMPL>
__DLL3_HASH::Links::Links(void)
{
    magic = MAGIC;
    hsh = NULL;
    h = 0xFFFFFFFF;
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH::Links::checkvalid(__DLL3_HASH * _hsh)
{
    if (validate && (magic != MAGIC))
        __DLL3_LISTERR(ITEM_NOT_VALID);
    if (_hsh == NULL && hsh != NULL)
        __DLL3_LISTERR(ALREADY_ON_LIST);
    else if (hsh != _hsh)
        __DLL3_LISTERR(NOT_ON_THIS_LIST);
}

template <__DLL3_HASH_TEMPL>
__DLL3_HASH::Links::~Links(void) ALLOW_THROWS
{
    if (validate && hsh != NULL)
        __DLL3_LISTERR(LIST_NOT_EMPTY);
    magic = 0;
}

template <__DLL3_HASH_TEMPL>
__DLL3_HASH :: Hash(void)
{
    hashorder = 0;
    hashsize = dll3_hash_primes[hashorder];
    hash = new std::vector<theHash>;
    hash->resize(hashsize);
    cnt = 0;
}

template <__DLL3_HASH_TEMPL>
__DLL3_HASH :: ~Hash(void) ALLOW_THROWS
{
    delete hash;
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH :: _add(Links * item)
{
    T * derived = dynamic_cast<T*>(item);
    item->h = HashT::obj2hash(*derived) % hashsize;
    (*hash)[item->h].add_tail(derived);
    item->hsh = this;
    cnt++;
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH :: add(Links * item)
{
    lockwarn();
    item->checkvalid(NULL);
    _add(item);
    rehash();
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH :: remove(Links * item)
{
    lockwarn();
    item->checkvalid(this);
    T * derived = dynamic_cast<T*>(item);
    (*hash)[item->h].remove(derived);
    item->hsh = NULL;
    cnt--;
    rehash();
}

template <__DLL3_HASH_TEMPL>
T * __DLL3_HASH :: find(const KeyT &key) const
{
    lockwarn();
    uint32_t h = HashT::key2hash(key) % hashsize;
    theHash &lst = (*hash)[h];
    for (T * item = lst.get_head();
         item != NULL;
         item = lst.get_next(item))
    {
        if (HashT::hashMatch(*item,key))
            return item;
    }
    return NULL;
}

template <__DLL3_HASH_TEMPL>
const bool __DLL3_HASH :: onlist(Links * item) const
{
    lockwarn();
    if (item->hsh != NULL)
        return true;
    return false;
}

template <__DLL3_HASH_TEMPL>
const bool __DLL3_HASH :: onthislist(Links * item) const
{
    lockwarn();
    if (item->hsh == this)
        return true;
    return false;
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH :: rehash(void)
{
    int average = cnt / hashsize;
    if (average  > 5 && hashorder < dll3_num_hash_primes)
        _rehash(hashorder+1);
    if (average == 0 && hashorder > 0)
        _rehash(hashorder-1);
}

template <__DLL3_HASH_TEMPL>
void __DLL3_HASH :: _rehash(int newOrder)
{
    std::vector<theHash> * oldHash = hash;
    int oldHashSize = hashsize;
    hashorder = newOrder;
    hashsize = dll3_hash_primes[newOrder];
    hash = new std::vector<theHash>;
    hash->resize(hashsize);
    for (int h = 0; h < oldHashSize; h++)
        while ( T * t = (*oldHash)[h].dequeue_head())
            _add( t );
    delete oldHash;
}


template <__DLL3_HASHLRU_TEMPL>
__DLL3_HASHLRU :: Links :: Links(void)
{
    magic = MAGIC;
}

template <__DLL3_HASHLRU_TEMPL>
__DLL3_HASHLRU :: Links :: ~Links(void) ALLOW_THROWS
{
    if (validate && (hlru != NULL))
        __DLL3_LISTERR(LIST_NOT_EMPTY);
    magic = 0;
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: Links :: checkvalid(__DLL3_HASHLRU * _hlru)
   
{
    if (validate && (magic != MAGIC))
        __DLL3_LISTERR(ITEM_NOT_VALID);
    if (_hlru == NULL && hlru != NULL)
        __DLL3_LISTERR(ALREADY_ON_LIST);
    else if (hlru != _hlru)
        __DLL3_LISTERR(NOT_ON_THIS_LIST);
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: lockwarn(void)
{
    if (lockWarn == true && isLocked() == false)
        __DLL3_LISTERR(LIST_NOT_LOCKED);
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: add(Links * item)
{
    lockwarn();
    item->checkvalid(NULL);
    T * derived = dynamic_cast<T*>(item);
    list.add_tail(derived);
    hash.add(derived);
    item->hlru = this;
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: remove(Links * item)
{
    lockwarn();
    item->checkvalid(this);
    T * derived = dynamic_cast<T*>(item);
    list.remove(derived);
    hash.remove(derived);
    item->hlru = NULL;
}

template <__DLL3_HASHLRU_TEMPL>
void __DLL3_HASHLRU :: promote(Links * item)
{
    lockwarn();
    item->checkvalid(this);
    T * derived = dynamic_cast<T*>(item);
    list.remove(derived);
    list.add_tail(derived);
}

template <__DLL3_HASHLRU_TEMPL>
T * __DLL3_HASHLRU :: find(const KeyT &key)
{
    lockwarn();
    return hash.find(key);
}

template <__DLL3_HASHLRU_TEMPL>
T * __DLL3_HASHLRU :: get_oldest(void)
{
    lockwarn();
    return list.get_head();
}
