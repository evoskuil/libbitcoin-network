/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/socket.hpp>

#include <boost/asio.hpp>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace network {

#define NAME "socket"

socket::socket(threadpool& pool)
  : socket_(pool.service()),
    CONSTRUCT_TRACK(socket)
{
}

config::authority socket::get_authority() const
{
    boost_code ec;
    const auto endpoint = socket_.remote_endpoint(ec);
    return ec ? config::authority() : config::authority(endpoint);
}

// TODO: replace this with the return of a class that unlocks on destruct.
asio::socket& socket::get_locked_socket()
{
    mutex_.lock();
    return socket_;
}

// TODO: replace this with the return of a class that unlocks on destruct.
void socket::unlock_socket()
{
    mutex_.unlock();
}

// BUGBUG: socket::cancel fails with error::operation_not_supported
// on Windows XP and Windows Server 2003, but handler invocation is required.
// We should enable BOOST_ASIO_ENABLE_CANCELIO and BOOST_ASIO_DISABLE_IOCP
// on these platforms only. See: bit.ly/1YC0p9e
void socket::close()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (socket_.is_open())
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        mutex_.unlock_upgrade_and_lock();

        // Ignoring socket error codes creates exception safety.
        boost_code ignore;
        socket_.shutdown(asio::socket::shutdown_both, ignore);
        socket_.cancel(ignore);

        mutex_.unlock();
        //---------------------------------------------------------------------
        return;
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
}

} // namespace network
} // namespace libbitcoin
