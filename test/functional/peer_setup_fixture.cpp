/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include "peer_setup_fixture.hpp"
#include <future>

using namespace bc::system;
using namespace bc::network::messages::peer;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

peer_setup_fixture::peer_setup_fixture(const configurator& configure)
  : settings_{ chain::selection::mainnet },
    net_{ settings_, log_ }
{
    test::clear(test::directory);
    settings_.path = TEST_DIRECTORY;
    settings_.inbound.connections = 1;
    settings_.inbound.binds.clear();
    settings_.inbound.binds.emplace_back(PEER_FUNCTIONAL_ENDPOINT);
    settings_.outbound.connections = 0;
    settings_.outbound.seeds.clear();

    // Apply test-specific configuration overrides.
    if (configure)
        configure(settings_);

    std::promise<code> started{};
    net_.start([&](const code& ec) NOEXCEPT
    {
        started.set_value(ec);
    });

    // Block until the network is started.
    auto ec = started.get_future().get();
    BOOST_REQUIRE_MESSAGE(!ec, ec.message());

    std::promise<code> running{};
    net_.run([&](const code& ec) NOEXCEPT
    {
        running.set_value(ec);
    });

    // Block until the network is running.
    ec = running.get_future().get();
    BOOST_REQUIRE_MESSAGE(!ec, ec.message());
    socket_.connect(settings_.inbound.binds.back().to_endpoint());
}

peer_setup_fixture::~peer_setup_fixture()
{
    socket_.close();
    net_.close();
}

void peer_setup_fixture::send(const std::string& command,
    const data_chunk& payload)
{
    const auto head = heading::factory(settings_.identifier, command, payload);
    data_chunk frame(heading::size() + payload.size());
    BOOST_REQUIRE(head.serialize({ frame.data(),
        std::next(frame.data(), heading::size()) }));

    std::copy(payload.begin(), payload.end(),
        std::next(frame.begin(), heading::size()));
    boost::asio::write(socket_, boost::asio::buffer(frame));
}

std::pair<std::string, data_chunk> peer_setup_fixture::receive()
{
    data_array<heading::size()> head_data{};
    boost::asio::read(socket_, boost::asio::buffer(head_data));
    const auto head = heading::deserialize(head_data);
    BOOST_REQUIRE(head);

    data_chunk payload(head->payload_size);
    if (!payload.empty())
        boost::asio::read(socket_, boost::asio::buffer(payload));

    return { head->command, std::move(payload) };
}

data_chunk peer_setup_fixture::receive(const std::string& command)
{
    while (true)
    {
        auto message = receive();
        if (message.first == command)
            return std::move(message.second);
    }
}

bool peer_setup_fixture::handshake(uint64_t services, uint32_t value)
{
    version out{};
    out.value = value;
    out.services = services;
    out.timestamp = sign_cast<uint64_t>(zulu_time());
    out.nonce = 42424242;
    out.user_agent = "/test/";
    out.start_height = 0;
    out.relay = false;
    send(out, value);

    // The node sends its version upon attach and verack upon our version.
    auto got_version = false;
    auto got_acknowledge = false;
    while (!got_version || !got_acknowledge)
    {
        const auto message = receive();
        if (message.first == version::command)
        {
            node_version = version::deserialize(value, message.second);
            if (!node_version)
                return false;

            got_version = true;
        }
        else if (message.first == version_acknowledge::command)
        {
            got_acknowledge = true;
        }
    }

    send(version_acknowledge{}, value);
    return true;
}

BC_POP_WARNING()
