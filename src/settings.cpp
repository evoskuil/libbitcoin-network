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
#include <bitcoin/network/settings.hpp>

#include <algorithm>
#include <filesystem>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/messages/http/http.hpp>
#include <bitcoin/network/messages/peer/messages.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages::peer;

static_assert(heading::maximum_payload(level::canonical, true) == 4'000'000);
static_assert(heading::maximum_payload(level::canonical, false) == 1'800'003);

// TODO: make witness support configurable.
// Common default values (no settings context).
settings::settings() NOEXCEPT
  : threads(1),
    address_upper(10),
    address_lower(5),
    protocol_maximum(level::maximum_protocol),
    protocol_minimum(level::minimum_protocol),
    services_maximum(service::maximum_services),
    services_minimum(service::minimum_services),
    invalid_services(176),
    enable_address(false),
    enable_address_v2(false),
    enable_witness_tx(false),
    enable_compact(false),
    enable_alert(false),
    enable_reject(false),
    enable_relay(false),
    enable_ipv6(false),
    enable_loopback(false),
    validate_checksum(false),
    identifier(0),
    inbound_connections(0),
    outbound_connections(10),
    connect_batch_size(5),
    retry_timeout_seconds(1),
    connect_timeout_seconds(5),
    handshake_timeout_seconds(30),
    seeding_timeout_seconds(30),
    channel_heartbeat_minutes(5),
    channel_inactivity_minutes(10),
    channel_expiration_minutes(1440),
    maximum_skew_minutes(120),
    host_pool_capacity(0),
    rate_limit(1024),
    minimum_buffer(4'000'000),
    user_agent(BC_USER_AGENT)
{
}

// Use push_back due to initializer_list bug:
// stackoverflow.com/a/20168627/1172329
settings::settings(chain::selection context) NOEXCEPT
  : settings()
{
    // Configure common deviations from defaults.
    switch (context)
    {
        case chain::selection::mainnet:
        {
            identifier = 3652501241;
            seeds.reserve(4);
            seeds.emplace_back("mainnet1.libbitcoin.net", 8333_u16);
            seeds.emplace_back("mainnet2.libbitcoin.net", 8333_u16);
            seeds.emplace_back("mainnet3.libbitcoin.net", 8333_u16);
            seeds.emplace_back("mainnet4.libbitcoin.net", 8333_u16);
            binds.emplace_back(asio::address{}, 8333_u16);
            break;
        }

        case chain::selection::testnet:
        {
            identifier = 118034699;
            seeds.reserve(4);
            seeds.emplace_back("testnet1.libbitcoin.net", 18333_u16);
            seeds.emplace_back("testnet2.libbitcoin.net", 18333_u16);
            seeds.emplace_back("testnet3.libbitcoin.net", 18333_u16);
            seeds.emplace_back("testnet4.libbitcoin.net", 18333_u16);
            binds.emplace_back(asio::address{}, 18333_u16);
            break;
        }

        case chain::selection::regtest:
        {
            identifier = 3669344250;
            binds.emplace_back(asio::address{}, 18444_u16);

            // Regtest is private network only, so there is no seeding.
            break;
        }

        default:
        case chain::selection::none:
        {
        }
    }
}

void settings::initialize() NOEXCEPT
{
    BC_ASSERT_MSG(friends.empty(), "friends not empty");

    // Dynamic conversion of peers is O(N^2), so set on initialize.
    // This converts endpoints to addresses, so will produce the default
    // address for any hosts that are DNS names (i.e. not IP addresses).
    friends = system::projection<network::config::authorities>(peers);
}

bool settings::witness_node() const NOEXCEPT
{
    return to_bool(services_minimum & service::node_witness);
}

bool settings::inbound_enabled() const NOEXCEPT
{
    return to_bool(inbound_connections) && !binds.empty();
}

bool settings::outbound_enabled() const NOEXCEPT
{
    return to_bool(outbound_connections)
        && to_bool(host_pool_capacity)
        && to_bool(connect_batch_size);
}

bool settings::advertise_enabled() const NOEXCEPT
{
    return inbound_enabled() && !selfs.empty();
}

size_t settings::maximum_payload() const NOEXCEPT
{
    return heading::maximum_payload(protocol_maximum,
        to_bool(services_maximum & service::node_witness));
}

config::authority settings::first_self() const NOEXCEPT
{
    return selfs.empty() ? config::authority{} : selfs.front();
}

// Randomized from 50% to maximum milliseconds (specified in seconds).
steady_clock::duration settings::retry_timeout() const NOEXCEPT
{
    const auto from = retry_timeout_seconds * 500_u64;
    const auto to = retry_timeout_seconds * 1'000_u64;
    return milliseconds{ system::pseudo_random::next(from, to) };
}

// Randomized from 50% to maximum milliseconds (specified in seconds).
steady_clock::duration settings::connect_timeout() const NOEXCEPT
{
    const auto from = connect_timeout_seconds * 500_u64;
    const auto to = connect_timeout_seconds * 1'000_u64;
    return milliseconds{ system::pseudo_random::next(from, to) };
}

steady_clock::duration settings::channel_handshake() const NOEXCEPT
{
    return seconds(handshake_timeout_seconds);
}

steady_clock::duration settings::channel_germination() const NOEXCEPT
{
    return seconds(seeding_timeout_seconds);
}

steady_clock::duration settings::channel_heartbeat() const NOEXCEPT
{
    return minutes(channel_heartbeat_minutes);
}

steady_clock::duration settings::channel_inactivity() const NOEXCEPT
{
    return minutes(channel_inactivity_minutes);
}

steady_clock::duration settings::channel_expiration() const NOEXCEPT
{
    return minutes(channel_expiration_minutes);
}

steady_clock::duration settings::maximum_skew() const NOEXCEPT
{
    return minutes(maximum_skew_minutes);
}

size_t settings::minimum_address_count() const NOEXCEPT
{
    // Cannot overflow as long as both are uint16_t.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return safe_multiply<size_t>(connect_batch_size, outbound_connections);
    BC_POP_WARNING()
}

std::filesystem::path settings::file() const NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return path / "hosts.cache";
    BC_POP_WARNING()
}

bool settings::disabled(const address_item& item) const NOEXCEPT
{
    return !enable_ipv6 && config::is_v6(item.ip);
}

bool settings::insufficient(const address_item& item) const NOEXCEPT
{
    return (item.services & services_minimum) != services_minimum;
}

bool settings::unsupported(const address_item& item) const NOEXCEPT
{
    return to_bool(item.services & invalid_services);
}

bool settings::blacklisted(const address_item& item) const NOEXCEPT
{
    return contains(blacklists, item);
}

bool settings::whitelisted(const address_item& item) const NOEXCEPT
{
    return whitelists.empty() || contains(whitelists, item);
}

bool settings::peered(const address_item& item) const NOEXCEPT
{
    // Friends should be mapped from peers by initialize().
    return contains(friends, item);
}

bool settings::excluded(const address_item& item) const NOEXCEPT
{
    return !is_specified(item)
        || disabled(item)
        || insufficient(item)
        || unsupported(item)
        || peered(item)
        || blacklisted(item)
        || !whitelisted(item);
}

// tcp_server
// ----------------------------------------------------------------------------

settings::tcp_server::tcp_server(const std::string_view& logging_name) NOEXCEPT
  : name(logging_name)
{
}

steady_clock::duration settings::tcp_server::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

bool settings::tcp_server::enabled() const NOEXCEPT
{
    return !binds.empty() && to_bool(connections);
}

// http_server
// ----------------------------------------------------------------------------

system::string_list settings::http_server::host_names() const NOEXCEPT
{
    // secure changes default port from 80 to 443.
    const auto port = secure ? http::default_tls : http::default_http;
    return config::to_host_names(hosts, port);
}

// websocket_server
// ----------------------------------------------------------------------------

} // namespace network
} // namespace libbitcoin
