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

#ifndef __DLL3_BTREE_H__
#define __DLL3_BTREE_H__ 1

#include <string.h>
#include <stdio.h>

#include "dll3.h"

namespace DLL3 {

struct BtreeError : BackTraceUtil::BackTrace {
    enum BtreeErrorValue {
        ALREADY_ON_LIST,
        DUPLICATE_ITEM,
        BTREE_EMPTY,
        ITEM_NOT_FOUND,
        DIDNT_FIND_SAME_ITEM,
        SCREWED_UP_ROOT,
        SCREWED_UP_LEFT_SIB,
        SCREWED_UP_RIGHT_SIB,
        CANT_STEAL_OR_COALESCE,
        NONROOT_NODE_SHRUNK,
        __NUMERRS
    } err;
    static const std::string errStrings[__NUMERRS];
    BtreeError(BtreeErrorValue _e) : err(_e) {
        std::cerr << "throwing BtreeError: " << Format() << std::endl;
    }
    /** return a descriptive string matching the error */
    /*virtual*/ const std::string _Format(void) const;
};
#define __DLL3_BTREEERR(e) throw BtreeError(BtreeError::e)

// this is an internal type used by BTREE

template <class T, int ORDER>
struct BTREENode : DLL3::List<
    BTREENode<T,ORDER>,ORDER,false,true>::Links
{
    typedef DLL3::List<BTREENode,ORDER,false,true> List_t;
    char numkeys;
    char cur;
    bool leaf;
    bool root;
    BTREENode * nodes[ORDER];
    T * keys[ORDER-1];
    BTREENode( void )
    {
        int i;
        numkeys = cur = 0;
        leaf = root = false;
        for (i=0; i < (ORDER); i++)
            nodes[i] = NULL;
        for (i=0; i < (ORDER-1); i++)
            keys[i] = NULL;
    }
};

// to use BTREE, type T must have a companion comparator class
// which provides information about how to do keying on the item. This
// comparator class must have the following static members:
//
//    static int key_compare( const T &item, const BTREE_Key_Type &key )
//    static int key_compare( const T &item, const T &item2 )
//    static std::string key_format( const T &item )
//
// the first 2 methods must return -1 if the item.key is less than key,
// 0 if the item.key is equal to the key, 1 if the item.key is 
// greator than key.
//
// key_format should return a string formatting the contents of item.
// this will be used by printtree(). if you do not intend to use printtree(),
// then you may of course choose to leave the body of this dummied--i.e.,
// return "";

template <class T, class BTREE_Key_Type,
          class BTREE_Key_Comparator, int instance, int ORDER>
class BTREE : public WaitUtil::Lockable {
    static const int HALF_ORDER = ORDER / 2;
    static const int ORDER_MO   = ORDER - 1;
    static const int ORDER_MT   = ORDER - 2;
    typedef BTREENode <T,ORDER>  NODE;
    typedef typename NODE::List_t NODESTORE;
    typedef DLL3::List <T,instance,false,true> itemsList_t;
    itemsList_t items;
public:
    typedef typename itemsList_t::Links  Links;
private:
    NODE * root;
    int numnodes;
    int numitems;
    int depth;

    void empty_store( NODESTORE * store )
    {
        while ( store->get_cnt() > 0 )
            store->remove( store->get_tail() );
    }

    // utility function for splitting a full node into
    // two nodes. the node to split is 'n'; the item *nt
    // is the new item to add; this function also figures out
    // which item is the 'pivot' item, the item which will go
    // in the parent of the two new nodes, in between the pointers
    // to these nodes.  *nt is updated when the pivot record is
    // discovered.  also as we go up through the tree towards the
    // root, we update the 'right' pointer by returning a new one.

    NODE * splitnode( NODE * n, T ** nt, NODE * right )
    {
        NODE * newright = new NODE;
        int i;

        numnodes++;
        newright->numkeys = HALF_ORDER;
        n->numkeys = HALF_ORDER;

        newright->root = false;
        if ( n->leaf )
            newright->leaf = true;

        // the way the recs are moved around
        // depends on where nt goes. if nt is not
        // the pivot, the pivot is intentionally
        // left as the rightmost rec of the leftnode,
        // so that it can be collected after this if-block;
        // determine if our new rec is the pivot,
        // if it belongs in the left node, or in the right node.

        // the way we determine where the pivot, is by looking
        // at 'cur' in the node, which indicates where the
        // nodewalking stopped in this node.

        if ( n->cur < HALF_ORDER )
        {
            // copy right half of left node into right node.
            for ( i = 0; i < HALF_ORDER; i++ )
            {
                newright->keys [i] = n->keys [ i + HALF_ORDER ];
                newright->nodes[i] = n->nodes[ i + HALF_ORDER ];
                n->keys [ i + HALF_ORDER ] = 0;
                n->nodes[ i + HALF_ORDER ] = 0;
            }
            newright->nodes[i] = n->nodes[ i + HALF_ORDER ];

            // then slide over remaining components of left
            // node that must move to accommodate new item.
            for ( i = (HALF_ORDER-1); i >= n->cur; i-- )
            {
                n->keys [i+1] = n->keys [i];
                n->nodes[i+2] = n->nodes[i+1];
            }

            // then insert new item.
            n->keys [(int) n->cur  ] = *nt;
            n->nodes[(int) n->cur+1] = right;

            // new pivot is left in leftnode.
            *nt = n->keys[HALF_ORDER];
            n->keys[HALF_ORDER] = 0;
        }
        else if ( n->cur == HALF_ORDER )
        {
            // the new node is the pivot, so don't update 
            // nt; just split leftnode into rightnode and 
            // we're done. right half of nodes in leftnode
            // plus their rightpointers are copied. the first
            // left ptr of the new rightnode points to the
            // passed-in rightnode.

            newright->nodes[0] = right;
            for ( i = 0; i < HALF_ORDER; i++ )
            {
                newright->keys [i]   = n->keys [ i + HALF_ORDER ];
                newright->nodes[i+1] = n->nodes[ i + HALF_ORDER + 1 ];
                n->keys [ i + HALF_ORDER ] = 0;
                n->nodes[ i + HALF_ORDER + 1 ] = 0;
            }
        }
        else
        {
            // start copying things over to the right;
            // when we come to the place where nt is 
            // supposed to go, insert it.

            n->cur -= HALF_ORDER+1;
            int j = HALF_ORDER+1;
            newright->nodes[0] = n->nodes[j];
            n->nodes[j] = 0;

            for ( i = 0; i < HALF_ORDER; i++ )
            {
                if ( i == n->cur )
                {
                    // this is where nt goes
                    newright->keys [i]   = *nt;
                    newright->nodes[i+1] = right;
                }
                else
                {
                    newright->keys [i]   = n->keys [j];
                    newright->nodes[i+1] = n->nodes[j+1];
                    n->keys [j]   = 0;
                    n->nodes[j+1] = 0;
                    j++;
                }
            }

            // new pivot is left in leftnode
            *nt = n->keys[HALF_ORDER];
            n->keys[HALF_ORDER] = 0;
        }

        return newright;
    }
    void _printtree( NODE * n ) const
    {
        int i;
        printf( "node at %#lx, numkeys = %d, root = %d, leaf = %d\n",
                (unsigned long)n, n->numkeys, n->root, n->leaf );
        for ( i = 0 ; i < n->numkeys; i++ )
        {
            if ( n->nodes[i] )
                _printtree( n->nodes[i] );
            if ( n->keys[i] )
                printf( "node at %#lx index %d -> %s\n",
                        (unsigned long)n, i, 
                        BTREE_Key_Comparator::key_format(
                            *n->keys[i]).c_str());
        }
        if ( n->nodes[i] )
            _printtree( n->nodes[i] );
    }
public:
    BTREE( void )
    {
        root = new NODE;
        root->leaf = root->root = true;
        numnodes = 1;
        numitems = 0;
        depth = 1;
    }
    ~BTREE( void )
    {
        delete root;
    }
    T * find( const BTREE_Key_Type &key ) const
    {
        NODE * n = root;
        T * t;
        int i,v;
        if ( numitems == 0 )
            return 0;
        while ( 1 )
        {
            for ( i = 0; i < n->numkeys; i++ )
            {
                t = n->keys[i];
                v = BTREE_Key_Comparator::key_compare(*t, key);
                if ( v == 0 )
                    return t;
                if ( v < 0 )
                    break;
            }
            if ( n->leaf )
                return 0;
            n = n->nodes[i];
        }
    }
    void add( T * nt )
    {
        if ( items.onlist(nt))
        {
            __DLL3_BTREEERR(ALREADY_ON_LIST);
        }

        items.add_tail(nt);

        NODESTORE store;
        NODE * n = root;
        int i;

        // start walking down from the rootnode a level at a time,
        // until we either find an exact match or we hit a leaf.
        // each time we go down a level, add an element to the store
        // linked-list so that we can trace our path back up the tree
        // if we need to split and promote.

        while ( 1 )
        {
            for ( n->cur = 0; n->cur < n->numkeys; n->cur++ )
            {
                int k = 
                    BTREE_Key_Comparator::key_compare(
                        *n->keys[(int)n->cur], *nt);
                if ( k == 0 )
                {
                    __DLL3_BTREEERR(DUPLICATE_ITEM);
                }
                if ( k <= 0 )
                    break;
            }

            store.add_tail( n );

            // break out if we're at the leaf node, or
            // proceed to the next level down the tree.

            if ( n->leaf )
                break;

            n = n->nodes[(int)n->cur];
        }

        // if we hit this point that means we haven't seen an
        // exact match, so we should insert at the leaf level.
        // if we overflow a node here, split the node and start
        // promoting up the tree. keep splitting and promoting up
        // until we reach a node where we're not overflowing a node.
        // if we reach the root and we overflow the root, split the
        // root and create a new root with the promoted item.

        NODE * right = 0;
        while ( n != 0 )
        {
            if ( n->numkeys < ORDER_MO )
            {
                // this node is not full, so we can insert
                // into this node at the appropriate index,
                // and be done. slide over recs in this node
                // to make room for the new rec.

                n->numkeys++;
                for ( i = ORDER_MO; i > n->cur; i-- )
                    n->nodes[i] = n->nodes[i-1];
                for ( i = ORDER_MT; i >= n->cur; i-- )
                    n->keys[i] = n->keys[i-1];
                n->keys [(int)n->cur]   = nt;
                n->nodes[(int)n->cur+1] = right;
                numitems++;
                break;
            }

            // this node is full, so split it and prepare to 
            // promote a pivot record. split_node locates the pivot
            // and updates 'newr' to point to the pivot. it may
            // also update nt to point to a new pivot record.

            right = splitnode( n, &nt, right );
            store.remove( n );
            n = store.get_tail();

            if ( n == 0 )
            {
                // we're at the root node, so we must create
                // a new root node and insert the pivot record
                // by itself here.

                NODE * newroot = new NODE;
                newroot->root = true;
                newroot->numkeys = 1;
                root ->root = false;
                right->root = false;
                newroot->nodes[0] = root;
                newroot->nodes[1] = right;
                newroot->keys [0] = nt;
                numnodes++;
                numitems++;
                depth++;
                root = newroot;
            }
        }
        empty_store( &store );
    }

    void remove( T * dt )
    {
        NODESTORE store;
        NODE * n = root, * foundn = 0;
        int i, foundnindex;

        // begin walking down the tree looking for the
        // item. at each node, store a pointer and index
        // to the node in the nodstore.
        // if we find an exact match, bail out even if its
        // not the leaf.

        if ( numitems == 0 )
        {
            __DLL3_BTREEERR(BTREE_EMPTY);
        }
        bool exact = false;

        while ( 1 )
        {
            for ( i = 0; i < n->numkeys; i++ )
            {
                int k =
                    BTREE_Key_Comparator::key_compare(
                        *n->keys[i], *dt);
                if ( k == 0 )
                    exact = true;
                if ( k <= 0 )
                    break;
            }

            n->cur = i;
            store.add_tail( n );

            if ( exact || n->leaf )
                break;

            n = n->nodes[i];
        }

        // observe carefully that the 'i' value in the above
        // for loop actually must properly survive until quite 
        // a ways down (a for-loop with no init-case).

        if ( !exact )
        {
            __DLL3_BTREEERR(ITEM_NOT_FOUND);
        }
        if ( n->keys[(int)n->cur] != dt )
        {
            __DLL3_BTREEERR(DIDNT_FIND_SAME_ITEM);
        }

        // store the ref to the node which contains the deleted item.
        foundn = n;
        foundnindex = n->cur;

        // go to the right on this last node.
        n->cur++;

        // if we're not at the leaf, continue walking down to the 
        // leaf, and store the path we used to get there. the first
        // time, go to the right, from then on go to leftmost.
        // this way we can end up at the smallest node to the right
        // of the target (for pulling up).

        bool first = true;
        while ( ! n->leaf )
        {
            n = n->nodes[ first ? n->cur : 0 ];

            first = false;
            n->cur = 0;
            store.add_tail( n );

            // yes, setting i here affects the for loop below, but
            // it is the desired behavior.

            i = 0; 
        }

        // if we've walked down to a leaf where the deleted item
        // was in a nonleaf, pull up the smallest leaf item into
        // the deleted item's slot.

        if ( foundn != n )
            foundn->keys[ foundnindex ] = n->keys[ i ];

        // now in the leaf, slide over the remaining items 
        // to take up the space of the item that was removed.
        // don't need to mess with ptrs because we know
        // that this is a leaf.  note that there is no init
        // of 'i', this is intentional; see above.

        for ( ; i < ORDER_MO; i++ )
            n->keys[ i ] = n->keys[ i+1 ];

        // clear out the right-hand slot left behind by the slide.
        n->keys[ ORDER_MT ] = 0;
        n->numkeys--;

        // start analyzing nodes for redistribution.
        // return back up the list of nodes until we 
        // no longer need to steal or coalesce.

        n = store.get_tail();
        while ( n )
        {
            NODE * delete_item = NULL;

            if (( n->numkeys >= HALF_ORDER ) || ( n->root ))
                break;

            NODE *parent, *l_sib = 0, *r_sib = 0;
            enum sibwho { SIB_NONE, SIB_LEFT, SIB_RIGHT }; // sibling
            sibwho whichsib_steal = SIB_NONE, whichsib_coalesce = SIB_NONE;

            parent = store.get_prev( n );
            if ( !parent )
            {
                __DLL3_BTREEERR(SCREWED_UP_ROOT);
            }

            // check left sib -- if more than half full we can use 
            // left sib's rightmost entry to steal; if its only
            // exactly half full, mark it as candidate for coalescing.
            // first check that we actually have a left sib.

            if ( parent->cur > 0 )
            {
                l_sib = parent->nodes[ parent->cur-1 ];
                if ( !l_sib )
                {
                    __DLL3_BTREEERR(SCREWED_UP_LEFT_SIB);
                }
                if ( l_sib->numkeys > HALF_ORDER )
                    whichsib_steal = SIB_LEFT;
                else
                    whichsib_coalesce = SIB_LEFT;
            }

            // if we can steal from the left, don't bother
            // looking to the right. otherwise
            // if we have a right sib, check it.
            // if more than half full we can use
            // right sib's leftmost entry to steal. if its only half
            // full, mark it as candidate for coalescing.

            if (( whichsib_steal == SIB_NONE ) &&
                ( parent->cur < parent->numkeys ))
            {
                r_sib = parent->nodes[ parent->cur+1 ];
                if ( !r_sib )
                {
                    __DLL3_BTREEERR(SCREWED_UP_RIGHT_SIB);
                }
                if ( r_sib->numkeys > HALF_ORDER )
                    whichsib_steal = SIB_RIGHT;
                else
                    whichsib_coalesce = SIB_RIGHT;
            }

            if (( whichsib_steal == SIB_NONE ) &&
                ( whichsib_coalesce == SIB_NONE ))  // should not happen
            {
                __DLL3_BTREEERR(CANT_STEAL_OR_COALESCE);
            }

            if ( whichsib_steal != SIB_NONE )
            {
                // steal! and disable coalescing
                whichsib_coalesce = SIB_NONE;

                switch ( whichsib_steal )
                {
                case SIB_LEFT:
                    // since we're going to the left, 
                    // we have to adjust parent's index so that it
                    // points to the data item which pivots l from r

                    parent->cur--;

                    // first slide over items in curnod to make room
                    // for item that is stolen
                    for ( i = ORDER_MO; i > 0; i-- )
                    {
                        if ( i != ORDER_MO )
                            n->keys[i] = n->keys[i-1];
                        n->nodes[i] = n->nodes[i-1];
                    }

                    // pull down pivot item from parent
                    n->keys[0] = parent->keys[ (int) parent->cur ];
                    n->numkeys++;

                    // steal item from left sib and put in parent node.
                    i = l_sib->numkeys;
                    parent->keys[ (int) parent->cur ] = l_sib->keys[ i-1 ];

                    // and steal pointer, too
                    n->nodes[ 0 ] = l_sib->nodes[ i ];
                    l_sib->numkeys--;

                    // clear out now-unused slots in sibling
                    l_sib->keys [i-1] = 0;
                    l_sib->nodes[i]   = 0;
                    break;

                case SIB_RIGHT:
                    // pull down pivot item from parent down into curnod
                    i = n->numkeys;
                    n->keys[i] = parent->keys[ (int) parent->cur ];

                    // pull right sib's smallest item up into parent
                    parent->keys[ (int) parent->cur ] = r_sib->keys[0];

                    // grab lowest pointer in right sib for curnod
                    n->nodes[i+1] = r_sib->nodes[0];
                    n->numkeys++;

                    // slide down items in right sib
                    for ( i = 0; i < ORDER_MO; i++ )
                    {
                        if ( i != ORDER_MT )
                            r_sib->keys[i] = r_sib->keys[i+1];
                        r_sib->nodes[i] = r_sib->nodes[i+1];
                    }

                    // clear out unused slot in sib

                    i = r_sib->numkeys-1;
                    r_sib->numkeys = i;
                    r_sib->keys [i]   = 0;
                    r_sib->nodes[i+1] = 0;
                    break;

                case SIB_NONE:
                    // satisfy complaining compilers
                    break;
                }
            }

            if ( whichsib_coalesce != SIB_NONE )
            {
                NODE * l = NULL, * r = NULL;

                switch ( whichsib_coalesce )
                {
                case SIB_LEFT:
                    l = l_sib;
                    r = n;
                    // since we're going to the left, 
                    // we have to adjust parent index so that it
                    // points to the data item which pivots l from r
                    parent->cur--;
                    break;

                case SIB_RIGHT:
                    l = n;
                    r = r_sib;
                    // in this case parent index already points to
                    // the item which pivots l from r
                    break;

                case SIB_NONE:
                    // satisfy complaining compilers
                    break;
                }

                // slide items in r all the way to the right.

                int nr = r->numkeys;
                int nl = l->numkeys;
                int s = ORDER_MO - nr;
                for ( i = nr; i >= 0; i-- )
                {
                    if ( i != nr )
                        r->keys[s+i] = r->keys[i];
                    r->nodes[s+i] = r->nodes[i];
                }

                // suck all items of l into left half of r.

                for ( i = 0; i < nl; i++ )
                {
                    r->keys[i] = l->keys[i];
                    r->nodes[i] = l->nodes[i];
                }

                // there's one more pointer to get.
                r->nodes[i] = l->nodes[i];

                // also get the pivot item from the parent.
                r->keys[i] = parent->keys[ (int) parent->cur ];

                // adjust item count.  this node is full!
                // (a half-full node plus a node which is 1 less than
                // half full, plus one item from parent node, makes a
                // full node.)
                r->numkeys = ORDER_MO;

                // slide parent items around
                for ( i = parent->cur; i < ORDER_MT; i++ )
                {
                    parent->keys [i] = parent->keys [i+1];
                    parent->nodes[i] = parent->nodes[i+1];
                }

                // and one more ptr
                parent->nodes[i] = parent->nodes[i+1];

                // clear out unused positions
                parent->keys [i]   = 0;
                parent->nodes[i+1] = 0;

                // if parent is root and hits zero
                // change root pointer and decrease depth
                if ( --parent->numkeys == 0 )
                {
                    if ( !parent->root )
                    {
                        __DLL3_BTREEERR(NONROOT_NODE_SHRUNK);
                    }
                    numnodes--;
                    depth--;
                    delete_item = parent;
                    r->root = true;
                    store.remove( root );
                    delete root;
                    root = r;
                }

                // delete sib
                switch ( whichsib_coalesce )
                {
                case SIB_LEFT:
                    delete_item = l_sib;
                    break;

                case SIB_RIGHT:
                    delete_item = n;
                    break;

                case SIB_NONE:
                    // satisfy complaining compilers
                    break;
                }

                numnodes--;
            }

            NODE * prev_item = store.get_prev( n );
            if ( delete_item != NULL )
            {
                if ( store.onthislist( delete_item ))
                    store.remove( delete_item );
                delete delete_item;
            }
            n = prev_item;
        }

        empty_store( &store );
        numitems--;
        items.remove(dt);
    }

    int get_cnt() const
    {
        return items.get_cnt();
    }

    void printtree( void ) const
    {
        printf( "TREE: nodes %d, items %d, depth %d\n",
                numnodes, numitems, depth );
        _printtree( root );
    }

    T * get_first( void ) const
    {
        // walk nodes[0] down to leaf, then return keys[0]
        NODE * n = root;
        while ( !n->leaf )
            n = n->nodes[0];
        return n->keys[0];
    }

#if 0

    T * get_next( T * t ) const
    {
        // find t in the tree; if it has a pointer to its right,
        // follow nodes[i] down to leaf and return keys[0].
        // else
        // if it has an item to its right, return that.
        // else
        // find the parent's index. if there's an item to the right
        // of that index, return it.
        // else
        // recurse to its parent, repeat.
        // if we hit the far right of the root, return null.
    }
#endif
};

} // namespace DLL3

#endif /* __DLL3_BTREE_H__ */
