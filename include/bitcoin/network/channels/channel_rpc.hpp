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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_RPC_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_RPC_HPP

#include <memory>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>

// TODO: This template can be re-based on channel_http giving it the ability to
// TODO: support RPC over both tcp and http/ws. In that case the settings type
// TODO: can be templatized and reader/writer configured at compile. Without an
// TODO: http reader the socket cannot be upgraded to ws and thus operates as a
// TODO: simple TCP socket, and TLS remains possible with all protocols. Http
// TODO: methods are dispatched from channel to base protocol and re-dispatched
// TODO: to subscribers. TCP/WS are dispatched to protocol as a custom method.
// TODO: this allows the base protocol to differentiate these as non-http for
// TODO: selection of the proper response path. As http is half duplex, notify
// TODO: is only provided to a TCP or upgraded (WS) socket.

namespace libbitcoin {
namespace network {

/// Read rpc-request and send rpc-response, dispatch to Interface.
template <typename Interface>
class channel_rpc
  : public channel
{
public:
    typedef std::shared_ptr<channel_rpc> ptr;
    using options_t = network::settings::tls_server;
    using dispatcher = rpc::dispatcher<Interface>;

    /// Subscribe to request from client (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Unused, class Handler>
    inline void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());
        dispatcher_.subscribe(std::forward<Handler>(handler));
    }

    /// Construct rpc channel to encapsulate and communicate on the socket.
    inline channel_rpc(const logger& log, const socket::ptr& socket,
        uint64_t identifier, const settings_t& settings,
        const options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options),
        request_buffer_(options.minimum_buffer)
    {
    }

    /// Senders (requires strand).
    inline void send_code(const code& ec, result_handler&& handler) NOEXCEPT;
    inline void send_error(rpc::result_t&& error,
        result_handler&& handler) NOEXCEPT;
    inline void send_result(rpc::value_t&& result, size_t size_hint,
        result_handler&& handler) NOEXCEPT;
    inline void send_notification(rpc::string_t&& method,
        rpc::params_t&& notification, size_t size_hint,
        result_handler&& handler) NOEXCEPT;

    /// Resume reading from the socket (requires strand).
    inline void resume() NOEXCEPT override;

protected:
    /// Serialize and write response to client (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <typename Message>
    inline void send(Message&& message, size_t size_hint,
        result_handler&& handler) NOEXCEPT;

    /// Handle send completion, invokes receive() for non-notifications.
    template <typename Message>
    inline void handle_send(const code& ec, size_t bytes,
        const std::string& method, const result_handler& handler) NOEXCEPT;

    /// Stranded handler invoked from stop().
    inline void stopping(const code& ec) NOEXCEPT override;

    /// Read request buffer (requires strand).
    virtual inline http::flat_buffer& request_buffer() NOEXCEPT;

    /// Override to dispatch request to subscribers by requested method.
    virtual inline void dispatch(const rpc::request_cptr& request) NOEXCEPT;

    /// Must call after successful message handling if no stop.
    virtual inline void receive() NOEXCEPT;

    /// Handle incoming messages.
    virtual inline void handle_receive(const code& ec, size_t bytes,
        const rpc::request_cptr& request) NOEXCEPT;

private:
    // These are protected by strand.
    rpc::version version_;
    rpc::id_option identity_;
    http::flat_buffer request_buffer_;
    dispatcher dispatcher_{};
    bool reading_{};
};

} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <typename Interface>
#define CLASS channel_rpc<Interface>

#include <bitcoin/network/impl/channels/channel_rpc.ipp>

#undef CLASS
#undef TEMPLATE

#endif
