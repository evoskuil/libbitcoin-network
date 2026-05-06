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

#include <memory>
#include <utility>
#include <variant>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {
    
using namespace system;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// HTTP (read).
// ----------------------------------------------------------------------------

void socket::http_read(http::flat_buffer& buffer,
    http::request& request, count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_read, shared_from_this(),
            std::ref(buffer), std::ref(request), std::move(handler)));
}

// private
void socket::do_http_read(ref<http::flat_buffer> buffer,
    const ref<http::request>& request,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    async_read_http(buffer.get(), request.get(), handler);
}

// private
void socket::handle_http_read(const boost_code& ec, size_t size,
    const ref<http::request>& request, const http_parser_ptr& parser,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    if (!ec && boost::beast::websocket::is_upgrade(parser->get()))
    {
        handler(set_websocket(parser->get()), size);
        return;
    }

    if (!ec)
    {
        request.get() = parser->release();
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http-read", ec);
    handler(code, size);
}

// HTTP (write).
// ----------------------------------------------------------------------------

void socket::http_write(http::response&& response,
    count_handler&& handler) NOEXCEPT
{
    const auto out = move_shared(std::move(response));
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_write, shared_from_this(),
            out, std::move(handler)));
}

// private
void socket::do_http_write(const http::response_ptr& response,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    async_write_http(std::move(*response), handler);
}

// private
void socket::handle_http_write(const boost_code& ec, size_t size,
    const http::response_ptr&, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http-write", ec);
    handler(code, size);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
