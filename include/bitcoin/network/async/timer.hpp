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
#ifndef LIBBITCOIN_NETWORK_ASYNC_TIMER_HPP
#define LIBBITCOIN_NETWORK_ASYNC_TIMER_HPP

#include <chrono>
#include <utility>
#include <bitcoin/network/async/time.hpp>

namespace libbitcoin {
namespace network {

// From: github.com/picanumber/bureaucrat/blob/master/time_lapse.h
// boost::timer::auto_cpu_timer requires the boost timer lib dependency.

/// Class to measure the execution time of a callable.
template <typename Time=milliseconds, class Clock=steady_clock>
struct timer
{
    /// Returns the duration (in chrono's type system) of the elapsed time.
    template <typename Function, typename... Args>
    static Time duration(Function func, Args&&... args)
    {
        auto start = Clock::now();
        func(std::forward<Args>(args)...);
        return std::chrono::duration_cast<Time>(Clock::now() - start);
    }

    /// Returns the quantity (count) of the elapsed time as TimeT units.
    template <typename Function, typename ...Args>
    static typename Time::rep execution(Function func, Args&&... args)
    {
        return duration(func, std::forward<Args>(args)...).count();
    }
};

} // namespace network
} // namespace libbitcoin

#endif