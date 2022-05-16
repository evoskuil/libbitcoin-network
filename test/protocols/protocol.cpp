/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#include "../test.hpp"

struct protocol_tests_setup_fixture
{
    protocol_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }

    ~protocol_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_tests, protocol_tests_setup_fixture)

using namespace bc::network;
using namespace bc::system::chain;
using namespace bc::network::messages;

class mock_channel
  : public channel
{
public:
    using channel::channel;

    // Capture last sent payload.
    void send_bytes(const system::chunk_ptr& payload,
        result_handler&&) override
    {
        payload_ = payload;
    }

    // Override protected base to notify subscribers.
    code notify(identifier, uint32_t, system::reader&) override
    {
        return error::success;
        ////return channel::notify(id, version, source);
    }

    // Get last sent payload.
    system::chunk_ptr sent() const
    {
        return payload_;
    }

private:
    system::chunk_ptr payload_;
};

// Use mock acceptor to inject mock channel.
class mock_acceptor
  : public acceptor
{
public:
    mock_acceptor(asio::strand& strand, asio::io_context& service,
        const settings& settings)
      : acceptor(strand, service, settings), stopped_(false), port_(0)
    {
    }

    // Get captured port.
    uint16_t port() const
    {
        return port_;
    }

    // Get captured stopped.
    bool stopped() const
    {
        return stopped_;
    }

    // Capture port.
    code start(uint16_t port) override
    {
        port_ = port;
        return error::success;
    }

    // Capture stopped.
    void stop() override
    {
        stopped_ = true;
    }

    // Inject mock channel.
    void accept(accept_handler&& handler) override
    {
        const auto socket = std::make_shared<network::socket>(service_);
        const auto created = std::make_shared<mock_channel>(socket, settings_);

        // Must be asynchronous or is an infinite recursion.
        // This error code will set the re-listener timer and channel pointer is ignored.
        boost::asio::post(strand_, [=]()
        {
            handler(error::success, created);
        });
    }

private:
    bool stopped_;
    uint16_t port_;
};

// Use mock connector to inject mock channel.
class mock_connector
  : public connector
{
public:
    mock_connector(asio::strand& strand, asio::io_context& service,
        const settings& settings)
      : connector(strand, service, settings), stopped_(false)
    {
    }

    // Get captured stopped.
    bool stopped() const
    {
        return stopped_;
    }

    // Capture stopped.
    void stop() override
    {
        stopped_ = true;
    }

    // Inject mock channel.
    void connect(const std::string&, uint16_t, connect_handler&& handler) override
    {
        const auto socket = std::make_shared<network::socket>(service_);
        const auto created = std::make_shared<mock_channel>(socket, settings_);
        handler(error::success, created);
    }

private:
    bool stopped_;
};

// Use mock p2p network to inject mock channels.
class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    // Create mock acceptor to inject mock channel.
    acceptor::ptr create_acceptor() override
    {
        return std::make_shared<mock_acceptor>(strand(), service(),
            network_settings());
    }

    // Create mock connector to inject mock channel.
    connector::ptr create_connector() override
    {
        return std::make_shared<mock_connector>(strand(), service(),
            network_settings());
    }
};

class mock_session
  : public session
{
public:
    mock_session(p2p& network)
      : session(network)
    {
    }

    bool stopped() const noexcept
    {
        return session::stopped();
    }

    void attach_handshake(const channel::ptr&,
        result_handler&&) const noexcept override
    {
    }

    bool inbound() const noexcept override
    {
        return false;
    }

    bool notify() const noexcept override
    {
        return true;
    }
};

class mock_protocol
  : public protocol
{
public:
    typedef std::shared_ptr<mock_protocol> ptr;

    mock_protocol(const session& session, channel::ptr channel)
      : protocol(session, channel)
    {
    }

    virtual ~mock_protocol()
    {
    }

    config::authority authority() const
    {
        return protocol::authority();
    }

    uint64_t nonce() const noexcept
    {
        return protocol::nonce();
    }

    messages::version::ptr peer_version() const noexcept
    {
        return protocol::peer_version();
    }

    void set_peer_version(messages::version::ptr value) noexcept
    {
        protocol::set_peer_version(value);
    }

    uint32_t negotiated_version() const noexcept
    {
        return protocol::negotiated_version();
    }

    void set_negotiated_version(uint32_t value) noexcept
    {
        protocol::set_negotiated_version(value);
    }

    void stop(const code& ec)
    {
        protocol::stop(ec);
    }

    const network::settings& settings() const
    {
        return protocol::settings();
    }

    void saves(const messages::address_items& addresses)
    {
        return protocol::saves(addresses);
    }

    void fetches(fetches_handler&& handler)
    {
        return protocol::fetches(std::move(handler));
    }

    const std::string& name() const override
    {
        static const std::string name{ "name" };
        return name;
    }
};

BOOST_AUTO_TEST_SUITE_END()
