/**
 * Copyright (c) 2011-2014 sx developers (see AUTHORS)
 *
 * This file is part of sx.
 *
 * sx is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SX_CLIENT_HPP
#define SX_CLIENT_HPP

#include <sx/utility/config.hpp>
#include <bitcoin/bitcoin.hpp>
#include <obelisk/obelisk.hpp>

/* NOTE: don't declare 'using namespace foo' in headers. */

namespace sx {
    
/**
 * The default number of threads initialized by OBELISK_FULLNODE.
 */
const size_t default_threadpool_size = 1;

/**
 * The default polling period initialized by OBELISK_FULLNODE.
 */
const uint32_t default_poll_period_ms = 100;

/**
 * This prevents code repetition while retaining stack-based allocation.
 * A single thread is allocated in the pool and service settings are loaded
 * from config.
 *
 * TODO: this is not testable as is - move into function.
 */
#define OBELISK_FULLNODE(__pool__, __fullnode__) \
    threadpool __pool__(default_threadpool_size); \
    obelisk::fullnode_interface __fullnode__( \
        __pool__, \
        get_obelisk_service_setting(), \
        get_obelisk_client_certificate_setting().generic_string(), \
        get_obelisk_server_public_key_setting())

/**
 * Poll obelisk for changes until stopped.
 *
 * @param[in]  fullnode   An instance of the obelisk full node interface.
 * @param[in]  pool       The threadpool used by fullnode to poll.
 * @param[in]  stopped    A flag that signals cessation of polling.
 * @param[in]  period_ms  The polling period in milliseconds, defaults to 100.
 */
void poll(obelisk::fullnode_interface& fullnode, bc::threadpool& pool,
    bool& stopped, uint32_t period_ms=default_poll_period_ms);

} // sx

#endif