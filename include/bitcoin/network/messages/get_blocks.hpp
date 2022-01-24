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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_GET_BLOCKS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_GET_BLOCKS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

struct BCT_API get_blocks
{
    typedef std::vector<size_t> indexes;
    typedef std::shared_ptr<const get_blocks> ptr;

    static const identifier id;
    static const std::string command;
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;

    static size_t locator_size(size_t top);
    static indexes locator_heights(size_t top);

    static get_blocks deserialize(uint32_t version, system::reader& source);
    void serialize(uint32_t version, system::writer& sink) const;
    size_t size(uint32_t version) const;

    ////uint32_t protocol_version;
    system::hash_list start_hashes;
    system::hash_digest stop_hash;
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif