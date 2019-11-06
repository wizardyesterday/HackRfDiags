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

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

#include "fd_mgr.h"

void
fd_interface :: make_nonblocking( void )
{
    fcntl( fd, F_SETFL,
           fcntl( fd, F_GETFL, 0 ) | O_NONBLOCK );
}

bool
fd_mgr :: loop( struct timeval *tv )
{
    fd_interface * fdi, * nfdi;

    while ( ifds.get_cnt() > die_threshold )
    {
        fd_set rfds, wfds;
        int max, cc;

        FD_ZERO( &rfds );
        FD_ZERO( &wfds );

        max = -1;

        {
            WaitUtil::Lock  lck(&ifds);
            for ( fdi = ifds.get_head(); fdi; fdi = nfdi )
            {
                nfdi = ifds.get_next(fdi);
                bool dosrd, doswr;
                fdi->select_rw( this, &dosrd, &doswr );
                if ( dosrd )
                {
                    if ( debug )
                        fprintf( stderr,
                                 "selecting for read on fd %d\n", fdi->fd );
                    FD_SET( fdi->fd, &rfds );
                    if ( fdi->fd > max )
                        max = fdi->fd;
                }
                if ( doswr )
                {
                    if ( debug )
                        fprintf( stderr, "selecting for write on fd %d\n",
                                 fdi->fd );
                    FD_SET( fdi->fd, &wfds );
                    if ( fdi->fd > max )
                        max = fdi->fd;
                }
                if ( fdi->do_close )
                {
                    if ( debug )
                        fprintf( stderr,
                                 "deleting fd %d due to do_close flag\n",
                                 fdi->fd );
                    ifds.remove( fdi );
                    delete fdi;
                }
            }
        } // unlock lck

        if ( ifds.get_cnt() <= die_threshold )
            break;

        if ( max == -1 )
        {
            fprintf( stderr, "max == -1?!\n" );
            break;
        }

        cc = select( max+1, &rfds, &wfds, NULL, tv );

        if ( tv != NULL && cc == 0 )
            return false;

        if ( cc <= 0 )
        {
            if ( debug )
                fprintf( stderr, "select returns %d?! (%d:%s)\n",
                         cc, errno, strerror( errno ));
            continue;
        }

        {
            WaitUtil::Lock lck(&ifds);
            for ( fdi = ifds.get_head(); fdi; fdi = nfdi )
            {
                bool del = false;
                nfdi = ifds.get_next(fdi);

                if ( FD_ISSET( fdi->fd, &rfds ))
                {
                    if ( debug )
                        fprintf( stderr, "servicing read on fd %d\n",
                                 fdi->fd );
                    lck.unlock(); // handler may add a new conn
                    if ( fdi->read(this) == fd_interface::DEL )
                    {
                        if ( debug )
                            fprintf( stderr,
                                     "deleting %d due to false read\n",
                                     fdi->fd );
                        del = true;
                    }
                    lck.lock();
                }
                if ( FD_ISSET( fdi->fd, &wfds ))
                {
                    if ( debug )
                        fprintf( stderr, "servicing write on fd %d\n",
                                 fdi->fd );
                    lck.unlock(); // handler may add a new conn
                    if ( fdi->write(this) == fd_interface::DEL )
                    {
                        if ( debug )
                            fprintf( stderr,
                                     "deleting %d due to false write\n",
                                     fdi->fd );
                        del = true;
                    }
                    lck.lock();
                }
                if ( !del && fdi->do_close )
                {
                    del = true;
                    if ( debug )
                        fprintf( stderr,
                                 "deleting %d due to do_close\n", fdi->fd );
                }
                if ( del )
                {
                    ifds.remove( fdi );
                    delete fdi;
                }
            }
        } // unlock lck
    }
    {
        WaitUtil::Lock lck(&ifds);
        for ( fdi = ifds.get_head(); fdi; fdi = nfdi )
        {
            nfdi = ifds.get_next(fdi);
            ifds.remove( fdi );
            delete fdi;
        }
    } // unlock lck
    return true;
}
