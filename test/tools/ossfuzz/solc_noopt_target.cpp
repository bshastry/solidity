#include <test/tools/fuzzer.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    string input(reinterpret_cast<const char*>(data), size);
    testCompiler(input, /*optimize=*/false, /*quiet=*/true);
    return 0;
}
