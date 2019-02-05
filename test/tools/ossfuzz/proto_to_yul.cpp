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

	// Proto to yul.
	// Constant is a signed 32-bit integer
	std::ostream &operator<<(std::ostream &os, const Const &x)
	{
		return os << "(" << x.val() << ")";
	}

	std::ostream &operator<<(std::ostream &os, const VarRef &x)
	{
		return os << "a[" << (static_cast<uint32_t>(x.varnum()) % 100) << "]";
	}

	std::ostream &operator<<(std::ostream &os, const Lvalue &x)
	{
		return os << x.varref();
	}

	std::ostream &operator<<(std::ostream &os, const Rvalue &x)
	{
		if (x.has_varref())
			return os << x.varref();
		if (x.has_cons())
			return os << x.cons();
		if (x.has_binop())
			return os << x.binop();
		return os << "1";
	}

	std::ostream &operator<<(std::ostream &os, const BinaryOp &x)
	{
		switch (x.op()) {
			case BinaryOp::ADD:
				os << "add(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::SUB:
				os << "sub(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::MUL:
				os << "mul(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::DIV:
				os << "div(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::MOD:
				os << "mod(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::XOR:
				os << "xor(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::AND:
				os << "and(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::OR:
				os << "or(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::EQ:
				os << "eq(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::LT:
				os << "lt(" << x.left() << "," << x.right() << ")";
				break;
			case BinaryOp::GT:
				os << "gt(" << x.left() << "," << x.right() << ")";
				break;
		}
		return os;
	}

	std::ostream &operator<<(std::ostream &os, const VarDecl &x)
	{
		os << "let x_" << x.id() << " := " << x.expr() << "\n";
		return os;
	}

	std::ostream &operator<<(std::ostream &os, const TypedVarDecl &x)
	{
		os << "let x_" << x.id();
		switch (x.type())
		{
			case TypedVarDecl::BOOL:
				os << ": bool := " << x.expr() << " : bool\n";
				break;
			case TypedVarDecl::S8:
				os << ": s8 := " << x.expr() << " : s8\n";
				break;
			case TypedVarDecl::S32:
				os << ": s32 := " << x.expr() << " : s32\n";
				break;
			case TypedVarDecl::S64:
				os << ": s64 := " << x.expr() << " : s64\n";
				break;
			case TypedVarDecl::S128:
				os << ": s128 := " << x.expr() << " : s128\n";
				break;
			case TypedVarDecl::S256:
				os << ": s256 := " << x.expr() << " : s256\n";
				break;
			case TypedVarDecl::U8:
				os << ": u8 := " << x.expr() << " : u8\n";
				break;
			case TypedVarDecl::U32:
				os << ": u32 := " << x.expr() << " : u32\n";
				break;
			case TypedVarDecl::U64:
				os << ": u64 := " << x.expr() << " : u64\n";
				break;
			case TypedVarDecl::U128:
				os << ": u128 := " << x.expr() << " : u128\n";
				break;
			case TypedVarDecl::U256:
				os << ": u256 := " << x.expr() << " : u256\n";
				break;
		}
		return os;
	}

	std::ostream &operator<<(std::ostream &os, const UnaryOp &x)
	{
		switch (x.op()) {
			case UnaryOp::NOT:
				os << "not(" << x.operand() << ")";
				break;
			case UnaryOp::MLOAD:
				os << "mload(" << x.operand() << ")";
				break;
			case UnaryOp::SLOAD:
				os << "sload(" << x.operand() << ")";
				break;
		}
		return os;
	}

	std::ostream &operator<<(std::ostream &os, const AssignmentStatement &x)
	{
		return os << x.lvalue() << " := " << x.rvalue() << "\n";
	}

	std::ostream &operator<<(std::ostream &os, const IfStmt &x)
	{
		return os << "if " << x.cond() << " {\n"
		          << x.if_body() << "}\n";
	}

	std::ostream &operator<<(std::ostream &os, const StoreFunc &x)
	{
		switch (x.st())
		{
			case StoreFunc::MEMORY:
				os << "mstore(" << x.loc() << ", " << x.val() << ")\n";
				break;
			case StoreFunc::STORAGE:
				os << "sstore(" << x.loc() << ", " << x.val() << ")\n";
				break;
		}
		return os;
	}

	std::ostream &operator<<(std::ostream &os, const Statement &x)
	{
		if (x.has_decl()) return os << x.decl();
		if (x.has_assignment()) return os << x.assignment();
		if (x.has_ifstmt()) return os << x.ifstmt();
		if (x.has_storage_func()) return os << x.storage_func();
		return os << "\n";
	}

	std::ostream &operator<<(std::ostream &os, const StatementSeq &x)
	{
		for (auto &st : x.statements()) os << st;
		return os;
	}

	std::ostream &operator<<(std::ostream &os, const Function &x)
	{
		os << "{\n"
		<< "let a,b := foo(calldataload(0),calldataload(32))\n"
		<< "sstore(0, a)\n"
		<< "sstore(32, b)\n"
		<< "function foo(bar, baz) -> ret1, ret2\n"
		<< "{\n"
		<< x.statements()
		<< "}\n"
		<< "}\n";
	}

// ---------------------------------

	std::string FunctionToString(const Function &input)
	{
		std::ostringstream os;
		os << input;
		return os.str();
	}

	std::string ProtoToYul(const uint8_t *data, size_t size)
	{
		Function message;
		if (!message.ParsePartialFromArray(data, size))
			return "#error invalid proto\n";
		return FunctionToString(message);
	}

} // namespace yul_fuzzer
