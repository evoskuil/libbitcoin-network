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
#ifndef LIBBITCOIN_NETWORK_P2P_NETWORK_HPP
#define LIBBITCOIN_NETWORK_P2P_NETWORK_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/connections.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/hosts.hpp>
#include <bitcoin/network/message_subscriber.hpp>
#include <bitcoin/network/pending_channels.hpp>
#include <bitcoin/network/network_interface.hpp>
#include <bitcoin/network/sessions/session_inbound.hpp>
#include <bitcoin/network/sessions/session_manual.hpp>
#include <bitcoin/network/sessions/session_outbound.hpp>
#include <bitcoin/network/sessions/session_seed.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Top level public networking interface, partly thread safe.
class BCT_API p2p_network
  : public network_interface, enable_shared_from_base<p2p_network>
{
public:
    typedef std::shared_ptr<p2p_network> ptr;
    typedef subscriber<const code&> stop_subscriber;
    typedef resubscriber<const code&, channel::ptr> channel_subscriber;

    // Templates (send/receive).
    // ------------------------------------------------------------------------

    /// Send message to all connections.
    template <typename Message>
    void broadcast(const typename Message& message,
        channel_handler handle_channel, result_handler handle_complete)
    {
        connections_->broadcast(message, handle_channel, handle_complete);
    }

    /// Subscribe to all incoming messages of a type.
    template <class Message>
    void subscribe(message_handler<Message>&& handler)
    {
        connections_->subscribe(
            std::forward<message_handler<Message>>(handler));
    }

    // Constructors.
    // ------------------------------------------------------------------------

    /// Construct an instance.
    p2p_network(const settings& settings);

    /// This class is not copyable.
    p2p_network(const p2p_network&) = delete;
    void operator=(const p2p_network&) = delete;

    /// Ensure all threads are coalesced.
    virtual ~p2p_network();

    // Start/Run sequences.
    // ------------------------------------------------------------------------

    /// Invoke startup and seeding sequence, call from constructing thread.
    virtual void start(result_handler handler);

    /// Synchronize the blockchain and then begin long running sessions,
    /// call from start result handler. Call base method to skip sync.
    virtual void run(result_handler handler);

    // Shutdown.
    // ------------------------------------------------------------------------

    /// Idempotent call to signal work stop, start may be reinvoked after.
    /// Returns the result of file save operation.
    virtual bool stop();

    /// Blocking call to coalesce all work and then terminate all threads.
    /// Call from thread that constructed this class, or don't call at all.
    /// This calls stop, and start may be reinvoked after calling this.
    virtual bool close();

    // Properties.
    // ------------------------------------------------------------------------

    /// Network configuration settings.
    virtual const settings& network_settings() const;

    /// Return the current top block height.
    virtual size_t top_height() const;

    /// Set the current top block height, for use in version messages.
    virtual void set_top_height(size_t value);

    /// Determine if the network is stopped.
    virtual bool stopped() const;

    /// Return a reference to the network threadpool.
    virtual threadpool& thread_pool();

    // Subscriptions.
    // ------------------------------------------------------------------------

    /// Subscribe to connection creation events.
    virtual void subscribe_connection(connect_handler handler);

    /// Subscribe to service stop event.
    virtual void subscribe_stop(result_handler handler);

    // Manual connections.
    // ----------------------------------------------------------------------------

    /// Maintain a connection to hostname:port.
    virtual void connect(const config::endpoint& peer);

    /// Maintain a connection to hostname:port.
    virtual void connect(const std::string& hostname, uint16_t port);

    /// Maintain a connection to hostname:port.
    /// The callback is invoked by the first connection creation only.
    virtual void connect(const std::string& hostname, uint16_t port,
        channel_handler handler);

    // Pending connections collection.
    // ------------------------------------------------------------------------

    /// Store a pending connection reference.
    virtual void pend(channel::ptr channel, result_handler handler);

    /// Free a pending connection reference.
    virtual void unpend(channel::ptr channel, result_handler handler);

    /// Test for a pending connection reference.
    virtual void pending(uint64_t version_nonce, truth_handler handler) const;

    // Connections collection.
    // ------------------------------------------------------------------------

    /// Determine if there exists a connection to the address.
    virtual void connected(const address& address,
        truth_handler handler) const;

    /// Store a connection.
    virtual void store(channel::ptr channel, result_handler handler);

    /// Remove a connection.
    virtual void remove(channel::ptr channel, result_handler handler);

    /// Get the number of connections.
    virtual void connected_count(count_handler handler) const;

    // Hosts collection.
    // ------------------------------------------------------------------------

    /// Get a randomly-selected address.
    virtual void fetch_address(address_handler handler) const;

    /// Store an address.
    virtual void store(const address& address, result_handler handler);

    /// Store a collection of addresses.
    virtual void store(const address::list& addresses, result_handler handler);

    /// Remove an address.
    virtual void remove(const address& address, result_handler handler);

    /// Get the number of addresses.
    virtual void address_count(count_handler handler) const;

protected:

    /// Attach a session to the network, caller must start the session.
    template <class Session, typename... Args>
    typename Session::ptr attach(Args&&... args)
    {
        return std::make_shared<Session>(*this, std::forward<Args>(args)...);
    }

    /// Override to attach specialized sessions.
    virtual session_seed::ptr attach_seed_session();
    virtual session_manual::ptr attach_manual_session();
    virtual session_inbound::ptr attach_inbound_session();
    virtual session_outbound::ptr attach_outbound_session();

private:
    void handle_manual_started(const code& ec, result_handler handler);
    void handle_inbound_started(const code& ec, result_handler handler);
    void handle_hosts_loaded(const code& ec, result_handler handler);
    void handle_hosts_saved(const code& ec, result_handler handler);
    void handle_new_connection(const code& ec, channel::ptr channel,
        result_handler handler);

    void handle_started(const code& ec, result_handler handler);
    void handle_running(const code& ec, result_handler handler);

    const settings& settings_;

    // These are thread safe.
    std::atomic<bool> stopped_;
    std::atomic<size_t> top_height_;
    bc::atomic<session_manual::ptr> manual_;
    threadpool threadpool_;
    hosts::ptr hosts_;
    pending_channels pending_;
    connections::ptr connections_;
    stop_subscriber::ptr stop_subscriber_;
    channel_subscriber::ptr channel_subscriber_;
};

} // namespace network
} // namespace libbitcoin

#endif
