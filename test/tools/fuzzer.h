/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libdevcore/CommonIO.h>
#include <libevmasm/Assembly.h>
#include <libsolc/libsolc.h>

#include <libdevcore/JSON.h>

#include <boost/program_options.hpp>

#include <string>
#include <sstream>
#include <iostream>

using namespace std;
using namespace dev;
using namespace dev::eth;
namespace po = boost::program_options;

namespace
{

    bool quiet = false;

    string contains(string const& _haystack, vector<string> const& _needles)
    {
        for (string const& needle: _needles)
            if (_haystack.find(needle) != string::npos)
                return needle;
        return "";
    }

    void runCompiler(string input)
    {
        string outputString(solidity_compile(input.c_str(), nullptr));
        Json::Value output;
        if (!jsonParseStrict(outputString, output))
        {
            cout << "Compiler produced invalid JSON output." << endl;
            abort();
        }
        if (output.isMember("errors"))
            for (auto const& error: output["errors"])
            {
                string invalid = contains(error["type"].asString(), vector<string>{
                        "Exception",
                        "InternalCompilerError"
                });
                if (!invalid.empty())
                {
                    cout << "Invalid error: \"" << error["type"].asString() << "\"" << endl;
                    abort();
                }
            }
    }

    void testStandardCompiler(string const& input)
    {
        if (!quiet)
            cout << "Testing compiler via JSON interface." << endl;

        runCompiler(input);
    }

    void testCompiler(string const& input, bool optimize)
    {
        if (!quiet)
            cout << "Testing compiler " << (optimize ? "with" : "without") << " optimizer." << endl;

        Json::Value config = Json::objectValue;
        config["language"] = "Solidity";
        config["sources"] = Json::objectValue;
        config["sources"][""] = Json::objectValue;
        config["sources"][""]["content"] = input;
        config["settings"] = Json::objectValue;
        config["settings"]["optimizer"] = Json::objectValue;
        config["settings"]["optimizer"]["enabled"] = optimize;
        config["settings"]["optimizer"]["runs"] = 200;

        // Enable all SourceUnit-level outputs.
        config["settings"]["outputSelection"]["*"][""][0] = "*";
        // Enable all Contract-level outputs.
        config["settings"]["outputSelection"]["*"]["*"][0] = "*";

        runCompiler(jsonCompactPrint(config));
    }

}