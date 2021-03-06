/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation.  You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
// rule_threshold.cc author Josh Rosenbaum <jrosenba@cisco.com>

#include <sstream>
#include <vector>

#include "conversion_state.h"
#include "helpers/converter.h"
#include "rule_states/rule_api.h"
#include "helpers/s2l_util.h"

namespace rules
{

namespace {


class React : public ConversionState
{
public:
    React(Converter& c) : ConversionState(c) {};
    virtual ~React() {};
    virtual bool convert(std::istringstream& data);
};

} // namespace

bool React::convert(std::istringstream& data_stream)
{
    std::string args;
    std::string tmp;
    std::istringstream::off_type pos = data_stream.tellg();

    args = util::get_rule_option_args(data_stream);

    // if there are no arguments, the option had a colon before a semicolon.
    // we are therefore done with this rule.
    if (!args.empty())
    {
        // a colon will have been parsed when retrieving the keyword.
        // Therefore, if a colon is present, we are in the next rule option.
        if (args.find(":") != std::string::npos)
        {
            data_stream.clear();
            data_stream.seekg(pos);
        }
        else
        {
            // since we still can't be sure if we passed the resp buffer,
            // check the next option and ensure it matches
            std::istringstream arg_stream(args);
            util::get_string(arg_stream, tmp, ",");

            if (!tmp.compare("msg") ||
                !tmp.compare("warn") ||
                !tmp.compare("block") ||
                !tmp.compare(0, 5, "proxy"))
            {
                // Now that we have confirmed this is a valid option, parse it!!
                table_api.open_table("react");
                bool find_msg = false;

                do
                {
                    if (!tmp.compare("warn"))
                        table_api.add_deleted_comment("warn");

                    else if (!tmp.compare("block"))
                        table_api.add_deleted_comment("block");

                    else if (!tmp.compare(0, 5, "proxy"))
                        table_api.add_deleted_comment(tmp);

                    else if (!tmp.compare("msg"))
                        find_msg = true;

                    else
                        rule_api.bad_rule(data_stream, "resp: " + tmp);

                } while (util::get_string(arg_stream, tmp, ","));


                // get this rule's msg
                if (find_msg)
                {
                    std::string msg = util::rule_option_find_val(data_stream, "msg");

                    if (!msg.empty())
                    {
                        if (msg.front() == '"' && msg.back() == '"')
                        {
                            msg.erase(msg.begin());
                            msg.pop_back();
                        }

                        table_api.add_option("msg", msg);
                    }
                }
                table_api.close_table(); // "react"
            }
            else
            {
                data_stream.clear();
                data_stream.seekg(pos);
            }


        }
    }

    // create this table to ensure reject is instatiated
    table_api.open_table("react");
    table_api.close_table();

    // Finally, update the rule type
    rule_api.update_rule_action("react");
    return set_next_rule_state(data_stream);
}

/**************************
 *******  A P I ***********
 **************************/


static ConversionState* ctor(Converter& c)
{ return new React(c); }

static const ConvertMap rule_react =
{
    "react",
    ctor,
};

const ConvertMap* react_map = &rule_react;

} // namespace rules
