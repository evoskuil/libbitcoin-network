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
#include <bitcoin/network/channels/channel_ws.hpp>

#include <optional>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

#define CLASS channel_ws

using namespace system;
using namespace network::http;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// public/virtual
// ----------------------------------------------------------------------------

// Failure to call after successful message handling causes stalled channel.
void channel_ws::receive() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped() || paused() || !upgraded_)
    {
        channel_http::receive();
        return;
    }

    read(request_buffer(),
        std::bind(&channel_ws::handle_upgraded,
            shared_from_base<channel_ws>(), _1, _2));
}

void channel_ws::handle_upgraded(const code& ec, size_t bytes) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        LOGQ("Websocket read abort [" << endpoint() << "]");
        return;
    }

    if (ec)
    {
        // Don't log common conditions.
        if (ec != error::peer_disconnect && ec != error::operation_canceled)
        {
            LOGF("Websocket read failure [" << endpoint() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    if (request_buffer().size() != bytes)
    {
        stop(error::size_mismatch);
        return;
    }

    // In http the request buffer is a running cache, however after the socket
    // is upgraded to a websocket, the buffer provides the request payload. As
    // such the listener may not be restarted until payload handling complete.
    dispatch(request_buffer());
}

void channel_ws::dispatch(const flat_buffer& buffer) NOEXCEPT
{
    const auto size = buffer.size();
    LOGA("Websocket read of " << size << " bytes [" << endpoint() << "]");

    // Create a synthetic message to leverage http dispatch (unknown->ws).
    const auto message = to_shared<request>();

    // Span body requires non-const data, which is safe here.
    BC_PUSH_WARNING(NO_CONST_CAST)
    const auto data = const_cast<void*>(buffer.data().data());
    BC_POP_WARNING()

    message->body() = span_value{ pointer_cast<uint8_t>(data), size };
    message->method(verb::unknown);
    channel_http::dispatch(message);
}

// pre-upgrade
// ----------------------------------------------------------------------------

void channel_ws::handle_receive(const code& ec, size_t bytes,
    const http::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (upgraded_)
    {
        LOGF("Http request in websocket state [" << endpoint() << "]");
        stop(network::error::operation_failed);
        return;
    }

    if (ec != error::upgraded)
    {
        channel_http::handle_receive(ec, bytes, request);
        return;
    }

    upgraded_ = true;
    LOGA("Websocket upgraded [" << endpoint() << "]");
    receive();
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
