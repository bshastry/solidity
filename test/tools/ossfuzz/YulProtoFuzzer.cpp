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

#include "sol_proto.pb.h"
#include <test/tools/fuzzer_common.h>
#include "proto-to-cxx/proto_to_cxx.h"
//#include "fuzzer-initialize/fuzzer_initialize.h"
#include "src/libfuzzer/libfuzzer_macro.h"

DEFINE_BINARY_PROTO_FUZZER(const Function& input) {
  auto S = FunctionToString(input);
  FuzzerUtil::testCompiler(S, true, true);
}
