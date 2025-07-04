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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_MAGIC_NUMBERS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_MAGIC_NUMBERS_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

/// Network protocol constants.
///----------------------------------------------------------------------------

/// Explicit limits.
constexpr size_t max_address = 1'000;
constexpr size_t max_bloom_filter_add = 520;
constexpr size_t max_bloom_filter_functions = 50;
constexpr size_t max_bloom_filter_load = 36'000;
////constexpr size_t max_bloom_filter_hashes = 2'000;
constexpr size_t max_client_filters = 1'000;
constexpr size_t max_client_filter_headers = 2'000;
constexpr size_t client_filter_checkpoint_interval = 1'000;
constexpr size_t max_get_blocks = 500;
constexpr size_t max_get_headers = 2'000;
constexpr size_t max_inventory = 50'000;
constexpr size_t heading_command_size = 12;

/// This is just a guess, required as memory guard.
constexpr size_t max_reject_message = max_uint16;

/// This is arbitrary, useful as an address pool and block announce guard.
constexpr size_t maximum_advertisement = 10;

/// Limit given 10 and max supported height chain (stop hash excluded).
constexpr size_t max_locator = 10_size + system::floored_log2(max_size_t);

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
