/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include "../../test.hpp"

////#if defined(HAVE_SLOW_TESTS)

BOOST_AUTO_TEST_SUITE(rpc_body_reader_tests)

using namespace network::http;
using namespace network::rpc;
using value = boost::json::value;

BOOST_AUTO_TEST_CASE(rpc_body_reader__construct1__default__null_model_terminated)
{
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    BOOST_REQUIRE(body.model.is_null());
    BOOST_REQUIRE(body.message.jsonrpc == version::undefined);
    BOOST_REQUIRE(!body.message.params.has_value());
    BOOST_REQUIRE(!body.message.id.has_value());
    BOOST_REQUIRE(body.message.method.empty());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__construct2__default__null_model_non_terminated)
{
    request_header header{};
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(header, body);
    BOOST_REQUIRE(body.model.is_null());
    BOOST_REQUIRE(body.message.jsonrpc == version::undefined);
    BOOST_REQUIRE(!body.message.params.has_value());
    BOOST_REQUIRE(!body.message.id.has_value());
    BOOST_REQUIRE(body.message.method.empty());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__init__simple_request__success)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_non_terminated__success_expected_consumed)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_terminated_with_newline__success_expected_consumed_including_newline)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_terminated_without_newline__consumed_not_done)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!reader.done());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_non_terminated__success_expected_request_model_cleared)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.message.jsonrpc == version::v2);
    BOOST_REQUIRE(body.message.id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 1);
    BOOST_REQUIRE_EQUAL(body.message.method, "test");
    BOOST_REQUIRE(body.model.is_null());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_terminated_with_newline__success_expected_request_model_cleared)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.message.jsonrpc == version::v2);
    BOOST_REQUIRE(body.message.id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 1);
    BOOST_REQUIRE_EQUAL(body.message.method, "test");
    BOOST_REQUIRE(body.model.is_null());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_terminated_without_newline__need_more_not_done)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(ec == error::http_error_t::need_more);
    BOOST_REQUIRE(!reader.done());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__over_length__body_limit)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(10, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(ec == error::http_error_t::body_limit);
}

BOOST_AUTO_TEST_SUITE_END()

////#endif // HAVE_SLOW_TESTS
