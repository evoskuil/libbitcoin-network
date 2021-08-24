/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NETWORK_CONCURRENT_SOCKET_HPP
#define LIBBITCOIN_NETWORK_CONCURRENT_SOCKET_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/concurrent/asio.hpp>
#include <bitcoin/network/concurrent/thread.hpp>
#include <bitcoin/network/concurrent/threadpool.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// This class is thread safe but the socket may not be used concurrently.
class BCT_API socket
  : system::noncopyable
    /*, public track<socket>*/
{
public:
    typedef std::shared_ptr<socket> ptr;

    /// Construct an instance.
    socket(threadpool& thread);

    /// Obtain the authority of the remote endpoint.
    system::config::authority authority() const;

    /// The underlying socket.
    asio::socket& get();

    /// Signal cancel of all outstanding work on the socket.
    virtual void stop();

private:
    // This is thread safe.
    threadpool& thread_;

    // This is protected by mutex.
    asio::socket socket_;
    mutable shared_mutex mutex_;
};

} // namespace network
} // namespace libbitcoin

#endif
