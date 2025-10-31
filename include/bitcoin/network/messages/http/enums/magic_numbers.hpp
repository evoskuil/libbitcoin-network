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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_MAGIC_NUMBERS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_MAGIC_NUMBERS_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// Client-server protocol constants.
///----------------------------------------------------------------------------

constexpr size_t max_url  = 1 * 1024;
constexpr uint16_t default_http = 80;
constexpr uint16_t default_tls = 443;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
