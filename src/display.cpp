/**
 * Copyright (c) 2011-2014 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin-explorer.
 *
 * libbitcoin-explorer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
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

#include <bitcoin/explorer/display.hpp>

#include <iostream>
#include <boost/format.hpp>
#include <bitcoin/explorer/dispatch.hpp>
#include <bitcoin/explorer/generated.hpp>
#include <bitcoin/explorer/utility/utility.hpp>

namespace libbitcoin {
namespace explorer {

#define BX_HOME_PAGE_URL "https://github.com/libbitcoin/libbitcoin-explorer"

void display_command_names(std::ostream& stream)
{
    const auto func = [&stream](std::shared_ptr<command> explorer_command)
    {
        BITCOIN_ASSERT(explorer_command != nullptr);
        if (!explorer_command->obsolete())
            stream << explorer_command->name() << std::endl;
    };

    broadcast(func);
}

void display_invalid_command(std::ostream& stream, const std::string& command,
    const std::string& superseding)
{
    if (superseding.empty())
        stream << format(BX_INVALID_COMMAND) % command;
    else
        stream << format(BX_DEPRECATED_COMMAND) % command % superseding;

    stream << std::endl;
}

void display_invalid_parameter(std::ostream& stream, 
    const std::string& message)
{
    stream << format(BX_INVALID_PARAMETER) % message << std::endl;
}

void display_unexpected_exception(std::ostream& stream,
    const std::string& message)
{
    stream << format(BX_UNEXPECTED_EXCEPTION) % message << std::endl;
}

void display_usage(std::ostream& stream)
{
    stream 
        << std::endl << BX_COMMAND_USAGE << std::endl
        << std::endl << BX_COMMANDS_HEADER << std::endl
        << std::endl;

    display_command_names(stream);

    stream
        << std::endl << BX_COMMANDS_HOME_PAGE << std::endl
        << std::endl << BX_HOME_PAGE_URL << std::endl;
}

} // namespace explorer
} // namespace libbitcoin