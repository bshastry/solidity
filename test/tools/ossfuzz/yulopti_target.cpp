#include <test/tools/yulopti.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    string input(reinterpret_cast<const char*>(data), size);
    YulOpti{}.runNonInteractive(input);
    return 0;
}