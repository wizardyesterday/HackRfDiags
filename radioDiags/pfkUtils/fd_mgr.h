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

#ifndef __FD_MGR_H__
#define __FD_MGR_H__

#include <stdio.h>
#include "dll3.h"

class fd_mgr;

typedef DLL3::List<class fd_interface,0> fd_interface_list_t;

class fd_interface : public fd_interface_list_t::Links {
protected:
    void make_nonblocking( void );
    int fd;
    bool do_close;
public:
    fd_interface( void ) { do_close = false; }
    virtual ~fd_interface( void ) {
        // placeholder : note that fd is NOT closed here, because
        //  perhaps the derived class wants to do something with it.
        //  if not, all derived class destructors must close( fd ).
    }

    enum rw_response {
        OK,   // the read or write was normal
        DEL   // the fd should be deleted
    };

    // if a fd doesn't need read or write, don't implement
    // them in the class; just allow the defaults here to be used.

    virtual rw_response read ( fd_mgr * ) { return DEL; }
    virtual rw_response write( fd_mgr * ) { return DEL; }

    // true means select, false means dont;
    //   note:   never return true if you've set do_close !!
    virtual void select_rw ( fd_mgr *, bool * do_read, bool * do_write ) = 0;

    // provided for convenience; if these are not used,
    // just use the defaults here.
    virtual bool write_to_fd( char * buf, int len ) { return false; }
    virtual bool over_write_threshold( void ) { return false; }

    friend class fd_mgr;
};

class fd_mgr {
    fd_interface_list_t  ifds;
    bool debug;
    int  die_threshold;
public:
    fd_mgr( bool _debug, int _die_threshold = 0 ) {
        debug = _debug; die_threshold = _die_threshold;
    }
    void register_fd( fd_interface * ifd ) {
        if ( debug )
            fprintf( stderr, "registering fd %d\n", ifd->fd );
        WaitUtil::Lock lck(&ifds);
        ifds.add_tail( ifd );
    }
    void unregister_fd( fd_interface * ifd ) {
        if ( debug )
            fprintf( stderr, "unregistering fd %d\n", ifd->fd );
        WaitUtil::Lock lck(&ifds);
        ifds.remove( ifd );
    }
    // return false if timeout, true if out of fds to service
    bool loop( struct timeval * tv );
    // don't return until out of fds
    void loop( void ) { while (!loop( NULL )) ; }
};

#endif /* __FD_MGR_H__ */
