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

#include <libevmasm/ConstantOptimiser.h>

#include <string>
#include <sstream>
#include <iostream>

using namespace std;
using namespace dev;
using namespace dev::eth;

namespace {
    void testConstantOptimizer(string const &input) {
        if (!quiet)
            cout << "Testing constant optimizer" << endl;
        vector<u256> numbers;
        stringstream sin(input);

        while (!sin.eof()) {
            h256 data;
            sin.read(reinterpret_cast<char *>(data.data()), 32);
            numbers.push_back(u256(data));
        }
        if (!quiet)
            cout << "Got " << numbers.size() << " inputs:" << endl;

        Assembly assembly;
        for (u256 const &n: numbers) {
            if (!quiet)
                cout << n << endl;
            assembly.append(n);
        }
        for (bool isCreation: {false, true}) {
            for (unsigned runs: {1, 2, 3, 20, 40, 100, 200, 400, 1000}) {
                ConstantOptimisationMethod::optimiseConstants(
                        isCreation,
                        runs,
                        EVMVersion{},
                        assembly,
                        const_cast<AssemblyItems &>(assembly.items())
                );
            }
        }
    }
}