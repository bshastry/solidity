#include <test/tools/fuzzer_common.h>

using namespace std;

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* _data, size_t _size)
{
    string input(reinterpret_cast<char const*>(_data), _size);
    FuzzerUtil::testCompiler(input, /*optimize=*/false, /*quiet=*/true);
    return 0;
}
