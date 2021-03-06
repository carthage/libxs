/*
    Copyright (c) 2009-2012 250bpm s.r.o.
    Copyright (c) 2007-2009 iMatix Corporation
    Copyright (c) 2007-2011 Other contributors as noted in the AUTHORS file

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

#ifndef __XS_CTX_HPP_INCLUDED__
#define __XS_CTX_HPP_INCLUDED__

#include <map>
#include <vector>
#include <string>

#include "../include/xs.h"

#include "mailbox.hpp"
#include "array.hpp"
#include "config.hpp"
#include "mutex.hpp"
#include "options.hpp"
#include "atomic_counter.hpp"

namespace xs
{

    class object_t;
    class io_thread_t;
    class socket_base_t;
    class reaper_t;

    //  Information associated with inproc endpoint. Note that endpoint options
    //  are registered as well so that the peer can access them without a need
    //  for synchronisation, handshaking or similar.
    struct endpoint_t
    {
        class socket_base_t *socket;
        options_t options;
    };

    //  Context object encapsulates all the global state associated with
    //  the library.

    class ctx_t
    {
    public:

        //  Create the context object.
        ctx_t ();

        //  Returns false if object is not a context.
        bool check_tag ();

        //  This function is called when user invokes xs_term. If there are
        //  no more sockets open it'll cause all the infrastructure to be shut
        //  down. If there are open sockets still, the deallocation happens
        //  after the last one is closed.
        int terminate ();

        //  Set context option.
        int setctxopt (int option_, const void *optval_, size_t optvallen_);

        //  Create and destroy a socket.
        xs::socket_base_t *create_socket (int type_);
        void destroy_socket (xs::socket_base_t *socket_);

        //  Send command to the destination thread.
        void send_command (uint32_t tid_, const command_t &command_);

        //  Returns the I/O thread that is the least busy at the moment.
        //  Affinity specifies which I/O threads are eligible (0 = all).
        //  Returns NULL is no I/O thread is available.
        xs::io_thread_t *choose_io_thread (uint64_t affinity_);

        //  Returns reaper thread object.
        xs::object_t *get_reaper ();

        //  Get the filter associated with the specified filter ID or NULL
        //  If such filter is not registered.
        xs_filter_t *get_filter (int filter_id_);

        //  Management of inproc endpoints.
        int register_endpoint (const char *addr_, endpoint_t &endpoint_);
        void unregister_endpoints (xs::socket_base_t *socket_);
        endpoint_t find_endpoint (const char *addr_);

        enum {
            term_tid = 0,
            reaper_tid = 1
        };

        ~ctx_t ();

    private:

        //  Plug in the extension specified.
        int plug (const void *ext_);

        //  Used to check whether the object is a context.
        uint32_t tag;

        //  Sockets belonging to this context. We need the list so that
        //  we can notify the sockets when xs_term() is called. The sockets
        //  will return ETERM then.
        typedef array_t <socket_base_t> sockets_t;
        sockets_t sockets;

        //  List of unused thread slots.
        typedef std::vector <uint32_t> emtpy_slots_t;
        emtpy_slots_t empty_slots;

        //  If true, xs_init has been called but no socket have been created
        //  yes. Launching of I/O threads is delayed.
        bool starting;

        //  If true, xs_term was already called.
        bool terminating;

        //  Synchronisation of accesses to global slot-related data:
        //  sockets, empty_slots, terminating. It also synchronises
        //  access to zombie sockets as such (as oposed to slots) and provides
        //  a memory barrier to ensure that all CPU cores see the same data.
        mutex_t slot_sync;

        //  The reaper thread.
        xs::reaper_t *reaper;

        //  I/O threads.
        typedef std::vector <xs::io_thread_t*> io_threads_t;
        io_threads_t io_threads;

        //  Array of pointers to mailboxes for both application and I/O threads.
        uint32_t slot_count;
        mailbox_t **slots;

        //  Mailbox for xs_term thread.
        mailbox_t term_mailbox;

        //  List of inproc endpoints within this context.
        typedef std::map <std::string, endpoint_t> endpoints_t;
        endpoints_t endpoints;

        //  Synchronisation of access to the list of inproc endpoints.
        mutex_t endpoints_sync;

        //  Maximum socket ID.
        static atomic_counter_t max_socket_id;

        //  Maximum number of sockets that can be opened at the same time.
        int max_sockets;

        //  Number of I/O threads to launch.
        int io_thread_count;

        //  Synchronisation of access to context options.
        mutex_t opt_sync;

        //  List of all dynamically loaded extension libraries.
#if defined XS_HAVE_WINDOWS
        typedef std::vector <HMODULE> plugins_t;
        plugins_t plugins;
#elif defined XS_HAVE_PLUGINS
        typedef std::vector <void*> plugins_t;
        plugins_t plugins;
#endif

        //  List of all filters plugged into the context.
        typedef std::map <int, xs_filter_t*> filters_t;
        filters_t filters;

        ctx_t (const ctx_t&);
        const ctx_t &operator = (const ctx_t&);
    };

}

#endif
