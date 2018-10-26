#include <libdevcore/CommonIO.h>
#include <libevmasm/Assembly.h>
#include <libevmasm/ConstantOptimiser.h>

#include <string>
#include <sstream>
#include <iostream>

using namespace std;
using namespace dev;
using namespace dev::eth;

namespace
{

    bool quiet = true;

    void testConstantOptimizer(string const& input)
    {
        if (!quiet)
            cout << "Testing constant optimizer" << endl;
        vector<u256> numbers;
        stringstream sin(input);

        while (!sin.eof())
        {
            h256 data;
            sin.read(reinterpret_cast<char*>(data.data()), 32);
            numbers.push_back(u256(data));
        }
        if (!quiet)
            cout << "Got " << numbers.size() << " inputs:" << endl;

        Assembly assembly;
        for (u256 const& n: numbers)
        {
            if (!quiet)
                cout << n << endl;
            assembly.append(n);
        }
        for (bool isCreation: {false, true})
        {
            for (unsigned runs: {1, 2, 3, 20, 40, 100, 200, 400, 1000})
            {
                ConstantOptimisationMethod::optimiseConstants(
                    isCreation,
                    runs,
                    EVMVersion{},
                    assembly,
                    const_cast<AssemblyItems&>(assembly.items())
                );
            }
        }
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    string input(reinterpret_cast<const char*>(data), size);

	testConstantOptimizer(input);
    return 0;
}
