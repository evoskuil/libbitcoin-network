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

namespace libbitcoin {
namespace network {
    
// TODO: Interface argument provides templated dispatch for rpc protocols.
// TODO: transport independent sender methods in each are duck typed for rpc protocol.
// TODO: network::channel_trpc<Interface> : network::channel
// TODO: network::channel_wrpc<Interface> : network::channel_ws
//                                        : network::channel_http
//                                        : network::channel
//
// TODO: reused channel state unique to electrum/bitcoind.
// TODO: server::channel_electrum_base
// TODO: server::channel_bitcoind_base
//
// TODO: aliases.
// TODO: server::channel_electrum_tcp : channel_electrum_base, network::channel_trpc<electrum>
// TODO: server::channel_electrum_web : channel_electrum_base, network::channel_wrpc<electrum>
// TODO: server::channel_bitcoind_tcp : channel_bitcoind_base, network::channel_trpc<bitcoind>
// TODO: server::channel_bitcoind_web : channel_bitcoind_base, network::channel_wrpc<bitcoind>
//
// TODO: base sender methods for passthrough to channel senders.
// TODO: these could be implemented in the two protocols to reduce compile time.
// TODO: network::protocol_rpc<Channel> : network::protocol
//
// TODO: reused protocol logic unique to electrum/bitcoind.
// TODO: server::protocol_electrum<Channel> : network::protocol_rpc<Channel>
// TODO: server::protocol_bitcoind<Channel> : network::protocol_rpc<Channel>
//
// TODO: aliases.
// TODO: server::protocol_electrum_tcp : network::protocol_electrum<channel_electrum_tcp>
// TODO: server::protocol_electrum_web : network::protocol_electrum<channel_electrum_web> 
// TODO: server::protocol_bitcoind_tcp : network::protocol_bitcoind<channel_bitcoind_tcp>
// TODO: server::protocol_bitcoind_web : network::protocol_bitcoind<channel_bitcoind_web>

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
        response_buffer_(system::to_shared<http::flat_buffer>()),
        request_buffer_(options.minimum_buffer)
    {
    }

    /// Senders (requires strand).
    inline void send_code(const code& ec, result_handler&& handler) NOEXCEPT;
    inline void send_error(rpc::result_t&& error,
        result_handler&& handler) NOEXCEPT;
    inline void send_result(rpc::value_t&& result, size_t size_hint,
        result_handler&& handler) NOEXCEPT;
    inline void send_notification(const rpc::string_t& method,
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

    /// Size and assign response_buffer_ (value type is json-rpc::json).
    template <typename Message>
    inline rpc::message_ptr<Message> assign_message(Message&& message,
        size_t size_hint) NOEXCEPT;

    /// Handle send<response> completion, invokes receive().
    template <typename Message>
    inline void handle_send(const code& ec, size_t bytes,
        const rpc::message_cptr<Message>& message,
        const result_handler& handler) NOEXCEPT;

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
    http::flat_buffer_ptr response_buffer_;
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
