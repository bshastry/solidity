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
#include <libevmasm/ConstantOptimiser.h>

#include <libdevcore/JSON.h>

#include <string>
#include <sstream>
#include <iostream>

using namespace std;
using namespace dev;
using namespace dev::eth;

struct FuzzerUtil
{
    static string contains(string const& _haystack, vector<string> const& _needles);
    static void runCompiler(string _input);
    static void testCompiler(string const& _input, bool _optimize, bool quiet);
    static void testConstantOptimizer(string const& _input, bool _quiet);
    static void testStandardCompiler(string const& _input, bool _quiet);
};