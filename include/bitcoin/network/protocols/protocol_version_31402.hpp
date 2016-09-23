/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_VERSION_31402_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_VERSION_31402_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>
#include <bitcoin/network/network_interface.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p_network;

class BCT_API protocol_version_31402
  : public protocol_timer, track<protocol_version_31402>
{
public:
    typedef std::shared_ptr<protocol_version_31402> ptr;

    /**
     * Construct a version protocol instance using configured minimums.
     * @param[in]  network   The network interface.
     * @param[in]  channel   The channel for the protocol.
     */
    protocol_version_31402(network_interface& network, channel::ptr channel);

    /**
     * Construct a version protocol instance.
     * @param[in]  network           The network interface.
     * @param[in]  channel           The channel for the protocol.
     * @param[in]  own_version       This node's maximum version.
     * @param[in]  own_services      This node's advertised services.
     * @param[in]  minimum_version   This required minimum version.
     * @param[in]  minimum_services  This required minimum services.
     */
    protocol_version_31402(network_interface& network, channel::ptr channel,
        uint32_t own_version, uint64_t own_services, uint32_t minimum_version,
        uint64_t minimum_services);
    
    /**
     * Start the protocol.
     * @param[in]  handler  Invoked upon stop or receipt of version and verack.
     */
    virtual void start(event_handler handler);

protected:
    virtual message::version version_factory() const;

    virtual void handle_version_sent(const code& ec);
    virtual void handle_verack_sent(const code& ec);

    virtual bool handle_receive_version(const code& ec,
        version_const_ptr version);
    virtual bool handle_receive_verack(const code& ec, verack_const_ptr);

    network_interface& network_;
    const uint32_t own_version_;
    const uint64_t own_services_;
    const uint32_t minimum_version_;
    const uint64_t minimum_services_;
};

} // namespace network
} // namespace libbitcoin

#endif

