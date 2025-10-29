/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_HTTP_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_HTTP_HPP

#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/distributors/distributors.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/http/messages.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Half-duplex reading of http-request and sending of http-response.
class BCT_API channel_http
  : public channel, protected tracker<channel_http>
{
public:
    typedef std::shared_ptr<channel_http> ptr;
    using options_t = settings::http_server;

    /// Subscribe to request from peer (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Message, typename Handler =
        distributor_http::handler<Message>>
    void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        distributor_.subscribe(std::forward<Handler>(handler));
    }

    /// Serialize and write response to peer (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <class Message>
    void send(Message&& response, result_handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        const auto ptr = system::make_shared(std::forward<Message>(response));

        // Capture response in intermediate completion handler.
        auto complete = [self = shared_from_base<channel_http>(), ptr,
            handle = std::move(handler)](const code& ec, size_t) NOEXCEPT
        {
            if (ec) self->stop(ec);
            handle(ec);
        };

        if (!ptr)
        {
            complete(error::bad_alloc, zero);
            return;
        }

        write(*ptr, std::move(complete));
    }

    /// Uses peer config for timeouts if not specified via other construct.
    /// Construct client channel to encapsulate and communicate on the socket.
    channel_http(const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero,
        const options_t& options={}) NOEXCEPT
      : channel(log, socket, settings, identifier, options.timeout()),
        request_buffer_(system::ceilinged_add(http::max_head, http::max_body)),
        distributor_(socket->strand()),
        tracker<channel_http>(log)
    {
    }

    /// Resume reading from the socket (requires strand).
    void resume() NOEXCEPT override;

    /// http is half-duplex, so reads must wait until send is completed.
    /// Must be called (only once) from protocol message handler (if no stop).
    /// Calling more than once is safe but implies a protocol problem. Failure
    /// to call after successful message handling results in stalled channel.
    /// This can be buried in the common send completion hander, conditioned on
    /// on the result code. This is simpler and more performant than having the
    /// distributor issue a completion handler to invoke read continuation.
    void read_request() NOEXCEPT;

protected:
    /// Stranded handler invoked from stop().
    void stopping(const code& ec) NOEXCEPT override;

private:
    void do_stop(const code& ec) NOEXCEPT;
    void handle_read_request(const code& ec, size_t bytes_read,
        const http::request_cptr& request) NOEXCEPT;

    // These are protected by strand.
    http::flat_buffer request_buffer_;
    distributor_http distributor_;
    bool reading_{};
};

} // namespace network
} // namespace libbitcoin

#endif
