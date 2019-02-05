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

#include <fstream>

#include <test/tools/ossfuzz/yul_proto.pb.h>
#include <test/tools/fuzzer_common.h>
#include <test/tools/ossfuzz/proto_to_yul.h>
#include <src/libfuzzer/libfuzzer_macro.h>

#include <libsolidity/interface/AssemblyStack.h>

using namespace yul_fuzzer;
using namespace dev::solidity;

DEFINE_BINARY_PROTO_FUZZER(const Function& input)
{
  std::string yul_source = FunctionToString(input);

  if (const char *dump_path = getenv("PROTO_FUZZER_DUMP_PATH"))
  {
    // With libFuzzer binary run this to generate a YUL source file x.yul:
    // PROTO_FUZZER_DUMP_PATH=x.yul ./a.out proto-input
    std::ofstream of(dump_path);
    of.write(yul_source.data(), yul_source.size());
  }

  // AssemblyStack entry point
  AssemblyStack stack(EVMVersion::constantinople(), AssemblyStack::Language::Yul);

  // Parse protobuf mutated YUL code
  if (!stack.parseAndAnalyze("source", yul_source))
    return;

  // Optimize
  // solAssert(analyzeParsed(), "Invalid source code after optimization.");
  stack.optimize();
}