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
#ifndef LIBBITCOIN_NETWORK_TEST_FUNCTIONAL_PEER_SETUP_FIXTURE
#define LIBBITCOIN_NETWORK_TEST_FUNCTIONAL_PEER_SETUP_FIXTURE

#include "../test.hpp"

#define PEER_FUNCTIONAL_ENDPOINT "127.0.0.1:65008"

// Runs a real net instance accepting on loopback, with the test acting as
// the remote peer over a raw blocking socket (framing via peer messages).
struct peer_setup_fixture
{
    DELETE_COPY_MOVE(peer_setup_fixture);

    using configurator = std::function<void(network::settings&)>;
    explicit peer_setup_fixture(const configurator& configure={});
    ~peer_setup_fixture();

    /// Write a framed message to the node.
    void send(const std::string& command, const system::data_chunk& payload);

    /// Serialize and write a framed message to the node.
    template <class Message>
    void send(const Message& message, uint32_t version)
    {
        system::data_chunk payload(message.size(version));
        BOOST_REQUIRE(message.serialize(version, payload));
        send(Message::command, payload);
    }

    /// Read one framed message from the node.
    std::pair<std::string, system::data_chunk> receive();

    /// Read framed messages from the node until the command matches.
    system::data_chunk receive(const std::string& command);

    /// Perform the version handshake, retains the node's version message.
    bool handshake(uint64_t services=0,
        uint32_t version=messages::peer::level::maximum_protocol);

    /// The node's version message (set by handshake).
    messages::peer::version::cptr node_version{};

protected:
    network::settings settings_;
    network::logger log_{};
    network::net net_;

private:
    boost::asio::io_context io_{};
    boost::asio::ip::tcp::socket socket_{ io_ };
};

#endif
