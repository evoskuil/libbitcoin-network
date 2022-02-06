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
#include <bitcoin/network/protocols/protocol_ping_31402.hpp>

#include <functional>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_ping_31402
static const std::string protocol_name = "ping";

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

protocol_ping_31402::protocol_ping_31402(const session& session,
    channel::ptr channel, const duration& heartbeat)
  : protocol_timer(session, channel, heartbeat)
{
}

void protocol_ping_31402::start()
{
    BC_ASSERT_MSG(stranded(), "stranded");

    // Timer/event start completes without invoking the handler.
    protocol_timer::start(BIND1(send_ping, _1));

    SUBSCRIBE2(ping, handle_receive_ping, _1, _2);

    // Send initial ping message by simulating first heartbeat.
    set_event(error::success);
}

// This is fired by the callback (i.e. base timer and stop handler).
void protocol_ping_31402::send_ping(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    if (ec && ec != error::channel_timeout)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure in ping timer for [" << authority() << "] "
            << ec.message();
        stop(ec);
        return;
    }

    SEND2(ping{}, handle_send, _1, ping::command);
}

void protocol_ping_31402::handle_receive_ping(const code& ec,
    ping::ptr)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    // TODO: log.
    ////if (stopped(ec))
    ////    return;
}

const std::string& protocol_ping_31402::name() const
{
    return protocol_name;
}

} // namespace network
} // namespace libbitcoin
