/*
    Copyright (c) 2012 250bpm s.r.o.
    Copyright (c) 2012 Other contributors as noted in the AUTHORS file

    This file is part of Crossroads I/O project.

    Crossroads I/O is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Crossroads is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "testutil.hpp"

int XS_TEST_MAIN ()
{
    fprintf (stderr, "resubscribe test running...\n");

    //  Create the basic infrastructure.
    void *ctx = xs_init ();
    assert (ctx);
    void *xpub = xs_socket (ctx, XS_XPUB);
    assert (xpub);
    void *sub = xs_socket (ctx, XS_SUB);
    assert (sub);

    //  Send two subscriptions upstream.
    int rc = xs_bind (xpub, "tcp://127.0.0.1:5560");
    assert (rc != -1);
    rc = xs_setsockopt (sub, XS_SUBSCRIBE, "a", 1);
    assert (rc == 0);
    rc = xs_setsockopt (sub, XS_SUBSCRIBE, "b", 1);
    assert (rc == 0);
    rc = xs_connect (sub, "tcp://127.0.0.1:5560");
    assert (rc != -1);

    //  Check whether subscriptions are correctly received.
    char buf [5];
    rc = xs_recv (xpub, buf, sizeof (buf), 0);
    assert (rc == 5);
    assert (buf [0] == 0);
    assert (buf [1] == 1);
    assert (buf [2] == 0);
    assert (buf [3] == 1);
    assert (buf [4] == 'a');
    rc = xs_recv (xpub, buf, sizeof (buf), 0);
    assert (rc == 5);
    assert (buf [0] == 0);
    assert (buf [1] == 1);
    assert (buf [2] == 0);
    assert (buf [3] == 1);
    assert (buf [4] == 'b');

    //  Tear down the connection.
    rc = xs_close (xpub);
    assert (rc == 0);
    sleep (1);

    //  Re-establish the connection.
    xpub = xs_socket (ctx, XS_XPUB);
    assert (xpub);
    rc = xs_bind (xpub, "tcp://127.0.0.1:5560");
    assert (rc != -1);

    //  We have to give control to the SUB socket here so that it has
    //  chance to resend the subscriptions.
    rc = xs_recv (sub, buf, sizeof (buf), XS_DONTWAIT);
    assert (rc == -1 && xs_errno () == EAGAIN);

    //  Check whether subscriptions are correctly generated.
    rc = xs_recv (xpub, buf, sizeof (buf), 0);
    assert (rc == 5);
    assert (buf [0] == 0);
    assert (buf [1] == 1);
    assert (buf [2] == 0);
    assert (buf [3] == 1);
    assert (buf [4] == 'a');
    rc = xs_recv (xpub, buf, sizeof (buf), 0);
    assert (rc == 5);
    assert (buf [0] == 0);
    assert (buf [1] == 1);
    assert (buf [2] == 0);
    assert (buf [3] == 1);
    assert (buf [4] == 'b');

    //  Clean up.
    rc = xs_close (sub);
    assert (rc == 0);
    rc = xs_close (xpub);
    assert (rc == 0);
    rc = xs_term (ctx);
    assert (rc == 0);

    return 0 ;
}
