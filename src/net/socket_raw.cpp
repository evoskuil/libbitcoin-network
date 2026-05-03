/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/net/socket.hpp>

#include <utility>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// RAW (read).
// ----------------------------------------------------------------------------

void socket::raw_read(http::flat_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_raw_read,
            shared_from_this(), std::ref(out), std::move(handler)));
}

// private
void socket::do_raw_read(ref<http::flat_buffer> out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    async_read(out.get(), handler);
}

// RAW (write).
// ----------------------------------------------------------------------------

void socket::raw_write(const asio::const_buffer& in, bool binary,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_raw_write,
            shared_from_this(), in, binary, std::move(handler)));
}

// private
void socket::do_raw_write(const asio::const_buffer& in, bool binary,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    async_write(in, binary, handler);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
