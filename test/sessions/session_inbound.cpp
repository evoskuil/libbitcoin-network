/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

struct session_inbound_tests_setup_fixture
{
    session_inbound_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }

    ~session_inbound_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }
};

BOOST_FIXTURE_TEST_SUITE(session_inbound_tests, session_inbound_tests_setup_fixture)

using namespace bc::network;
using namespace bc::system::chain;
using namespace bc::network::messages;

// Use to fake start success.
class mock_acceptor_start_success_accept_success
  : public acceptor
{
public:
    typedef std::shared_ptr<mock_acceptor_start_success_accept_success> ptr;

    mock_acceptor_start_success_accept_success(asio::strand& strand,
        asio::io_context& service, const settings& settings)
      : acceptor(strand, service, settings), stopped_(false), accepts_(zero), port_(0)
    {
    }

    // Get captured port.
    virtual uint16_t port() const
    {
        return port_;
    }

    // Get captured accepted.
    virtual bool accepted() const
    {
        return !is_zero(accepts_);
    }

    // Get captured reaccepted.
    virtual bool reaccepted() const
    {
        return accepts_ > one;
    }

    // Get captured stopped.
    virtual bool stopped() const
    {
        return stopped_;
    }

    // Capture port, succeed on first (others prevents tight success loop).
    code start(uint16_t port) override
    {
        port_ = port;
        return !accepted() ? error::success : error::unknown;
    }

    // Capture stopped and free channel.
    void stop() override
    {
        stopped_ = true;
        acceptor::stop();
    }

    // Handle accept.
    void accept(accept_handler&& handler) override
    {
        ++accepts_;
        const auto socket = std::make_shared<network::socket>(service_);
        const auto channel = std::make_shared<network::channel>(socket, settings_);

        // Must be asynchronous or is an infinite recursion.
        // This error code will set the re-listener timer and channel pointer is ignored.
        boost::asio::post(strand_, [=]()
        {
            handler(error::success, channel);
        });
    }

protected:
    bool stopped_;
    size_t accepts_;
    uint16_t port_;
};

// Use to fake start success.
class mock_acceptor_start_success_accept_fail
  : public mock_acceptor_start_success_accept_success
{
public:
    typedef std::shared_ptr<mock_acceptor_start_success_accept_fail> ptr;

    mock_acceptor_start_success_accept_fail(asio::strand& strand,
        asio::io_context& service, const settings& settings)
      : mock_acceptor_start_success_accept_success(strand, service, settings)
    {
    }

    // Handle accept with unknown error.
    void accept(accept_handler&& handler) override
    {
        ++accepts_;
        boost::asio::post(strand_, [=]()
        {
            handler(error::unknown, nullptr);
        });
    }
};

class mock_acceptor_start_stopped
  : public mock_acceptor_start_success_accept_fail
{
public:
    typedef std::shared_ptr<mock_acceptor_start_stopped> ptr;

    using mock_acceptor_start_success_accept_fail::mock_acceptor_start_success_accept_fail;

    // Handle accept with service_stopped error.
    void accept(accept_handler&& handler) override
    {
        ++accepts_;

        // Must be asynchronous or is an infinite recursion.
        // This error code will terminate the listener loop.
        boost::asio::post(strand_, [=]()
        {
            handler(error::service_stopped, nullptr);
        });
    }
};

class mock_acceptor_start_fail
  : public mock_acceptor_start_success_accept_fail
{
public:
    typedef std::shared_ptr<mock_acceptor_start_fail> ptr;

    using mock_acceptor_start_success_accept_fail::mock_acceptor_start_success_accept_fail;

    // Capture port, fail.
    code start(uint16_t port) override
    {
        port_ = port;
        return error::invalid_magic;
    }
};

// Use mock p2p network to inject mock channels.
template <class Acceptor>
class mock_p2p
  : public p2p
{
public:
    typename Acceptor::ptr acceptor;

    using p2p::p2p;

    // Create mock acceptor to inject mock channel.
    acceptor::ptr create_acceptor() override
    {
        acceptor = std::make_shared<Acceptor>(strand(), service(),
            network_settings());

        return acceptor;
    }
};

class mock_session_inbound
  : public session_inbound
{
public:
    mock_session_inbound(p2p& network)
      : session_inbound(network), accepted_(false), attached_(false)
    {
    }

    bool inbound() const noexcept
    {
        return session_inbound::inbound();
    }

    bool stopped() const noexcept
    {
        return session_inbound::stopped();
    }

    bool accepted() const noexcept
    {
        return accepted_;
    }

    bool require_accepted() const noexcept
    {
        return accept_.get_future().get();
    }

    void start_accept(const code& ec) override
    {
        if (!accepted_)
        {
            accepted_ = true;
            accept_.set_value(true);
        }

        session_inbound::start_accept(ec);
    }

    bool attached() const noexcept
    {
        return attached_;
    }

    bool require_attached() const noexcept
    {
        return attach_.get_future().get();
    }

    void attach_handshake(const channel::ptr& channel,
        result_handler handshake) const override
    {
        if (!attached_)
        {
            attached_ = true;
            attach_.set_value(true);
        }

        // Simulate handshake successful completion.
        handshake(error::success);
    }

private:
    mutable bool accepted_;
    mutable bool attached_;
    mutable std::promise<bool> accept_;
    mutable std::promise<bool> attach_;
};

// inbound

BOOST_AUTO_TEST_CASE(session_inbound__inbound__always__true)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session_inbound session(net);
    BOOST_REQUIRE(session.inbound());
}

// start

BOOST_AUTO_TEST_CASE(session_inbound__start__no_inbound_connections__stopped)
{
    settings set(selection::mainnet);
    set.inbound_connections = 0;
    p2p net(set);
    mock_session_inbound session(net);
    BOOST_REQUIRE(session.stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        // Will not cause started to be set.
        session.start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(session.stopped());
}

BOOST_AUTO_TEST_CASE(session_inbound__start__started__operation_failed)
{
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    p2p net(set);
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        // Will cause started to be set.
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<code> restarted;
    boost::asio::post(net.strand(), [&]()
    {
        // Already started returns operation_failed.
        session->start([&](const code& ec)
        {
            restarted.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(restarted.get_future().get(), error::operation_failed);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

// net start

BOOST_AUTO_TEST_CASE(session_inbound__net_start__no_inbound_connections__expected)
{
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    set.connect_batch_size = 0;
    set.outbound_connections = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    // Start will return invalid_magic if executed, but this will bypass it.
    set.inbound_connections = 0;
    set.inbound_port = 42;

    mock_p2p<mock_acceptor_start_fail> net(set);

    std::promise<bool> promise_run;
    const auto run_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_run.set_value(true);
    };

    const auto start_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        net.run(run_handler);
    };

    net.start(start_handler);
    BOOST_REQUIRE(promise_run.get_future().get());
}

BOOST_AUTO_TEST_CASE(session_inbound__net_start__inbound_port_zero__expected)
{
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    set.connect_batch_size = 0;
    set.outbound_connections = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    // Start will return invalid_magic if executed, but this will bypass it.
    set.inbound_port = 0;
    set.inbound_connections = 42;

    mock_p2p<mock_acceptor_start_fail> net(set);

    std::promise<bool> promise_run;
    const auto run_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_run.set_value(true);
    };

    const auto start_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        net.run(run_handler);
    };

    net.start(start_handler);
    BOOST_REQUIRE(promise_run.get_future().get());
}

BOOST_AUTO_TEST_CASE(session_inbound__net_start__port_and_connections__expected)
{
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    set.connect_batch_size = 0;
    set.outbound_connections = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    // Start will return invalid_magic when executed.
    set.inbound_port = 42;
    set.inbound_connections = 1;

    mock_p2p<mock_acceptor_start_fail> net(set);

    std::promise<bool> promise_run;
    const auto run_handler = [&](const code& ec)
    {
        // mock_acceptor configured to return invalid_magic.
        BOOST_REQUIRE_EQUAL(ec, error::invalid_magic);
        promise_run.set_value(true);
    };

    const auto start_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        net.run(run_handler);
    };

    net.start(start_handler);
    BOOST_REQUIRE(promise_run.get_future().get());
}

// stop

BOOST_AUTO_TEST_CASE(session_inbound__stop__started__stopped)
{
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    p2p net(set);
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__stopped__stopped)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session_inbound session(net);

    std::promise<bool> promise;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        promise.set_value(true);
    });

    BOOST_REQUIRE(promise.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

BOOST_AUTO_TEST_CASE(session_inbound__start__acceptor_start_failure__not_accept)
{
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_fail> net(set);
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    // mock_acceptor_start_fail.start returns invalid_magic, so start_accept aborts.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::invalid_magic);
    BOOST_REQUIRE_EQUAL(net.acceptor->port(), set.inbound_port);

    BOOST_REQUIRE(!session->stopped());
    BOOST_REQUIRE(net.acceptor);
    BOOST_REQUIRE(!net.acceptor->stopped());

    // Accept is not invoked (race, but always false).
    BOOST_REQUIRE(!net.acceptor->accepted());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(net.acceptor->stopped());

    // Attach is not invoked.
    BOOST_REQUIRE(!session->attached());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__start__acceptor_started_accept_returns_stopped__not_attach)
{
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_stopped> net(set);
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.acceptor->port(), set.inbound_port);

    BOOST_REQUIRE(!session->stopped());
    BOOST_REQUIRE(net.acceptor);
    BOOST_REQUIRE(!net.acceptor->stopped());

    // Block until accepted.
    BOOST_REQUIRE(session->require_accepted());

    // Accept is invoked, but not reinvoked (race, but always false).
    BOOST_REQUIRE(net.acceptor->accepted());
    BOOST_REQUIRE(!net.acceptor->reaccepted());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(net.acceptor->stopped());

    // Not attached because accept returned stopped.
    BOOST_REQUIRE(!session->attached());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_error__not_attach)
{
    settings set(selection::mainnet);
    set.seeds.clear();
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_success_accept_fail> net(set);
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.acceptor->port(), set.inbound_port);

    BOOST_REQUIRE(!session->stopped());
    BOOST_REQUIRE(net.acceptor);
    BOOST_REQUIRE(!net.acceptor->stopped());

    // Block until accepted.
    BOOST_REQUIRE(session->require_accepted());

    // Accept is invoked, but not reinvoked (race, but always false).
    BOOST_REQUIRE(net.acceptor->accepted());
    BOOST_REQUIRE(!net.acceptor->reaccepted());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(net.acceptor->stopped());

    // Another accept is not generally invoked because stop is set first, but this is a race.
    ////BOOST_REQUIRE(!net.acceptor->reaccepted());

    // Not attached because accept returned error.
    BOOST_REQUIRE(!session->attached());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_success__attach)
{
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_success_accept_success> net(set);

    // Start the network so that it can be stopped.
    std::promise<bool> promise_net_started;
    const auto net_start_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_net_started.set_value(true);
    };

    net.start(net_start_handler);
    BOOST_REQUIRE(promise_net_started.get_future().get());

    // Start the session using the network reference.
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> promise_session_started;
    boost::asio::post(net.strand(), [&]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            promise_session_started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(promise_session_started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.acceptor->port(), set.inbound_port);

    BOOST_REQUIRE(!session->stopped());
    BOOST_REQUIRE(net.acceptor);
    BOOST_REQUIRE(!net.acceptor->stopped());

    // Block until accepted.
    BOOST_REQUIRE(session->require_accepted());

    // Accept is invoked, but not reinvoked (race, but always false).
    BOOST_REQUIRE(net.acceptor->accepted());
    BOOST_REQUIRE(!net.acceptor->reaccepted());

    // Attached on first accept and before stop (blocking).
    BOOST_REQUIRE(session->require_attached());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session->stop();
        BOOST_REQUIRE(session->stopped());
        BOOST_REQUIRE(session->attached());

        session.reset();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(net.acceptor->stopped());

    // Another accept is not generally invoked because stop is set first, but this is a race.
    ////BOOST_REQUIRE(!net.acceptor->reaccepted());

    net.close();
}

// handle_accept:inbound_channel_count
// handle_accept:blacklisted

BOOST_AUTO_TEST_SUITE_END()