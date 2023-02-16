/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(tracker_tests)

// Started log with tracker is unsafe unless blocked on write completion.
// As the object is destroyed a job is created on an independent thread.

class tracked
  : tracker<tracked>
{
public:
    tracked(const logger& log) NOEXCEPT
      : tracker<tracked>(log)
    {
    }

    bool method() const NOEXCEPT
    {
        return true;
    };
};

#if !defined(NDEBUG)
BOOST_AUTO_TEST_CASE(tracker__construct1__guarded__safe_expected_messages)
{
    logger log{};
    std::promise<code> wait{};
    auto count = zero;
    log.subscribe([&](const code& ec, const std::string& message)
    {
        if (is_zero(count++))
        {
            const auto expected = std::string{ typeid(tracked).name() } + "(1)\n";
            BOOST_REQUIRE_EQUAL(message, expected);
        }
        else
        {
            const auto expected = std::string{ typeid(tracked).name() } + "(0)~\n";
            BOOST_REQUIRE_EQUAL(message, expected);

            wait.set_value(ec);
            return false;
        }

        return true;
    });

    auto foo = system::to_shared<tracked>(log);
    BOOST_REQUIRE(foo->method());

    foo.reset();
    BOOST_REQUIRE_EQUAL(wait.get_future().get(), error::success);
}
#endif

BOOST_AUTO_TEST_CASE(tracker__construct2__true__stopped)
{
    // The parameter value is unused.
    const logger log{ true };
    tracked foo{ log };
    BOOST_REQUIRE(foo.method());
}

BOOST_AUTO_TEST_CASE(tracker__construct2__false__stopped)
{
    // The parameter value is unused.
    const logger log{ false };
    tracked foo{ log };
    BOOST_REQUIRE(foo.method());
}

BOOST_AUTO_TEST_CASE(tracker__stop__always__safe)
{
    logger log{};
    log.stop();
    tracked foo{ log };
    BOOST_REQUIRE(foo.method());
}

BOOST_AUTO_TEST_SUITE_END()
