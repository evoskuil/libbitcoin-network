/**
 * Copyright (c) 2011-2016 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_P2P_INTERFACE_HPP
#define LIBBITCOIN_NETWORK_P2P_INTERFACE_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// A network services interface.
class BCT_API network_interface
{
public:
    typedef message::network_address address;
    typedef std::function<void(bool)> truth_handler;
    typedef std::function<void(size_t)> count_handler;
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, const address&)> address_handler;
    typedef std::function<void(const code&, channel::ptr)> channel_handler;
    typedef std::function<bool(const code&, channel::ptr)> connect_handler;

    // Properties.
    // ------------------------------------------------------------------------

    /// Network configuration settings.
    virtual const settings& network_settings() const = 0;

    /// Return the current top block height.
    virtual size_t top_height() const = 0;

    /// Set the current top block height, for use in version messages.
    virtual void set_top_height(size_t value) = 0;

    /// Determine if the network is stopped.
    virtual bool stopped() const = 0;

    /// Return a reference to the network threadpool.
    virtual threadpool& thread_pool() = 0;

    // Subscriptions.
    // ------------------------------------------------------------------------

    /// Subscribe to connection creation events.
    virtual void subscribe_connection(connect_handler handler) = 0;

    /// Subscribe to service stop event.
    virtual void subscribe_stop(result_handler handler) = 0;

    // Manual connections.
    // ----------------------------------------------------------------------------

    /// Maintain a connection to hostname:port.
    virtual void connect(const config::endpoint& peer) = 0;

    /// Maintain a connection to hostname:port.
    virtual void connect(const std::string& hostname, uint16_t port) = 0;

    /// Maintain a connection to hostname:port.
    /// The callback is invoked by the first connection creation only.
    virtual void connect(const std::string& hostname, uint16_t port,
        channel_handler handler) = 0;

    // Pending connections collection.
    // ------------------------------------------------------------------------

    /// Store a pending connection reference.
    virtual void pend(channel::ptr channel, result_handler handler) = 0;

    /// Free a pending connection reference.
    virtual void unpend(channel::ptr channel, result_handler handler) = 0;

    /// Test for a pending connection reference.
    virtual void pending(uint64_t version_nonce,
        truth_handler handler) const = 0;

    // Connections collection.
    // ------------------------------------------------------------------------

    /// Determine if there exists a connection to the address.
    virtual void connected(const address& address,
        truth_handler handler) const = 0;

    /// Store a connection.
    virtual void store(channel::ptr channel, result_handler handler) = 0;

    /// Remove a connection.
    virtual void remove(channel::ptr channel, result_handler handler) = 0;

    /// Get the number of connections.
    virtual void connected_count(count_handler handler) const = 0;

    // Hosts collection.
    // ------------------------------------------------------------------------

    /// Get a randomly-selected address.
    virtual void fetch_address(address_handler handler) const = 0;

    /// Store an address.
    virtual void store(const address& address, result_handler handler) = 0;

    /// Store a collection of addresses.
    virtual void store(const address::list& addresses,
        result_handler handler) = 0;

    /// Remove an address.
    virtual void remove(const address& address, result_handler handler) = 0;

    /// Get the number of addresses.
    virtual void address_count(count_handler handler) const = 0;
};

} // namespace network
} // namespace libbitcoin

#endif
