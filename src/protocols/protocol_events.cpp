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
#include <bitcoin/network/protocols/protocol_events.hpp>

#include <functional>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/network_interface.hpp>
#include <bitcoin/network/protocols/protocol.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_events

using namespace std::placeholders;

protocol_events::protocol_events(network_interface& network, channel::ptr channel,
    const std::string& name)
  : protocol(network, channel, name)
{
}

// Properties.
// ----------------------------------------------------------------------------

bool protocol_events::stopped() const
{
    return !handler_.load();
}

// Start.
// ----------------------------------------------------------------------------

void protocol_events::start(event_handler handler)
{
    handler_.store(handler);
    SUBSCRIBE_STOP1(handle_stopped, _1);
}

// Stop.
// ----------------------------------------------------------------------------

void protocol_events::handle_stopped(const code& ec)
{
    log::debug(LOG_NETWORK)
        << "Stop protocol_" << name() << " on [" << authority() << "] "
        << ec.message();
    
    // Event handlers can depend on this code for channel stop.
    set_event(error::channel_stopped);
}

// Set Event.
// ----------------------------------------------------------------------------

void protocol_events::set_event(const code& ec)
{
    auto handler = handler_.load();
    if (!handler)
        return;

    if (ec == error::channel_stopped)
        handler_.store(nullptr);

    handler(ec);
}

// Send Handler.
// ----------------------------------------------------------------------------

void protocol_events::handle_send(const code& ec, const std::string& command)
{
    if (stopped())
        return;

    if (ec)
    {
        log::debug(LOG_NETWORK)
            << "Failure sending '" << command << "' to [" << authority()
            << "] " << ec.message();
        stop(ec);
        return;
    }
}

} // namespace network
} // namespace libbitcoin
