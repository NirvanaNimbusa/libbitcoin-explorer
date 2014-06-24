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
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <stdint.h>
#include <thread>
//#include <sstream>
//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>
#include <bitcoin/bitcoin.hpp>
#include <obelisk/obelisk.hpp>
#include <sx/command/balance.hpp>
#include <sx/dispatch.hpp>
#include <sx/obelisk.hpp>
#include <sx/utility/coin.hpp>
#include <sx/utility/config.hpp>
#include <sx/utility/console.hpp>

using namespace bc;
using namespace sx;
using namespace sx::extensions;

static std::mutex mutex;
static std::condition_variable condition;
static int32_t remaining_count = bc::max_int32;

// TODO: for testability parameterize STDOUT and STDERR.
static void balance_fetched(const payment_address& payaddr,
    const std::error_code& error, const blockchain::history_list& history)
{
    if (error)
    {
        std::cerr << "balance: Failed to fetch history: " << error.message()
            <<std::endl;
        return;
    }

    uint64_t total_recv = 0, balance = 0, pending_balance = 0;
    for (const auto& row: history)
    {
        auto value = row.value;
        BITCOIN_ASSERT(value >= 0);
        total_recv += value;

        // Unconfirmed balance.
        if (row.spend.hash == null_hash)
            pending_balance += value;

        // Confirmed balance.
        if (row.output_height && 
            (row.spend.hash == null_hash || !row.spend_height))
            balance += value;

        BITCOIN_ASSERT(total_recv >= balance);
        BITCOIN_ASSERT(total_recv >= pending_balance);
    }

    std::cout << "Address: " << payaddr.encoded() << std::endl;
    std::cout << "  Paid balance:    " << balance << std::endl;
    std::cout << "  Pending balance: " << pending_balance << std::endl;
    std::cout << "  Total received:  " << total_recv << std::endl;
    std::cout << std::endl;
    std::lock_guard<std::mutex> lock(mutex);
    BITCOIN_ASSERT(remaining_count != bc::max_int32);
    --remaining_count;
    condition.notify_one();
}

// TODO: generalize json serialization.
// TODO: for testability parameterize STDOUT and STDERR.
static void json_balance_fetched(const payment_address& payaddr,
    const std::error_code& error, const blockchain::history_list& history)
{
    if (error)
    {
        std::cerr << "balance: Failed to fetch history: " << error.message()
            <<std::endl;
        return;
    }

    bool is_first = true;
    uint64_t total_recv = 0, balance = 0, pending_balance = 0;
    for (const auto& row: history)
    {
        uint64_t value = row.value;
        BITCOIN_ASSERT(value >= 0);
        total_recv += value;

        // Unconfirmed balance.
        if (row.spend.hash == null_hash)
            pending_balance += value;

        // Confirmed balance.
        if (row.output_height &&
            (row.spend.hash == null_hash || !row.spend_height))
            balance += value;

        BITCOIN_ASSERT(total_recv >= balance);
        BITCOIN_ASSERT(total_recv >= pending_balance);
    }

    // Put commas between each array item in json output.
    if (is_first)
        is_first = false;
    else
        std::cout << "," << std::endl;

    // Actual row data.
    std::cout << "{" << std::endl;
    std::cout << "  \"address\": \"" << payaddr.encoded()
        << "\"," << std::endl;
    std::cout << "  \"paid\":  \"" << balance << "\"," << std::endl;
    std::cout << "  \"pending\":  \"" << pending_balance << "\"," << std::endl;
    std::cout << "  \"received\":  \"" << total_recv << "\"" << std::endl;
    std::cout << "}";
    std::lock_guard<std::mutex> lock(mutex);

    BITCOIN_ASSERT(remaining_count != bc::max_int32);

    --remaining_count;
    condition.notify_one();
    if (remaining_count > 0)
        std::cout << ",";

    std::cout << std::endl;
}

console_result balance::invoke(std::istream& input, std::ostream& output,
    std::ostream& cerr)
{
    // Bound parameters.
    // TODO: improve code generation pluralization.
    auto addresses = get_addresss_argument();
    auto json = get_json_option();

    // TODO: implement support for defaulting a collection ARG to STDIN.
    payaddr_list payaddrs;
    if (addresses.empty())
    {
        payment_address payaddr;
        if (!payaddr.set_encoded(read_stream(input)))
        {
            // TODO: provide address info with error.
            cerr << boost::format(SX_BALANCE_INVALID_ADDRESS) << std::endl;
            return console_result::failure;
        }

        payaddrs.push_back(payaddr);
    }
    else if (!read_addresses(addresses, payaddrs))
    {
        // TODO: provide address info with error.
        cerr << boost::format(SX_BALANCE_INVALID_ADDRESS) << std::endl;
        return console_result::failure;
    }

    OBELISK_FULLNODE(pool, fullnode);

    if (json)
        output << "[" << std::endl;

    for (const payment_address& payaddr: payaddrs)
    {
        if (json)
            fullnode.address.fetch_history(payaddr,
                std::bind(json_balance_fetched, payaddr, 
                    std::placeholders::_1, std::placeholders::_2));
        else
            fullnode.address.fetch_history(payaddr,
                std::bind(balance_fetched, payaddr, 
                    std::placeholders::_1, std::placeholders::_2));
    }

    std::thread update_loop([&fullnode]
    {
        while (true)
        {
            fullnode.update();
            sleep_ms(100);
        }
    });
    update_loop.detach();

    std::unique_lock<std::mutex> lock(mutex);
    remaining_count = static_cast<int>(payaddrs.size());
    while (remaining_count > 0)
    {
        condition.wait(lock);
        BITCOIN_ASSERT(remaining_count >= 0);
    }

    if (json)
        output << "]" << std::endl;

    pool.stop();
    pool.join();
    return console_result::okay;
}

