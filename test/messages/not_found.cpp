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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(not_found_tests)

using namespace bc::network::messages;

BOOST_AUTO_TEST_CASE(not_found__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(not_found::command, "notfound");
    BOOST_REQUIRE(not_found::id == identifier::not_found);
    BOOST_REQUIRE_EQUAL(not_found::version_minimum, level::bip37);
    BOOST_REQUIRE_EQUAL(not_found::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(not_found__size__default__expected)
{
    constexpr auto expected = variable_size(zero);

    // passed to base class.
    BOOST_REQUIRE_EQUAL(not_found{}.size(level::canonical), expected);
}

BOOST_AUTO_TEST_SUITE_END()
