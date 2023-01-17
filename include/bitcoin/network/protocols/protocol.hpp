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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_HPP

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>

namespace libbitcoin {
namespace network {

#define PROTOCOL_ARGS(handler, args) \
    std::forward<Handler>(handler), \
    shared_from_base<Protocol>(), \
    std::forward<Args>(args)...
#define BOUND_PROTOCOL(handler, args) \
    std::bind(PROTOCOL_ARGS(handler, args))

#define PROTOCOL_ARGS_TYPE(handler, args) \
    std::forward<Handler>(handler), \
    std::shared_ptr<Protocol>(), \
    std::forward<Args>(args)...
#define BOUND_PROTOCOL_TYPE(handler, args) \
    std::bind(PROTOCOL_ARGS_TYPE(handler, args))

class session;

/// This class is thread safe, except for:
/// * start/started must be called on strand.
/// * setters should only be invoked during handshake.
/// Abstract base class for protocols.
/// handle_ methods are always invoked on the strand.
class BCT_API protocol
  : public enable_shared_from_base<protocol>
{
public:
    DELETE_COPY_MOVE(protocol);

    /// The channel is stopping (called on strand by stop subscription).
    /// This must be called only from the channel strand (not thread safe).
    virtual void stopping(const code& ec) NOEXCEPT;

protected:
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, const messages::address_items&)>
        fetches_handler;

    /// Construct an instance.
    protocol(const session& session, const channel::ptr& channel) NOEXCEPT;

    /// Asserts that protocol is stopped.
    virtual ~protocol() NOEXCEPT;

    /// Macro helpers (use macros).
    /// -----------------------------------------------------------------------

    /// Bind a method in the base or derived class (use BIND#).
    template <class Protocol, typename Handler, typename... Args>
    auto bind(Handler&& handler, Args&&... args) NOEXCEPT ->
        decltype(BOUND_PROTOCOL_TYPE(handler, args)) const
    {
        return BOUND_PROTOCOL(handler, args);
    }

    /// Send a message instance to peer (use SEND#).
    template <class Protocol, class Message, typename Handler, typename... Args>
    void send(Message&& message, Handler&& handler, Args&&... args) NOEXCEPT
    {
        channel_->send<Message>(system::to_shared(std::forward<Message>(message)),
            BOUND_PROTOCOL(handler, args));
    }

    /// Subscribe to channel messages by type (use SUBSCRIBE#).
    /// Handler is invoked with error::subscriber_stopped if already stopped.
    template <class Protocol, class Message, typename Handler, typename... Args>
    void subscribe(Handler&& handler, Args&&... args) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        channel_->subscribe<Message>(BOUND_PROTOCOL(handler, args));
    }

    /// Start/Stop.
    /// -----------------------------------------------------------------------

    /// Set protocol started state (strand required).
    virtual void start() NOEXCEPT;

    /// Get protocol started state (strand required).
    virtual bool started() const NOEXCEPT;

    /// Channel is stopped or code set.
    virtual bool stopped(const code& ec=error::success) const NOEXCEPT;

    /// Stop the channel.
    virtual void stop(const code& ec) NOEXCEPT;

    /// Pause the channel (strand required).
    virtual void pause() NOEXCEPT;

    /////// Resume the channel (strand required).
    ////virtual void resume() NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The current thread is on the channel strand.
    virtual bool stranded() const NOEXCEPT;

    /// Declare protocol canonical name.
    virtual const std::string& name() const NOEXCEPT = 0;

    /// The authority of the peer.
    virtual config::authority authority() const NOEXCEPT;

    /// The nonce of the channel.
    virtual uint64_t nonce() const NOEXCEPT;

    /// The protocol version of the peer.
    virtual messages::version::ptr peer_version() const NOEXCEPT;

    /// Set protocol version of the peer (set only during handshake).
    virtual void set_peer_version(const messages::version::ptr& value) NOEXCEPT;

    /// The negotiated protocol version.
    virtual uint32_t negotiated_version() const NOEXCEPT;

    /// Set negotiated protocol version (set only during handshake).
    virtual void set_negotiated_version(uint32_t value) NOEXCEPT;

    /// Network settings.
    const network::settings& settings() const NOEXCEPT;

    /// Return a logging instance.
    const logger& log() const NOEXCEPT;

    /// Addresses.
    /// -----------------------------------------------------------------------

    /// Fetch a set of peer addresses from the address pool.
    virtual void fetches(fetches_handler&& handler) NOEXCEPT;

    /// Save a set of peer addresses to the address pool.
    virtual void saves(const messages::address_items& addresses) NOEXCEPT;
    virtual void saves(const messages::address_items& addresses,
        result_handler&& handler) NOEXCEPT;

    // Capture send results, logged by default.
    virtual void handle_send(const code& ec) NOEXCEPT;

private:
    void do_fetches(const code& ec,
        const messages::address_items& addresses,
        const fetches_handler& handler) NOEXCEPT;
    void handle_fetches(const code& ec, const messages::address_items& addresses,
        const fetches_handler& handler) NOEXCEPT;

    void do_saves(const code& ec, const result_handler& handler) NOEXCEPT;
    void handle_saves(const code& ec, const result_handler& handler) NOEXCEPT;

    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    channel::ptr channel_;

    // This is thread safe.
    const session& session_;

    // This is protected by strand.
    bool started_;
};

#undef PROTOCOL_ARGS
#undef BOUND_PROTOCOL
#undef PROTOCOL_ARGS_TYPE
#undef BOUND_PROTOCOL_TYPE

// See define.hpp for BIND# macros.

#define SEND1(message, method, p1) \
    send<CLASS>(message, &CLASS::method, p1)
#define SEND2(message, method, p1, p2) \
    send<CLASS>(message, &CLASS::method, p1, p2)
#define SEND3(message, method, p1, p2, p3) \
    send<CLASS>(message, &CLASS::method, p1, p2, p3)

#define SUBSCRIBE2(message, method, p1, p2) \
    subscribe<CLASS, message>(&CLASS::method, p1, p2)
#define SUBSCRIBE3(message, method, p1, p2, p3) \
    subscribe<CLASS, message>(&CLASS::method, p1, p2, p3)

} // namespace network
} // namespace libbitcoin

#endif
