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
#include <bitcoin/network/net/pump.hpp>

#include <memory>
#include <string>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace bc::system::messages;

#define SUBSCRIBER(name) name##_subscriber_
#define SUBSCRIBER_TYPE(name) name##_subscriber

#define MAKE_SUBSCRIBER(name) \
    SUBSCRIBER(name)(std::make_shared<SUBSCRIBER_TYPE(name)>(strand_))

#define CASE_NOTIFY(name) \
    case identifier::name: \
        return do_notify<messages::name>(SUBSCRIBER(name), version, reader)

#define STOP_SUBSCRIBER(name) \
    SUBSCRIBER(name)->stop(ec, nullptr)

pump::pump(asio::strand& strand)
  : strand_(strand),
    ////address_subscriber_(std::make_shared<address_subscriber>(strand_)),
    MAKE_SUBSCRIBER(address),
    MAKE_SUBSCRIBER(alert),
    MAKE_SUBSCRIBER(block),
    MAKE_SUBSCRIBER(block_transactions),
    MAKE_SUBSCRIBER(compact_block),
    MAKE_SUBSCRIBER(compact_filter),
    MAKE_SUBSCRIBER(compact_filter_checkpoint),
    MAKE_SUBSCRIBER(compact_filter_headers),
    MAKE_SUBSCRIBER(fee_filter),
    MAKE_SUBSCRIBER(filter_add),
    MAKE_SUBSCRIBER(filter_clear),
    MAKE_SUBSCRIBER(filter_load),
    MAKE_SUBSCRIBER(get_address),
    MAKE_SUBSCRIBER(get_blocks),
    MAKE_SUBSCRIBER(get_block_transactions),
    MAKE_SUBSCRIBER(get_compact_filter_checkpoint),
    MAKE_SUBSCRIBER(get_compact_filter_headers),
    MAKE_SUBSCRIBER(get_compact_filters),
    MAKE_SUBSCRIBER(get_data),
    MAKE_SUBSCRIBER(get_headers),
    MAKE_SUBSCRIBER(headers),
    MAKE_SUBSCRIBER(inventory),
    MAKE_SUBSCRIBER(memory_pool),
    MAKE_SUBSCRIBER(merkle_block),
    MAKE_SUBSCRIBER(not_found),
    MAKE_SUBSCRIBER(ping),
    MAKE_SUBSCRIBER(pong),
    MAKE_SUBSCRIBER(reject),
    MAKE_SUBSCRIBER(send_compact),
    MAKE_SUBSCRIBER(send_headers),
    MAKE_SUBSCRIBER(transaction),
    MAKE_SUBSCRIBER(verack),
    MAKE_SUBSCRIBER(version)
{
}

code pump::notify(identifier id, uint32_t version, reader& reader) const
{
    switch (id)
    {
        ////case identifier::address:
        ////    return do_notify<messages::address>(
        ////        address_subscriber_, version, reader);
        CASE_NOTIFY(address);
        CASE_NOTIFY(alert);
        CASE_NOTIFY(block);
        CASE_NOTIFY(block_transactions);
        CASE_NOTIFY(compact_block);
        CASE_NOTIFY(compact_filter);
        CASE_NOTIFY(compact_filter_checkpoint);
        CASE_NOTIFY(compact_filter_headers);
        CASE_NOTIFY(fee_filter);
        CASE_NOTIFY(filter_add);
        CASE_NOTIFY(filter_clear);
        CASE_NOTIFY(filter_load);
        CASE_NOTIFY(get_address);
        CASE_NOTIFY(get_blocks);
        CASE_NOTIFY(get_block_transactions);
        CASE_NOTIFY(get_compact_filter_checkpoint);
        CASE_NOTIFY(get_compact_filter_headers);
        CASE_NOTIFY(get_compact_filters);
        CASE_NOTIFY(get_data);
        CASE_NOTIFY(get_headers);
        CASE_NOTIFY(headers);
        CASE_NOTIFY(inventory);
        CASE_NOTIFY(memory_pool);
        CASE_NOTIFY(merkle_block);
        CASE_NOTIFY(not_found);
        CASE_NOTIFY(ping);
        CASE_NOTIFY(pong);
        CASE_NOTIFY(reject);
        CASE_NOTIFY(send_compact);
        CASE_NOTIFY(send_headers);
        CASE_NOTIFY(transaction);
        CASE_NOTIFY(verack);
        CASE_NOTIFY(version);
        case identifier::unknown:
        default:
            return error::unknown_message;
    }
}

void pump::stop(const code& ec)
{
    ////address_subscriber_->stop(ec, nullptr);
    STOP_SUBSCRIBER(address);
    STOP_SUBSCRIBER(alert);
    STOP_SUBSCRIBER(block);
    STOP_SUBSCRIBER(block_transactions);
    STOP_SUBSCRIBER(compact_block);
    STOP_SUBSCRIBER(compact_filter);
    STOP_SUBSCRIBER(compact_filter_checkpoint);
    STOP_SUBSCRIBER(compact_filter_headers);
    STOP_SUBSCRIBER(fee_filter);
    STOP_SUBSCRIBER(filter_add);
    STOP_SUBSCRIBER(filter_clear);
    STOP_SUBSCRIBER(filter_load);
    STOP_SUBSCRIBER(get_address);
    STOP_SUBSCRIBER(get_blocks);
    STOP_SUBSCRIBER(get_block_transactions);
    STOP_SUBSCRIBER(get_compact_filter_checkpoint);
    STOP_SUBSCRIBER(get_compact_filter_headers);
    STOP_SUBSCRIBER(get_compact_filters);
    STOP_SUBSCRIBER(get_data);
    STOP_SUBSCRIBER(get_headers);
    STOP_SUBSCRIBER(headers);
    STOP_SUBSCRIBER(inventory);
    STOP_SUBSCRIBER(memory_pool);
    STOP_SUBSCRIBER(merkle_block);
    STOP_SUBSCRIBER(not_found);
    STOP_SUBSCRIBER(ping);
    STOP_SUBSCRIBER(pong);
    STOP_SUBSCRIBER(reject);
    STOP_SUBSCRIBER(send_compact);
    STOP_SUBSCRIBER(send_headers);
    STOP_SUBSCRIBER(transaction);
    STOP_SUBSCRIBER(verack);
    STOP_SUBSCRIBER(version);
}

#undef SUBSCRIBER
#undef SUBSCRIBER_TYPE
#undef MAKE_SUBSCRIBER
#undef CASE_NOTIFY
#undef STOP_SUBSCRIBER

} // namespace network
} // namespace libbitcoin