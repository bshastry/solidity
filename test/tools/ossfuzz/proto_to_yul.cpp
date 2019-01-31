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

#include <test/tools/ossfuzz/proto_to_yul.h>
#include <test/tools/ossfuzz/yul_proto.pb.h>

#include <ostream>
#include <sstream>

namespace yul_fuzzer {

// Forward decls.
std::ostream &operator<<(std::ostream &os, const BinaryOp &x);
std::ostream &operator<<(std::ostream &os, const StatementSeq &x);

// Proto to C++.
std::ostream &operator<<(std::ostream &os, const Const &x) {
  return os << "(" << x.val() << ")";
}
std::ostream &operator<<(std::ostream &os, const VarRef &x) {
  return os << "a[" << (static_cast<uint32_t>(x.varnum()) % 100) << "]";
}
std::ostream &operator<<(std::ostream &os, const Lvalue &x) {
  return os << x.varref();
}
std::ostream &operator<<(std::ostream &os, const Rvalue &x) {
    if (x.has_varref()) return os << x.varref();
    if (x.has_cons())   return os << x.cons();
    if (x.has_binop())  return os << x.binop();
    return os << "1";
}
std::ostream &operator<<(std::ostream &os, const BinaryOp &x) {
  switch (x.op()) {
    case BinaryOp::PLUS: os << "add(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::MINUS: os << "sub(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::MUL: os << "mul(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::DIV: os << "div(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::MOD: os << "mod(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::XOR: os << "xor(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::AND: os << "and(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::OR: os << "or(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::EQ: os << "eq(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::LT: os << "lt(" << x.left() << "," << x.right() << ")"; break;
    case BinaryOp::GT: os << "gt(" << x.left() << "," << x.right() << ")"; break;
  }
}
std::ostream &operator<<(std::ostream &os, const UnaryOp &x) {
  switch (x.op()) {
    case UnaryOp::NOT: os << "not(" << x.operand() << ")"; break;
  }
}
std::ostream &operator<<(std::ostream &os, const AssignmentStatement &x) {
  return os << x.lvalue() << "=" << x.rvalue() << ";\n";
}
std::ostream &operator<<(std::ostream &os, const IfElse &x) {
  return os << "if (" << x.cond() << "){\n"
            << x.if_body() << "} else { \n"
            << x.else_body() << "}\n";
}
std::ostream &operator<<(std::ostream &os, const While &x) {
  return os << "while (" << x.cond() << "){\n" << x.body() << "}\n";
}
std::ostream &operator<<(std::ostream &os, const Statement &x) {
  if (x.has_assignment()) return os << x.assignment();
  if (x.has_ifelse())     return os << x.ifelse();
  if (x.has_while_loop()) return os << x.while_loop();
  return os << "(void)0;\n";
}
std::ostream &operator<<(std::ostream &os, const StatementSeq &x) {
  for (auto &st : x.statements()) os << st;
  return os;
}
std::ostream &operator<<(std::ostream &os, const Function &x) {
  return os << "void foo(int *a) {\n" << x.statements() << "}\n";
}

// ---------------------------------

std::string FunctionToString(const Function &input) {
  std::ostringstream os;
  os << input;
  return os.str();

}
std::string ProtoToYul(const uint8_t *data, size_t size) {
  Function message;
  if (!message.ParsePartialFromArray(data, size))
    return "#error invalid proto\n";
  return FunctionToString(message);
}

} // namespace yul_fuzzer