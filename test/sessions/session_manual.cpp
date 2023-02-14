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

BOOST_AUTO_TEST_SUITE(session_manual_tests)

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace bc::network::config;
using namespace bc::system::chain;

class mock_connector_connect_success
  : public connector
{
public:
    typedef std::shared_ptr<mock_connector_connect_success> ptr;

    using connector::connector;

    // Get captured connected.
    bool connected() const NOEXCEPT
    {
        return !is_zero(connects_);
    }

    // Get captured endpoint.
    const endpoint& peer() const NOEXCEPT
    {
        return peer_;
    }

    // Get captured stopped.
    bool stopped() const NOEXCEPT
    {
        return stopped_;
    }

    // Capture stopped and free channel.
    void stop() NOEXCEPT override
    {
        stopped_ = true;
        connector::stop();
    }

    // Handle connect, capture first connected hostname and port.
    void connect(const endpoint& peer,
        channel_handler&& handler) NOEXCEPT override
    {
        if (is_zero(connects_++))
            peer_ = peer;

        const auto socket = std::make_shared<network::socket>(log(), service_);
        const auto channel = std::make_shared<network::channel>(log(), socket,
            settings_);

        // Must be asynchronous or is an infinite recursion.
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            // Connect result code is independent of channel stop code.
            // Error code would set re-listener timer, channel pointer ignored.
            handler(error::success, channel);
        });
    }

protected:
    bool stopped_{ false };
    size_t connects_{ zero };
    endpoint peer_;
};

class mock_connector_connect_fail
  : public mock_connector_connect_success
{
public:
    typedef std::shared_ptr<mock_connector_connect_fail> ptr;

    using mock_connector_connect_success::mock_connector_connect_success;

    // Handle connect with service_stopped error.
    void connect(const endpoint& peer,
        channel_handler&& handler) NOEXCEPT override
    {
        if (is_zero(connects_++))
            peer_ = peer;

        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            // This error is eaten by handle_connect, due to retry logic.
            // invalid_magic is a non-terminal code (timer retry).
            // Connect result code is independent of the channel stop code.
            handler(error::invalid_magic, nullptr);
        });
    }
};

class mock_session_manual
  : public session_manual
{
public:
    using session_manual::session_manual;

    bool inbound() const NOEXCEPT override
    {
        return session_manual::inbound();
    }

    bool notify() const NOEXCEPT override
    {
        return session_manual::notify();
    }

    bool stopped() const NOEXCEPT override
    {
        return session_manual::stopped();
    }

    void defer(result_handler&& handler, const uintptr_t& id) NOEXCEPT override
    {
        // This captures the session, which remains in scope until handler completes.
        // The timer closure must be released before exit (ensure session stopped).
        // Anything posted to the network threadpool blocks p2p.close().
        session_manual::defer(std::move(handler), id);
    }

    const endpoint& start_connect_endpoint() const NOEXCEPT
    {
        return start_connect_endpoint_;
    }

    // Capture first start_connect call.
    void start_connect(const code&, const endpoint& peer,
        const connector::ptr& connector,
        const p2p::channel_notifier& handler) NOEXCEPT override
    {
        // Must be first to ensure connector::start_connect() preceeds promise release.
        session_manual::start_connect({}, peer, connector, handler);

        if (is_one(connects_))
            reconnect_.set_value(true);

        if (is_zero(connects_++))
        {
            start_connect_endpoint_ = peer;
            connect_.set_value(true);
        }
    }

    bool connected() const NOEXCEPT
    {
        return !is_zero(connects_);
    }

    bool require_connected() const NOEXCEPT
    {
        return connect_.get_future().get();
    }

    bool require_reconnect() const NOEXCEPT
    {
        return reconnect_.get_future().get();
    }

    void attach_handshake(const channel::ptr&,
        result_handler&& handshake) const NOEXCEPT override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
            handshake_.set_value(true);
        }

        // Simulate handshake successful completion.
        handshake(error::success);
    }

    bool attached_handshake() const NOEXCEPT
    {
        return handshaked_;
    }

    bool require_attached_handshake() const NOEXCEPT
    {
        return handshake_.get_future().get();
    }

protected:
    mutable bool handshaked_{ false };
    mutable std::promise<bool> handshake_;

private:
    code connect_code_{ error::success };
    endpoint start_connect_endpoint_;
    size_t connects_{ zero };
    mutable std::promise<bool> connect_;
    mutable std::promise<bool> reconnect_;
};

class mock_session_manual_handshake_failure
  : public mock_session_manual
{
public:
    using mock_session_manual::mock_session_manual;

    void attach_handshake(const channel::ptr&,
        result_handler&& handshake) const NOEXCEPT override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
            handshake_.set_value(true);
        }

        // Simulate handshake failure.
        handshake(error::invalid_checksum);
    }
};

template <class Connector = connector>
class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    // Get last created connector.
    typename Connector::ptr get_connector() const NOEXCEPT
    {
        return connector_;
    }

    // Create mock connector to inject mock channel.
    connector::ptr create_connector() NOEXCEPT override
    {
        return ((connector_ = std::make_shared<Connector>(log(), strand(),
            service(), network_settings())));
    }

    session_inbound::ptr attach_inbound_session() NOEXCEPT override
    {
        return attach<mock_inbound_session>(*this);
    }

    session_outbound::ptr attach_outbound_session() NOEXCEPT override
    {
        return attach<mock_outbound_session>(*this);
    }

    session_seed::ptr attach_seed_session() NOEXCEPT override
    {
        return attach<mock_seed_session>(*this);
    }

private:
    typename Connector::ptr connector_;

    class mock_inbound_session
      : public session_inbound
    {
    public:
        using session_inbound::session_inbound;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };

    class mock_outbound_session
      : public session_outbound
    {
    public:
        using session_outbound::session_outbound;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };

    class mock_seed_session
      : public session_seed
    {
    public:
        using session_seed::session_seed;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };
};

// properties

BOOST_AUTO_TEST_CASE(session_manual__inbound__always__false)
{
    const logger log{};
    settings set(selection::mainnet);
    p2p net(set, log);
    mock_session_manual session(net, 1);
    BOOST_REQUIRE(!session.inbound());
}

BOOST_AUTO_TEST_CASE(session_manual__notify__always__true)
{
    const logger log{};
    settings set(selection::mainnet);
    p2p net(set, log);
    mock_session_manual session(net, 1);
    BOOST_REQUIRE(session.notify());
}

// stop

BOOST_AUTO_TEST_CASE(session_manual__stop__started__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        // Will cause started to be set (only).
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__stop__stopped__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<> net(set, log);
    mock_session_manual session(net, 1);

    std::promise<bool> promise;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        promise.set_value(true);
    });

    BOOST_REQUIRE(promise.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

// start

BOOST_AUTO_TEST_CASE(session_manual__start__started__operation_failed)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<code> restart;
    boost::asio::post(net.strand(), [=, &restart]()
    {
        session->start([&](const code& ec)
        {
            restart.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(restart.get_future().get(), error::operation_failed);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

// connect

BOOST_AUTO_TEST_CASE(session_manual__connect_unhandled__stopped__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint peer{ "42.42.42.42", 42 };

    boost::asio::post(net.strand(), [=]()
    {
        // This synchronous overload has no handler, so cannot capture values.
        session->connect(peer);
    });

    // No handler so rely on connect.
    BOOST_REQUIRE(session->require_connected());

    // A connector was created/subscribed, which requires unstarted service stop.
    BOOST_REQUIRE(net.get_connector());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__connect_handled__stopped__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint peer{ "42.42.42.42", 42 };

    std::promise<code> connected;
    boost::asio::post(net.strand(), [=, &connected]()
    {
        session->connect(peer, [&](const code& ec, const channel::ptr& channel)
        {
            BOOST_REQUIRE(ec && !channel);
            connected.set_value(ec);
            return true;
        });
    });

    BOOST_REQUIRE_EQUAL(connected.get_future().get(), error::service_stopped);

    // A connector was created/subscribed, which requires unstarted service stop.
    BOOST_REQUIRE(net.get_connector());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__handle_connect__connect_fail__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set, log);

    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint peer{ "42.42.42.42", 42 };

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    auto first = true;
    std::promise<bool> started_connect;
    std::promise<code> connected;
    boost::asio::post(net.strand(), [&]()
    {
        session->connect(peer, [&](const code& ec, const channel::ptr& channel)
        {
            BOOST_REQUIRE(ec && !channel);

            if (first)
            {
                connected.set_value(ec);
                first = false;
            }

            // Continue after connect fail, reenters here.
            return true;
        });

        // connector.connect has been invoked, though its handler is pending.
        started_connect.set_value(true);
    });

    BOOST_REQUIRE(started_connect.get_future().get());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    // connector.connect sets invalid_magic, causing a timer reconnect.
    BOOST_REQUIRE_EQUAL(connected.get_future().get(), error::invalid_magic);

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__handle_connect__connect_success_stopped__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<mock_connector_connect_success> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint expected{ "42.42.42.42", 42 };

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    std::promise<code> connected;
    boost::asio::post(net.strand(), [=, &stopped, &connected]()
    {
        session->connect(expected, [&](const code& ec, const channel::ptr& channel)
        {
            BOOST_REQUIRE(!channel);
            connected.set_value(ec);
            return true;
        });

        // connector.connect has been invoked, though its handler is pending.
        // Stop the session after connect but before handle_connect is invoked.
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE_EQUAL(connected.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(session->require_connected());
    BOOST_REQUIRE_EQUAL(session->start_connect_endpoint(), expected);
    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__handle_channel_start__handshake_error__invalid_checksum)
{
    const logger log{};
    settings set(selection::mainnet);

    mock_p2p<mock_connector_connect_success> net(set, log);

    auto session = std::make_shared<mock_session_manual_handshake_failure>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint expected{ "42.42.42.42", 42 };

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    auto first = true;
    std::promise<code> connected;
    boost::asio::post(net.strand(), [&]()
    {
        session->connect(expected, [&](const code& ec, const channel::ptr& channel)
        {
            BOOST_REQUIRE(ec && !channel);

            if (first)
            {
                connected.set_value(ec);
                first = false;
            }

            // Continue after connect fail, reenters here.
            return true;
        });
    });

    // mock_session_manual_handshake_failure sets channel.stop(invalid_checksum).
    BOOST_REQUIRE_EQUAL(connected.get_future().get(), error::invalid_checksum);
    BOOST_REQUIRE(session->require_connected());
    BOOST_REQUIRE_EQUAL(session->start_connect_endpoint(), expected);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(session->attached_handshake());
}

// start via network (not required for coverage)
// ============================================================================

BOOST_AUTO_TEST_CASE(session_manual__start__network_start__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<> net(set, log);

    std::promise<code> started;
    net.start([&](const code& ec)
    {
        started.set_value(ec);
    });
    
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_no_connections__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());

    // Connector is not invoked.
    mock_p2p<> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            run.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_configured_connection__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());

    const endpoint expected{ "42.42.42.42", 42 };
    set.peers.push_back(expected);

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);

    // Connector is established and connect is called for all configured
    // connections prior to completion of network run call.
    BOOST_REQUIRE(net.get_connector());
    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_configured_connections__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());

    const endpoint expected{ "42.42.42.4", 42 };
    set.peers.push_back({ "42.42.42.1", 42 });
    set.peers.push_back({ "42.42.42.2", 42 });
    set.peers.push_back({ "42.42.42.3", 42 });
    set.peers.push_back(expected);

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);

    // Connector is established and connect is called for all configured
    // connections prior to completion of network run call. The last connection
    // is reflected by the mock connector as connections invoked in order.
    BOOST_REQUIRE(net.get_connector());
    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect1__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());

    const endpoint expected{ "42.42.42.42", 42 };

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            net.connect(expected);
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect2__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());

    const endpoint expected{ "42.42.42.42", 42 };

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            net.connect(expected);
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect3__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());

    const endpoint expected{ "42.42.42.42", 42 };

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set, log);

    auto first = true;
    std::promise<code> start{};
    std::promise<code> run{};
    std::promise<code> connect{};
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            net.connect(expected, [&](const code& ec, const channel::ptr& channel)
            {
                BOOST_REQUIRE(ec && !channel);

                if (first)
                {
                    connect.set_value(ec);
                    first = false;
                }

                // Continue after connect fail, reenters here.
                return true;
            });

            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);

    // The connection loops on connect failure until service stop.
    net.close();

    // connector.connect sets invalid_magic, causing a timer reconnect.
    BOOST_REQUIRE_EQUAL(connect.get_future().get(), error::invalid_magic);
    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
}

BC_POP_WARNING()

BOOST_AUTO_TEST_SUITE_END()
