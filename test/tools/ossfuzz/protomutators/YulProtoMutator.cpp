#include <test/tools/ossfuzz/protomutators/YulProtoMutator.h>

#include <libyul/Exceptions.h>

#include <src/text_format.h>

using namespace solidity::yul::test::yul_fuzzer;

using namespace protobuf_mutator;

using namespace std;

template <typename Proto>
using YPR = YulProtoCBRegistration<Proto>;
using YPM = YulProtoMutator;
using PrintChanges = YPM::PrintChanges;

MutationInfo::MutationInfo(ProtobufMessage const* _message, string const& _info):
	ScopeGuard([&]{ exitInfo(); }), m_protobufMsg(_message)
{
	writeLine("----------------------------------");
	writeLine("YULMUTATOR: " + _info);
	writeLine("Before");
	writeLine(SaveMessageAsText(*m_protobufMsg));

}

void MutationInfo::exitInfo()
{
	writeLine("After");
	writeLine(SaveMessageAsText(*m_protobufMsg));
}

template <typename T>
void YulProtoMutator::functionWrapper(
	CustomFuzzMutator<T> const& _callback,
	T* _message,
	unsigned _seed,
	unsigned _period,
	string const& _info,
	PrintChanges _printChanges)
{
	YulRandomNumGenerator random(_seed);

	if (_seed % _period == 0)
	{
		if (_printChanges == PrintChanges::Yes)
		{
			MutationInfo m{_message, _info};
			_callback(_message, random);
		}
		else
			_callback(_message, random);
	}
}

// Add idempotent OR
static YPR<Expression> idempotentOr(
	[](Expression* _message, unsigned _seed)
	{
		YPM::functionWrapper<Expression>(
			[](Expression* _message, YulRandomNumGenerator& _rand)
			{
				if (_message->has_binop() && _message->binop().op() == BinaryOp::OR)
				{
					// x
					auto leftOperand = new Expression();
					leftOperand->CopyFrom(_message->binop().left());
					// y
					auto rightOperand = new Expression();
					rightOperand->CopyFrom(_message->binop().right());
					auto binop = new BinaryOp();
					binop->set_op(BinaryOp::OR);
					auto orOp = new Expression();
					// or(x, y)
					orOp->set_allocated_binop(_message->release_binop());
					switch (_rand() % 4)
					{
					// or(x, y) -> or(or(x, y), x)
					case 0:
					{
						// or(or(x, y), x)
						binop->set_allocated_left(orOp);
						binop->set_allocated_right(leftOperand);
						break;
					}
					// or(x, y) -> or(x, or(x, y))
					case 1:
					{
						// or(x, or(x, y))
						binop->set_allocated_left(leftOperand);
						binop->set_allocated_right(orOp);
						break;
					}
					// or(x, y) -> or(or(x, y), y)
					case 2:
					{
						// or(or(x, y), y)
						binop->set_allocated_left(orOp);
						binop->set_allocated_right(rightOperand);
						break;
					}
					// or(x, y) -> or(y, or(x, y))
					case 3:
					{
						// or(y, or(x, y))
						binop->set_allocated_left(rightOperand);
						binop->set_allocated_right(orOp);
						break;
					}
					}
					_message->set_allocated_binop(binop);
				}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add idempotent OR expression"
		);
	}
);

// Add idempotent OR
static YPR<Expression> idempotentAnd(
	[](Expression* _message, unsigned _seed)
	{
	  YPM::functionWrapper<Expression>(
		  [](Expression* _message, YulRandomNumGenerator& _rand)
		  {
			if (_message->has_binop() && _message->binop().op() == BinaryOp::AND)
			{
				// x
				auto leftOperand = new Expression();
				leftOperand->CopyFrom(_message->binop().left());
				// y
				auto rightOperand = new Expression();
				rightOperand->CopyFrom(_message->binop().right());
				auto binop = new BinaryOp();
				binop->set_op(BinaryOp::AND);
				auto andOp = new Expression();
				// and(x, y)
				andOp->set_allocated_binop(_message->release_binop());
				switch (_rand() % 4)
				{
				// and(x, y) -> and(and(x, y), x)
				case 0:
				{
					// and(and(x, y), x)
					binop->set_allocated_left(andOp);
					binop->set_allocated_right(leftOperand);
					break;
				}
				// and(x, y) -> and(x, and(x, y))
				case 1:
				{
					// and(x, and(x, y))
					binop->set_allocated_left(leftOperand);
					binop->set_allocated_right(andOp);
					break;
				}
				// and(x, y) -> and(and(x, y), y)
				case 2:
				{
					// and(and(x, y), y)
					binop->set_allocated_left(andOp);
					binop->set_allocated_right(rightOperand);
					break;
				}
				// and(x, y) -> and(y, and(x, y))
				case 3:
				{
					// and(y, and(x, y))
					binop->set_allocated_left(rightOperand);
					binop->set_allocated_right(andOp);
					break;
				}
				}
				_message->set_allocated_binop(binop);
			}
		  },
		  _message,
		  _seed,
		  YPM::s_highIP,
		  "Add idempotent AND expression"
	  );
	}
);

// Add assignment to m/s/calldataload(0)
static YPR<AssignmentStatement> assignLoadZero(
	[](AssignmentStatement* _message, unsigned _seed)
	{
		YPM::functionWrapper<AssignmentStatement>(
			[](AssignmentStatement* _message, YulRandomNumGenerator& _rand)
			{
				_message->clear_expr();
				_message->set_allocated_expr(YPM::loadFromZero(_rand));
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Assign load from zero"
		);
	}
);

static YPR<Expression> mutateExpr(
	[](Expression* _message, unsigned _seed)
	{
		YPM::functionWrapper<Expression>(
			[](Expression* _message, YulRandomNumGenerator& _rand)
			{
				YPM::clearExpr(_message);
				switch (_rand() % 5)
				{
				case 0:
				{
					auto tmp = YPM::binopExpression(_rand);
					_message->CopyFrom(*tmp);
					delete tmp;
					break;
				}
				case 1:
				{
					auto tmp = YPM::refExpression(_rand);
					_message->CopyFrom(*tmp);
					delete tmp;
					break;
				}
				case 2:
				{
					auto tmp = YPM::litExpression(_rand);
					_message->CopyFrom(*tmp);
					delete tmp;
					break;
				}
				case 3:
				{
					auto tmp = YPM::loadExpression(_rand);
					_message->CopyFrom(*tmp);
					delete tmp;
					break;
				}
				case 4:
				{
					auto tmp = YPM::loadFromZero(_rand);
					_message->CopyFrom(*tmp);
					delete tmp;
					break;
				}
				}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Mutate expression"
		);
	}
);

// Invert condition of an if statement
static YPR<IfStmt> invertIfCondition(
	[](IfStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<IfStmt>(
			[](IfStmt* _message, YulRandomNumGenerator&)
			{
				if (_message->has_cond())
				{
					auto notOp = new UnaryOp();
					notOp->set_op(UnaryOp::NOT);
					auto oldCond = _message->release_cond();
					notOp->set_allocated_operand(oldCond);
					auto ifCond = new Expression();
					ifCond->set_allocated_unop(notOp);
					_message->set_allocated_cond(ifCond);
				}
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"If condition inverted"
		);
	}
);

// Remove inverted condition in if statement
static YPR<IfStmt> revertIfCondition(
	[](IfStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<IfStmt>(
			[](IfStmt* _message, YulRandomNumGenerator&)
			{
				if (_message->has_cond() && _message->cond().has_unop() &&
						_message->cond().unop().has_op() && _message->cond().unop().op() == UnaryOp::NOT)
				{
					auto oldCondition = _message->release_cond();
					auto unop = oldCondition->release_unop();
					auto conditionWithoutNot = unop->release_operand();
					_message->set_allocated_cond(conditionWithoutNot);
					delete (oldCondition);
					delete (unop);
				}
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"If condition reverted"
		);
	}
);

// Append break statement to a statement block
static YPR<Block> addBreakStmt(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				_message->add_statements()->set_allocated_breakstmt(new BreakStmt());
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Break statement added"
		);
	}
);

// Remove break statement in body of a for-loop statement
static YPR<ForStmt> removeBreakStmt(
	[](ForStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<ForStmt>(
			[](ForStmt* _message, YulRandomNumGenerator&)
			{
				if (_message->has_block())
					for (auto& stmt: *_message->mutable_block()->mutable_statements())
						if (stmt.has_breakstmt())
						{
							delete stmt.release_breakstmt();
							stmt.clear_breakstmt();
							break;
						}
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Break statement removed"
		);
	}
);

// Add continue statement to statement block.
static YPR<Block> addContStmt(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				_message->add_statements()->set_allocated_contstmt(new ContinueStmt());
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Continue statement added"
		);
	}
);

/// Remove continue statement from for-loop body
static YPR<ForStmt> removeContinueStmt(
	[](ForStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<ForStmt>(
			[](ForStmt* _message, YulRandomNumGenerator&)
			{
				if (_message->has_block())
					for (auto& stmt: *_message->mutable_block()->mutable_statements())
						if (stmt.has_contstmt())
						{
							delete stmt.release_contstmt();
							stmt.clear_contstmt();
							break;
						}
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Continue statement removed"
		);
	}
);

/// Mutate expression into an s/m/calldataload
static YPR<Expression> addLoadZero(
	[](Expression* _message, unsigned _seed)
	{
		YPM::functionWrapper<Expression>(
			[](Expression* _message, YulRandomNumGenerator& _rand)
			{
				YPM::clearExpr(_message);
				auto tmp = YPM::loadExpression(_rand);
				_message->CopyFrom(*tmp);
				delete tmp;
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Expression mutated to a load operation"
		);
	}
);

/// Remove unary operation containing a load from memory/storage/calldata
static YPR<UnaryOp> removeLoad(
	[](UnaryOp* _message, unsigned _seed)
	{
		YPM::functionWrapper<UnaryOp>(
			[](UnaryOp* _message, YulRandomNumGenerator&)
			{
				auto operation = _message->op();
				if (operation == UnaryOp::MLOAD || operation == UnaryOp::SLOAD ||
					operation == UnaryOp::CALLDATALOAD)
				{
					delete _message->release_operand();
					_message->clear_op();
				}
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Remove load operation"
		);
	}
);

static YPR<Block> storeToLoadFrom(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				Expression* e;
				switch (_rand() % 5)
				{
				case 0:
					e = YPM::binopExpression(_rand);
					break;
				case 1:
					e = YPM::loadExpression(_rand);
					break;
				case 2:
					e = YPM::loadFromZero(_rand);
					break;
				case 3:
					e = YPM::litExpression(_rand);
					break;
				case 4:
					e = YPM::refExpression(_rand);
					break;
				}
				auto store = new StoreFunc();
				store->set_allocated_loc(e);
				store->set_allocated_val(new Expression());
				bool coinFlip = _rand() % 2 == 0;
				store->set_st(coinFlip ? StoreFunc::MSTORE : StoreFunc::SSTORE);
				auto assign = new AssignmentStatement();
				auto copyOfE = new Expression();
				copyOfE->CopyFrom(*e);
				auto loadOp = new UnaryOp();
				loadOp->set_op(coinFlip ? UnaryOp::MLOAD : UnaryOp::SLOAD);
				loadOp->set_allocated_operand(copyOfE);
				auto loadExpr = new Expression();
				loadExpr->set_allocated_unop(loadOp);
				assign->set_allocated_expr(loadExpr);
				assign->set_allocated_ref_id(YPM::varRef(_rand));
				_message->add_statements()->set_allocated_storage_func(store);
				_message->add_statements()->set_allocated_assignment(assign);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Store to and load from same location"
		);
	}
);

void movableExpr(Expression* _expr, YulRandomNumGenerator& _rand)
{
	auto e = new Expression();
	e->CopyFrom(*_expr);
	YPM::clearExpr(_expr);
	auto b = new BinaryOp();
	switch (_rand() % 27)
	{
	// sub(x,x)
	case 0:
	{
		b->set_op(BinaryOp::SUB);
		b->set_allocated_left(e);
		auto copyOfE = new Expression();
		copyOfE->CopyFrom(*e);
		b->set_allocated_right(copyOfE);
		_expr->set_allocated_binop(b);
		break;
	}
	// div (x,0)
	case 1:
	{
		b->set_op(BinaryOp::DIV);
		b->set_allocated_left(e);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_right(zeroLit);
		_expr->set_allocated_binop(b);
		break;
	}
	// div(0,x)
	case 2:
	{
		b->set_op(BinaryOp::DIV);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(zeroLit);
		b->set_allocated_right(e);
		_expr->set_allocated_binop(b);
		break;
	}
	// mul(x,0)
	case 3:
	{
		b->set_op(BinaryOp::MUL);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(e);
		b->set_allocated_right(zeroLit);
		_expr->set_allocated_binop(b);
		break;
	}
	// mul(0,x)
	case 4:
	{
		b->set_op(BinaryOp::MUL);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(zeroLit);
		b->set_allocated_right(e);
		_expr->set_allocated_binop(b);
		break;
	}
	// sdiv(x,0)
	case 5:
	{
		b->set_op(BinaryOp::SDIV);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(e);
		b->set_allocated_right(zeroLit);
		_expr->set_allocated_binop(b);
		break;
	}
	// sdiv(0,x)
	case 6:
	{
		b->set_op(BinaryOp::SDIV);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(zeroLit);
		b->set_allocated_right(e);
		_expr->set_allocated_binop(b);
		break;
	}
	// and(x,0)
	case 7:
	{
		b->set_op(BinaryOp::AND);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(e);
		b->set_allocated_right(zeroLit);
		_expr->set_allocated_binop(b);
		break;
	}
	// and(0,x)
	case 8:
	{
		b->set_op(BinaryOp::AND);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(zeroLit);
		b->set_allocated_right(e);
		_expr->set_allocated_binop(b);
		break;
	}
	// or(x, not(0))
	case 9:
	{
		b->set_op(BinaryOp::XOR);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(e);
		auto notOp = new UnaryOp();
		notOp->set_op(UnaryOp::NOT);
		notOp->set_allocated_operand(zeroLit);
		auto notExpr = new Expression();
		notExpr->set_allocated_unop(notOp);
		b->set_allocated_right(notExpr);
		_expr->set_allocated_binop(b);
		break;
	}
	// or(not(0), x)
	case 10:
	{
		b->set_op(BinaryOp::XOR);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_right(e);
		auto notOp = new UnaryOp();
		notOp->set_op(UnaryOp::NOT);
		notOp->set_allocated_operand(zeroLit);
		auto notExpr = new Expression();
		notExpr->set_allocated_unop(notOp);
		b->set_allocated_left(notExpr);
		_expr->set_allocated_binop(b);
		break;
	}
	// mod(x,0)
	case 11:
	{
		b->set_op(BinaryOp::MOD);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(e);
		b->set_allocated_right(zeroLit);
		_expr->set_allocated_binop(b);
		break;
	}
	// mod(0,x)
	case 12:
	{
		b->set_op(BinaryOp::MOD);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(zeroLit);
		b->set_allocated_right(e);
		_expr->set_allocated_binop(b);
		break;
	}
	// shl(x,0)
	case 13:
	{
		b->set_op(BinaryOp::SHL);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(e);
		b->set_allocated_right(zeroLit);
		_expr->set_allocated_binop(b);
		break;
	}
	// shr(x,0)
	case 14:
	{
		b->set_op(BinaryOp::SHR);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(e);
		b->set_allocated_right(zeroLit);
		_expr->set_allocated_binop(b);
		break;
	}
	// gt(x,not(0))
	case 15:
	{
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		auto notOp = new UnaryOp();
		notOp->set_op(UnaryOp::NOT);
		notOp->set_allocated_operand(zeroLit);
		auto notExpr = new Expression();
		notExpr->set_allocated_unop(notOp);
		b->set_op(BinaryOp::GT);
		b->set_allocated_left(e);
		b->set_allocated_right(notExpr);
		_expr->set_allocated_binop(b);
		break;
	}
	// lt(not(0), x)
	case 16:
	{
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		auto notOp = new UnaryOp();
		notOp->set_op(UnaryOp::NOT);
		notOp->set_allocated_operand(zeroLit);
		auto notExpr = new Expression();
		notExpr->set_allocated_unop(notOp);
		b->set_op(BinaryOp::LT);
		b->set_allocated_left(notExpr);
		b->set_allocated_right(e);
		_expr->set_allocated_binop(b);
		break;
	}
	// gt(0,x)
	case 17:
	{
		b->set_op(BinaryOp::GT);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(zeroLit);
		b->set_allocated_right(e);
		_expr->set_allocated_binop(b);
		break;
	}
	// lt(x,0)
	case 18:
	{
		b->set_op(BinaryOp::LT);
		auto zeroLit = new Expression();
		zeroLit->set_allocated_cons(YPM::intLiteral(0));
		b->set_allocated_left(e);
		b->set_allocated_right(zeroLit);
		_expr->set_allocated_binop(b);
		break;
	}
	// and(x,x)
	case 19:
	{
		b->set_op(BinaryOp::AND);
		b->set_allocated_left(e);
		auto copyOfE = new Expression();
		copyOfE->CopyFrom(*e);
		b->set_allocated_right(copyOfE);
		_expr->set_allocated_binop(b);
		break;
	}
	// or(x,x)
	case 20:
	{
		b->set_op(BinaryOp::OR);
		b->set_allocated_left(e);
		auto copyOfE = new Expression();
		copyOfE->CopyFrom(*e);
		b->set_allocated_right(copyOfE);
		_expr->set_allocated_binop(b);
		break;
	}
	// xor(x,x)
	case 21:
	{
		b->set_op(BinaryOp::XOR);
		b->set_allocated_left(e);
		auto copyOfE = new Expression();
		copyOfE->CopyFrom(*e);
		b->set_allocated_right(copyOfE);
		_expr->set_allocated_binop(b);
		break;
	}
	// eq(x,x)
	case 22:
	{
		b->set_op(BinaryOp::EQ);
		b->set_allocated_left(e);
		auto copyOfE = new Expression();
		copyOfE->CopyFrom(*e);
		b->set_allocated_right(copyOfE);
		_expr->set_allocated_binop(b);
		break;
	}
	// lt(x,x)
	case 23:
	{
		b->set_op(BinaryOp::LT);
		b->set_allocated_left(e);
		auto copyOfE = new Expression();
		copyOfE->CopyFrom(*e);
		b->set_allocated_right(copyOfE);
		_expr->set_allocated_binop(b);
		break;
	}
	// gt(x,x)
	case 24:
	{
		b->set_op(BinaryOp::GT);
		b->set_allocated_left(e);
		auto copyOfE = new Expression();
		copyOfE->CopyFrom(*e);
		b->set_allocated_right(copyOfE);
		_expr->set_allocated_binop(b);
		break;
	}
	// sgt(x,x)
	case 25:
	{
		b->set_op(BinaryOp::SGT);
		b->set_allocated_left(e);
		auto copyOfE = new Expression();
		copyOfE->CopyFrom(*e);
		b->set_allocated_right(copyOfE);
		_expr->set_allocated_binop(b);
		break;
	}
	// mod(x,x)
	case 26:
	{
		b->set_op(BinaryOp::MOD);
		b->set_allocated_left(e);
		auto copyOfE = new Expression();
		copyOfE->CopyFrom(*e);
		b->set_allocated_right(copyOfE);
		_expr->set_allocated_binop(b);
		break;
	}
	// xor(x,xor(x,y))
//	case 27:
//	{
//
//	}
//	// xor(x,xor(y,x))
//	case 28:
//	{
//
//	}
//	// xor(xor(x,y),x)
//	case 29:
//	{
//
//	}
//	// xor(xor(y,x),x)
//	case 30:
//	{
//
//	}
//	// or(x,and(x,y))
//	case 31:
//	{
//
//	}
//	// or(x,and(y,x))
//	case 32:
//	{
//
//	}
//	// or(and(x,y),x)
//	case 33:
//	{
//
//	}
//	// or(and(y,x),x)
//	case 34:
//	{
//
//	}
//	// and(x,or(x,y))
//	case 35:
//	{
//
//	}
//	// and(x,or(y,x))
//	case 36:
//	{
//
//	}
//	// and(or(x,y),x)
//	case 37:
//	{
//
//	}
//	// and(or(y,x),x)
//	case 38:
//	{
//
//	}
//	// and(x, not(x))
//	case 39:
//	{
//
//	}
//	// and(not(x), x)
//	case 40:
//	{
//
//	}
//	// or(x,not(x))
//	case 41:
//	{
//
//	}
//	// or(not(x),x)
//	case 42:
//	{
//
//	}
	}
}

static YPR<Expression> movableExprMutation(
	[](Expression* _message, unsigned _seed)
	{
		YPM::functionWrapper<Expression>(
			[](Expression* _message, YulRandomNumGenerator& _rand)
			{
				movableExpr(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Mutate to movable expression"
		);
	}
);

static YPR<Block> nonmovableFunction(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto f = new FunctionDef();
				f->set_num_input_params(0);
			    f->set_num_output_params(1);
				if (_rand() % 2 == 0)
					f->mutable_block()->add_statements()->set_allocated_storage_func(new StoreFunc());
				else
					f->mutable_block()->add_statements()->set_allocated_forstmt(new ForStmt());
				auto call = new FunctionExpr();
				call->set_index(0);
				auto callExpr = new Expression();
				callExpr->set_allocated_funcexpr(call);
				movableExpr(callExpr, _rand);
				auto s = new StoreFunc();
				s->set_st(StoreFunc::MSTORE);
				s->set_allocated_val(callExpr);
				_message->add_statements()->set_allocated_funcdef(f);
				_message->add_statements()->set_allocated_storage_func(s);
			},
			_message,
			_seed,
			13,
			"Add simple op with nonmovable function"
		);
	}
);

/// Add m/sstore(0, variable)
static YPR<Block> addStoreToZero(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
		[](Block* _message, YulRandomNumGenerator& _rand)
		{
			auto storeStmt = new StoreFunc();
			storeStmt->set_st(YPM::EnumTypeConverter<StoreFunc_Storage>{}.enumFromSeed(_rand()));
			storeStmt->set_allocated_loc(YPM::litExpression(_rand));
			storeStmt->set_allocated_val(YPM::refExpression(_rand));
			auto stmt = _message->add_statements();
			stmt->set_allocated_storage_func(storeStmt);
		},
		_message,
		_seed,
		YPM::s_highIP,
		"Store to zero added"
		);
	}
);

static YPR<Block> cyclicFunctions(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				auto f1 = new FunctionDef();
				auto f2 = new FunctionDef();
				auto f3 = new FunctionDef();

				auto f1callsf2 = new FunctionCall();
				f1callsf2->set_func_index(1);
				auto f2callsf3 = new FunctionCall();
				f2callsf3->set_func_index(2);
				auto f3callsf1 = new FunctionCall();
				f3callsf1->set_func_index(0);

				f1->mutable_block()->add_statements()->set_allocated_functioncall(f1callsf2);
				f2->mutable_block()->add_statements()->set_allocated_functioncall(f2callsf3);
				f3->mutable_block()->add_statements()->set_allocated_functioncall(f3callsf1);
				_message->add_statements()->set_allocated_funcdef(f1);
				_message->add_statements()->set_allocated_funcdef(f2);
				_message->add_statements()->set_allocated_funcdef(f3);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add cyclic calls"
		);
	}
);

static YPR<Block> callFunctionWithInfLoop(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				auto f1 = new FunctionDef();
				f1->set_num_input_params(0);
				f1->set_num_output_params(1);
				f1->set_force_call(true);
				auto f1callsf2 = new FunctionCall();
				f1callsf2->set_func_index(1);
				f1->mutable_block()->add_statements()->set_allocated_functioncall(f1callsf2);

				auto f2 = new FunctionDef();
				f2->mutable_block()->add_statements()->set_allocated_forstmt(new ForStmt());
				_message->add_statements()->set_allocated_funcdef(f1);
				_message->add_statements()->set_allocated_funcdef(f2);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Call function with infinite loop"
		);
	}
);

static YPR<Block> removeStore(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_storage_func())
					{
						delete stmt.release_storage_func();
						stmt.clear_storage_func();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Remove store"
		);
	}
);

static YPR<ForStmt> invertForCondition(
	[](ForStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<ForStmt>(
			[](ForStmt* _message, YulRandomNumGenerator&)
			{
				if (_message->has_for_cond())
				{
					auto notOp = new UnaryOp();
					notOp->set_op(UnaryOp::NOT);
					auto oldCond = _message->release_for_cond();
					notOp->set_allocated_operand(oldCond);
					auto forCond = new Expression();
					forCond->set_allocated_unop(notOp);
					_message->set_allocated_for_cond(forCond);
				}
				else
					_message->set_allocated_for_cond(new Expression());
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"For condition inverted"
		);
	}
);

/// Uninvert condition of a for statement
static YPR<ForStmt> uninvertForCondition(
	[](ForStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<ForStmt>(
			[](ForStmt* _message, YulRandomNumGenerator&)
			{
				if (_message->has_for_cond() && _message->for_cond().has_unop() &&
					_message->for_cond().unop().has_op() && _message->for_cond().unop().op() == UnaryOp::NOT)
				{
					auto oldCondition = _message->release_for_cond();
					auto unop = oldCondition->release_unop();
					auto newCondition = unop->release_operand();
					_message->set_allocated_for_cond(newCondition);
				}
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Uninvert for condition"
		);
	}
);

/// Make for loop condition a function call that returns a single value
static YPR<ForStmt> funcCallForCondition(
	[](ForStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<ForStmt>(
			[](ForStmt* _message, YulRandomNumGenerator& _rand)
			{
				if (_message->has_for_cond())
				{
					_message->clear_for_cond();
					auto functionCall = new FunctionExpr();
					functionCall->set_index(_rand());
					auto forCondExpr = new Expression();
					forCondExpr->set_allocated_funcexpr(functionCall);
					_message->set_allocated_for_cond(forCondExpr);
				}
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Function call in for condition added"
		);
	}
);

/// Define an identity function y = x
static YPR<Block> identityFunction(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto functionDef = new FunctionDef();
				functionDef->set_num_input_params(1);
				functionDef->set_num_output_params(1);
				auto functionBlock = new Block();
				auto assignmentStatement = new AssignmentStatement();
				assignmentStatement->set_allocated_ref_id(YPM::varRef(_rand));
				auto rhs = new Expression();
				rhs->set_allocated_varref(YPM::varRef(_rand));
				assignmentStatement->set_allocated_expr(rhs);
				functionBlock->add_statements()->set_allocated_assignment(assignmentStatement);
				functionDef->set_allocated_block(functionBlock);
				_message->add_statements()->set_allocated_funcdef(functionDef);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Identity function added"
		);
	}
);

// Add leave statement to a statement block
static YPR<Block> addLeave(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				_message->add_statements()->set_allocated_leave(new LeaveStmt());
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add leave to statement block"
		);
	}
);

// Remove leave statement from function statement-block.
static YPR<FunctionDef> removeLeave(
	[](FunctionDef* _message, unsigned _seed)
	{
		YPM::functionWrapper<FunctionDef>(
			[](FunctionDef* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_block()->mutable_statements())
					if (stmt.has_leave())
					{
						delete stmt.release_leave();
						stmt.clear_leave();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_lowIP,
			"Remove leave from function statement block"
		);
	}
);

// Add assignment to block
static YPR<Block> addAssignment(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto assignmentStatement = new AssignmentStatement();
				auto varRef = YPM::varRef(_rand);
				assignmentStatement->set_allocated_ref_id(varRef);
				auto rhs = YPM::varRef(_rand);
				auto rhsExpr = new Expression();
				rhsExpr->set_allocated_varref(rhs);
				assignmentStatement->set_allocated_expr(rhsExpr);
				_message->add_statements()->set_allocated_assignment(assignmentStatement);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add assignment to statement block"
		);
	}
);

// Remove assignment from block
static YPR<Block> removeAssignment(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_assignment())
					{
						delete stmt.release_assignment();
						stmt.clear_assignment();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Remove assignment from statement block"
		);
	}
);

// Add constant assignment
static YPR<Block> addConstantAssignment(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto assignmentStatement = new AssignmentStatement();
				assignmentStatement->set_allocated_ref_id(
					YPM::varRef(_rand)
				);
				assignmentStatement->set_allocated_expr(
					YPM::litExpression(_rand)
				);
				_message->add_statements()->set_allocated_assignment(assignmentStatement);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add constant assignment to statement block"
		);
	}
);

template<typename BlockStmt, typename Stmt>
void addStmtTemplated(BlockStmt* _b)
{
	if constexpr (std::is_same_v<std::decay_t<Stmt>, IfStmt>)
		_b->mutable_block()->add_statements()->set_allocated_ifstmt(new Stmt());
	else if constexpr (std::is_same_v<std::decay_t<Stmt>, ForStmt>)
		_b->mutable_block()->add_statements()->set_allocated_forstmt(new Stmt());
	else if constexpr (std::is_same_v<std::decay_t<Stmt>, BoundedForStmt>)
		_b->mutable_block()->add_statements()->set_allocated_boundedforstmt(new Stmt());
	else if constexpr (std::is_same_v<std::decay_t<Stmt>, SwitchStmt>)
		_b->mutable_block()->add_statements()->set_allocated_switchstmt(new Stmt());
	else if constexpr (std::is_same_v<std::decay_t<Stmt>, LeaveStmt>)
		_b->mutable_block()->add_statements()->set_allocated_leave(new Stmt());
	else if constexpr (std::is_same_v<std::decay_t<Stmt>, BreakStmt>)
		_b->mutable_block()->add_statements()->set_allocated_breakstmt(new Stmt());
	else if constexpr (std::is_same_v<std::decay_t<Stmt>, ContinueStmt>)
		_b->mutable_block()->add_statements()->set_allocated_contstmt(new Stmt());
	else if constexpr (std::is_same_v<std::decay_t<Stmt>, FunctionCall>)
		_b->mutable_block()->add_statements()->set_allocated_functioncall(new Stmt());
	else
		static_assert(YPM::AlwaysFalse<Stmt>::value, "Yul proto mutator: non-exhaustive visitor.");
}

template <typename BlockStmt>
void addControlFlowStmt(BlockStmt* _b, YulRandomNumGenerator& _rand)
{
	switch (_rand() % 8)
	{
	case 0:
		addStmtTemplated<BlockStmt, IfStmt>(_b);
		break;
	case 1:
		addStmtTemplated<BlockStmt, ForStmt>(_b);
		break;
	case 2:
		addStmtTemplated<BlockStmt, BoundedForStmt>(_b);
		break;
	case 3:
		addStmtTemplated<BlockStmt, SwitchStmt>(_b);
		break;
	case 4:
		addStmtTemplated<BlockStmt, LeaveStmt>(_b);
		break;
	case 5:
		addStmtTemplated<BlockStmt, BreakStmt>(_b);
		break;
	case 6:
		addStmtTemplated<BlockStmt, ContinueStmt>(_b);
		break;
	case 7:
		addStmtTemplated<BlockStmt, FunctionCall>(_b);
		break;
	}
}

static YPR<ForStmt> addControlFlowToFor(
	[](ForStmt* _message, unsigned _seed)
	{
	  YPM::functionWrapper<ForStmt>(
			[](ForStmt* _message, YulRandomNumGenerator& _rand)
			{
				addControlFlowStmt<ForStmt>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add control flow statement to for body"
		);
	}
);

static YPR<BoundedForStmt> addControlFlowToBoundedFor(
	[](BoundedForStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<BoundedForStmt>(
			[](BoundedForStmt* _message, YulRandomNumGenerator& _rand)
			{
				addControlFlowStmt<BoundedForStmt>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add control flow statement to bounded for body"
		);
	}
);

static YPR<IfStmt> addControlFlowToIf(
	[](IfStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<IfStmt>(
			[](IfStmt* _message, YulRandomNumGenerator& _rand)
			{
				addControlFlowStmt<IfStmt>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add control flow statement to if body"
		);
	}
);

static YPR<SwitchStmt> addControlFlowToSwitch(
	[](SwitchStmt* _message, unsigned _seed)
		{
			YPM::functionWrapper<SwitchStmt>(
			[](SwitchStmt* _message, YulRandomNumGenerator& _rand)
			{
				addControlFlowStmt<SwitchStmt>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add control flow statement to switch default block"
			);
		}
);

static YPR<CaseStmt> addControlFlowToSwitchCase(
	[](CaseStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<CaseStmt>(
			[](CaseStmt* _message, YulRandomNumGenerator& _rand)
			{
				addControlFlowStmt<CaseStmt>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add control flow statement to switch case block"
		);
	}
);

// Add if statement
static YPR<Block> addIfStmt(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto ifStmt = new IfStmt();
				ifStmt->set_allocated_cond(YPM::refExpression(_rand));
				// Add an assignment inside if
				auto ifBody = new Block();
				auto ifAssignment = new AssignmentStatement();
				ifAssignment->set_allocated_ref_id(YPM::varRef(_rand));
				ifAssignment->set_allocated_expr(YPM::refExpression(_rand));
				auto ifBodyStmt = ifBody->add_statements();
				ifBodyStmt->set_allocated_assignment(ifAssignment);
				ifStmt->set_allocated_block(ifBody);
				_message->add_statements()->set_allocated_ifstmt(ifStmt);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add if statement to statement block"
		);
	}
);

// Remove if statement
static YPR<Block> removeIfStmt(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_ifstmt())
					{
						delete stmt.release_ifstmt();
						stmt.clear_ifstmt();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_lowIP,
			"Remove if statement from statement block"
		);
	}
);

// Add switch statement
static YPR<Block> addSwitchStmt(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto switchStmt = new SwitchStmt();
				switchStmt->add_case_stmt();
				Expression *switchExpr = new Expression();
				switchExpr->set_allocated_varref(YPM::varRef(_rand));
				switchStmt->set_allocated_switch_expr(switchExpr);
				_message->add_statements()->set_allocated_switchstmt(switchStmt);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add switch statement to statement block"
		);
	}
);

// Remove switch statement
static YPR<Block> removeSwitchStmt(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_switchstmt())
					{
						delete stmt.release_switchstmt();
						stmt.clear_switchstmt();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_lowIP,
			"Remove switch statement from statement block"
		);
	}
);

// Add function call to statement block
static YPR<Block> addFuncCall(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto call = new FunctionCall();
				YPM::configureCall(call, _rand);
				_message->add_statements()->set_allocated_functioncall(call);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add function call to statement block"
		);
	}
);

// Remove function call
static YPR<Block> removeFuncCall(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_functioncall())
					{
						delete stmt.release_functioncall();
						stmt.clear_functioncall();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Remove function call from statement block"
		);
	}
);

// Add variable declaration
static YPR<Block> addVarDecl(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				_message->add_statements()->set_allocated_decl(new VarDecl());
				// Hoist var decl to beginning of block
				if (_message->statements_size() > 1)
					_message->mutable_statements(0)->Swap(
						_message->mutable_statements(_message->statements_size() - 1)
					);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add variable declaration to statement block"
		);
	}
);

// Add multivar decl
static YPR<Block> addMultiVarDecl(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto decl = new MultiVarDecl();
				decl->set_num_vars(_rand());
				_message->add_statements()->set_allocated_multidecl(decl);
				// Hoist multi var decl to beginning of block
				if (_message->statements_size() > 1)
					_message->mutable_statements(0)->Swap(
						_message->mutable_statements(_message->statements_size() - 1)
					);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add multi variable declaration to statement block"
		);
	}
);

// Remove variable declaration
static YPR<Block> removeVarDecl(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_decl())
					{
						delete stmt.release_decl();
						stmt.clear_decl();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Remove variable declaration from statement block"
		);
	}
);

// Remove multi variable declaration
static YPR<Block> removeMultiVarDecl(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_multidecl())
					{
						delete stmt.release_multidecl();
						stmt.clear_multidecl();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Remove multi variable declaration from statement block"
		);
	}
);

// Add function definition
static YPR<Block> addFuncDef(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto funcDef = new FunctionDef();
				// TODO: Remove hard coding
				auto numInputParams = _rand() % 5;
				auto numOutputParams = _rand() % 5;
				funcDef->set_num_input_params(numInputParams);
				funcDef->set_num_output_params(numOutputParams);
				funcDef->set_allocated_block(new Block());
				if (numOutputParams > 0)
				{
					// Output param index starts at numInputParams
					// Output param index ends at numInputParams + numOutputParams - 1
					auto outParamIndex = numInputParams + (_rand() % numOutputParams);
					auto outVarRef = new VarRef();
					outVarRef->set_varnum(outParamIndex);
					auto outParamAssign = new AssignmentStatement();
					outParamAssign->set_allocated_ref_id(outVarRef);
					funcDef->mutable_block()->add_statements()->set_allocated_assignment(outParamAssign);
				}
				if (numInputParams > 0)
				{
					auto inParamIndex = _rand() % numInputParams;
					auto inVarRef = new VarRef();
					inVarRef->set_varnum(inParamIndex);
					auto inExpr = new Expression();
					inExpr->set_allocated_varref(inVarRef);
					if (_rand() % 2 == 0)
					{
						auto dDCFIf = new IfStmt();
						dDCFIf->set_allocated_cond(inExpr);
						funcDef->mutable_block()->add_statements()->set_allocated_ifstmt(dDCFIf);
					}
					else
					{
						auto dDCFSwitch = new SwitchStmt();
						dDCFSwitch->set_allocated_switch_expr(inExpr);
						funcDef->mutable_block()->add_statements()->set_allocated_switchstmt(dDCFSwitch);
					}
				}
				_message->add_statements()->set_allocated_funcdef(funcDef);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add function definition to statement block"
		);
	}
);

// Remove function definition
static YPR<Block> removeFuncDef(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_funcdef())
					{
						delete stmt.release_funcdef();
						stmt.clear_funcdef();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Remove function definition from statement block"
		);
	}
);

// Add bounded for stmt
static YPR<Block> addBoundedFor(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				_message->add_statements()->set_allocated_boundedforstmt(new BoundedForStmt());
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add bounded for statement to statement block"
		);
	}
);

// Remove bounded for stmt
static YPR<Block> removeBoundedFor(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_boundedforstmt())
					{
						delete stmt.release_boundedforstmt();
						stmt.clear_boundedforstmt();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Remove bounded for statement from statement block"
		);
	}
);

// Add generic for stmt
static YPR<Block> addGenericFor(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				_message->add_statements()->set_allocated_forstmt(new ForStmt());
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add for statement to statement block"
		);
	}
);

// Remove generic for stmt
static YPR<Block> removeGenericFor(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_forstmt())
					{
						delete stmt.release_forstmt();
						stmt.clear_forstmt();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Remove for statement from statement block"
		);
	}
);

// Add revert stmt
static YPR<Block> addRevert(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto termStmt = new TerminatingStmt();
				auto revertStmt = new RetRevStmt();
				revertStmt->set_stmt(RetRevStmt::REVERT);
				revertStmt->set_allocated_pos(YPM::litExpression(_rand));
				revertStmt->set_allocated_size(YPM::litExpression(_rand));
				termStmt->set_allocated_ret_rev(revertStmt);
				_message->add_statements()->set_allocated_terminatestmt(termStmt);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add revert(0,0) statement to statement block"
		);
	}
);

// Remove revert statement
static YPR<Block> removeRevert(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_terminatestmt() && stmt.terminatestmt().has_ret_rev() &&
						stmt.terminatestmt().ret_rev().stmt() == RetRevStmt::REVERT)
					{
						delete stmt.release_terminatestmt();
						stmt.clear_terminatestmt();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_lowIP,
			"Remove revert statement from statement block"
		);
	}
);

// Mutate nullary op
static YPR<Expression> mutateNullaryOp(
	[](Expression* _message, unsigned _seed)
	{
		YPM::functionWrapper<Expression>(
			[](Expression* _message, YulRandomNumGenerator& _rand)
			{
				YPM::clearExpr(_message);
				auto op = new NullaryOp();
				op->set_op(
					YPM::EnumTypeConverter<NullaryOp_NOp>{}.enumFromSeed(_rand())
				);
				_message->set_allocated_nop(op);
			},
			_message,
			_seed,
			13,
			"Mutate nullary operation in expression"
		);
	}
);

// Mutate binary op
static YPR<BinaryOp> mutateBinaryOp(
	[](BinaryOp* _message, unsigned _seed)
	{
		YPM::functionWrapper<BinaryOp>(
			[](BinaryOp* _message, YulRandomNumGenerator& _rand)
			{
				_message->clear_op();
				_message->set_op(
					YPM::EnumTypeConverter<BinaryOp_BOp>{}.enumFromSeed(_rand())
				);
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Mutate binary operation in expression"
		);
	}
);

// Mutate unary op
static YPR<UnaryOp> mutateUnaryOp(
	[](UnaryOp* _message, unsigned _seed)
	{
		YPM::functionWrapper<UnaryOp>(
			[](UnaryOp* _message, YulRandomNumGenerator& _rand)
			{
				_message->clear_op();
				_message->set_op(
					YPM::EnumTypeConverter<UnaryOp_UOp>{}.enumFromSeed(_rand())
				);
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Mutate unary operation in expression"
		);
	}
);

// Add pop(call())
static YPR<Block> addPopCall(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto call = new LowLevelCall();
				call->set_callty(
					YPM::EnumTypeConverter<LowLevelCall_Type>{}.enumFromSeed(_rand())
				);
				auto popExpr = new Expression();
				popExpr->set_allocated_lowcall(call);
				auto popStmt = new PopStmt();
				popStmt->set_allocated_expr(popExpr);
				_message->add_statements()->set_allocated_pop(popStmt);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add pop(call()) statement to statement block"
		);
	}
);

// Remove pop
static YPR<Block> removePop(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_statements())
					if (stmt.has_pop())
					{
						delete stmt.release_pop();
						stmt.clear_pop();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Remove pop statement from statement block"
		);
	}
);

// Add pop(create)
static YPR<Block> addPopCreate(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				auto create = new Create();
				create->set_createty(
					YPM::EnumTypeConverter<Create_Type>{}.enumFromSeed(_rand())
				);
				auto popExpr = new Expression();
				popExpr->set_allocated_create(create);
				auto popStmt = new PopStmt();
				popStmt->set_allocated_expr(popExpr);
				_message->add_statements()->set_allocated_pop(popStmt);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add pop(create) statement to statement block"
		);
	}
);

// Add pop(f()) where f() -> r is a user-defined function.
// Assumes that f() already exists, if it does not this turns into pop(constant).
static YPR<Block> addPopUserFunc(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
			  auto functioncall = new FunctionExpr();
			  functioncall->set_index(_rand());
			  // TODO: Configure call args
//			  YPM::configureCallArgs(FunctionCall::SINGLE, functioncall, _rand);
			  auto funcExpr = new Expression();
			  funcExpr->set_allocated_funcexpr(functioncall);
			  auto popStmt = new PopStmt();
			  popStmt->set_allocated_expr(funcExpr);
			  _message->add_statements()->set_allocated_pop(popStmt);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add pop(f()) statement to statement block"
		);
	}
);

// Add function call in another function's body
static YPR<FunctionDef> addFuncCallInFuncBody(
	[](FunctionDef* _message, unsigned _seed)
	{
		YPM::functionWrapper<FunctionDef>(
			[](FunctionDef* _message, YulRandomNumGenerator& _rand)
			{
				auto functioncall = new FunctionCall();
				YPM::configureCall(functioncall, _rand);
				_message->mutable_block()->add_statements()->set_allocated_functioncall(functioncall);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add function call in function body"
		);
	}
);

// Remove function call from a function's body
static YPR<FunctionDef> removeFuncCallInFuncBody(
	[](FunctionDef* _message, unsigned _seed)
	{
		YPM::functionWrapper<FunctionDef>(
			[](FunctionDef* _message, YulRandomNumGenerator&)
			{
				for (auto &stmt: *_message->mutable_block()->mutable_statements())
					if (stmt.has_functioncall())
					{
						delete stmt.release_functioncall();
						stmt.clear_functioncall();
						break;
					}
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Remove function call from function body"
		);
	}
);

static YPR<FunctionDef>	writeToOutputParams(
	[](FunctionDef* _message, unsigned _seed)
	{
		YPM::functionWrapper<FunctionDef>(
			[](FunctionDef* _message, YulRandomNumGenerator& _rand)
			{
				unsigned numInputParams = _message->num_input_params() % 5;
				unsigned numOutputParams = _message->num_output_params() % 5;
				if (numOutputParams > 0)
				{
					if (_rand() % 2 == 0)
					{
						// All output params are assigned to
						for (unsigned i = 0; i < numOutputParams; i++)
						{
							auto varRef = new VarRef();
							varRef->set_varnum(numInputParams + i);
							auto assignment = new AssignmentStatement();
							assignment->set_allocated_ref_id(varRef);
							_message->mutable_block()->add_statements()->set_allocated_assignment(assignment);
						}
					}
					else
					{
						auto functionCall = new FunctionCall();
						functionCall->set_func_index(_rand());
						functionCall->mutable_out_param1()->set_varnum(numInputParams);
						functionCall->mutable_out_param2()->set_varnum(numInputParams + 1);
						functionCall->mutable_out_param3()->set_varnum(numInputParams + 2);
						functionCall->mutable_out_param4()->set_varnum(numInputParams + 3);
						_message->mutable_block()->add_statements()->set_allocated_functioncall(functionCall);
					}
				}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Mutate assign statements so they write to out param"
		);
	}
);

static YPR<FunctionDef> readFromInputParams(
	[](FunctionDef* _message, unsigned _seed)
	{
		YPM::functionWrapper<FunctionDef>(
			[](FunctionDef* _message, YulRandomNumGenerator& _rand)
			{
				unsigned numInputParams = _message->num_input_params() % 5;
				if (numInputParams > 0)
				{
					// Choose a random input parameter to read from
					auto varRef = new VarRef();
					varRef->set_varnum(_rand() % numInputParams);
					switch (_rand() % 4)
					{
					case 0:
					{
						auto ifStmt = new IfStmt();
						ifStmt->mutable_cond()->set_allocated_varref(varRef);
						_message->mutable_block()->add_statements()->set_allocated_ifstmt(ifStmt);
						break;
					}
					case 1:
					{
						auto switchStmt = new SwitchStmt();
						switchStmt->mutable_switch_expr()->set_allocated_varref(varRef);
						_message->mutable_block()->add_statements()->set_allocated_switchstmt(switchStmt);
						break;
					}
					case 2:
					{
						auto forStmt = new ForStmt();
						forStmt->mutable_for_cond()->set_allocated_varref(varRef);
						_message->mutable_block()->add_statements()->set_allocated_forstmt(forStmt);
						break;
					}
					case 3:
					{
						auto functionCall = new FunctionCall();
						auto getVarRef = [](YulRandomNumGenerator& _rand, unsigned _numInputParams) -> VarRef* {
						  auto v = new VarRef();
						  v->set_varnum(_rand() % _numInputParams);
						  return v;
						};
						functionCall->mutable_in_param1()->set_allocated_varref(varRef);
						functionCall->mutable_in_param2()->set_allocated_varref(getVarRef(_rand, numInputParams));
						functionCall->mutable_in_param3()->set_allocated_varref(getVarRef(_rand, numInputParams));
						functionCall->mutable_in_param4()->set_allocated_varref(getVarRef(_rand, numInputParams));
						_message->mutable_block()->add_statements()->set_allocated_functioncall(functionCall);
						break;
					}
					}
				}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Mutate conditional expressions to read from input params"
		);
	}
);

template <typename BlockStmt>
void randomiseStmtOrder(BlockStmt* _b, YulRandomNumGenerator& _rand)
{
	unsigned numStatements = _b->block().statements_size();
	auto swapIndex = [&]() { return _rand() % numStatements; };
	if (numStatements > 1)
		_b->mutable_block()->mutable_statements()->SwapElements(swapIndex(), swapIndex());
}


static YPR<FunctionDef> randomiseFunctionBlock(
	[](FunctionDef* _message, unsigned _seed)
	{
		YPM::functionWrapper<FunctionDef>(
			[](FunctionDef* _message, YulRandomNumGenerator& _rand)
			{
				randomiseStmtOrder<FunctionDef>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Change order of statements in function block"
		);
	}
);

static YPR<IfStmt> randomiseIfBlock(
	[](IfStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<IfStmt>(
			[](IfStmt* _message, YulRandomNumGenerator& _rand)
			{
				randomiseStmtOrder<IfStmt>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Change order of statements in if block"
		);
	}
);

static YPR<SwitchStmt> randomiseSwitchDefaultBlock(
	[](SwitchStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<SwitchStmt>(
			[](SwitchStmt* _message, YulRandomNumGenerator& _rand)
			{
				randomiseStmtOrder<SwitchStmt>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Change order of statements in switch default block"
		);
	}
);

static YPR<CaseStmt> randomiseCaseBlock(
	[](CaseStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<CaseStmt>(
			[](CaseStmt* _message, YulRandomNumGenerator& _rand)
			{
				randomiseStmtOrder<CaseStmt>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Change order of statements in switch-case block"
		);
	}
);

static YPR<Code> randomiseCodeBlock(
	[](Code* _message, unsigned _seed)
	{
		YPM::functionWrapper<Code>(
			[](Code* _message, YulRandomNumGenerator& _rand)
			{
				randomiseStmtOrder<Code>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Change order of statements in code block"
		);
	}
);

static YPR<Program> randomiseProgramBlock(
	[](Program* _message, unsigned _seed)
	{
		YPM::functionWrapper<Program>(
			[](Program* _message, YulRandomNumGenerator& _rand)
			{
				if (_message->has_block())
					randomiseStmtOrder<Program>(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Change order of statements in program block"
		);
	}
);

// Add dataoffset/datasize
static YPR<Expression> addDataExpr(
	[](Expression* _message, unsigned _seed)
	{
		YPM::functionWrapper<Expression>(
			[](Expression* _message, YulRandomNumGenerator& _rand)
			{
				YPM::clearExpr(_message);
				auto unopdata = new UnaryOpData();
				unopdata->set_identifier(_rand());
				unopdata->set_op(
					YPM::EnumTypeConverter<UnaryOpData_UOpData>{}.enumFromSeed(_rand())
				);
				_message->set_allocated_unopdata(unopdata);
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Mutate expression to dataoffset/size"
		);
	}
);

// Add variable reference inside for-loop body
static YPR<BoundedForStmt> addVarRefInForBody(
	[](BoundedForStmt* _message, unsigned _seed)
	{
		YPM::functionWrapper<BoundedForStmt>(
			[](BoundedForStmt* _message, YulRandomNumGenerator& _rand)
			{
				auto popStmt = new PopStmt();
				popStmt->set_allocated_expr(YPM::refExpression(_rand));
				_message->mutable_block()->add_statements()->set_allocated_pop(popStmt);
			},
			_message,
			_seed,
			YPM::s_mediumIP,
			"Add variable reference in for loop body"
		);
	}
);

// Mutate expression to a function call
static YPR<Expression> mutateExprToFuncCall(
	[](Expression* _message, unsigned _seed)
	{
		YPM::functionWrapper<Expression>(
			[](Expression* _message, YulRandomNumGenerator& _rand)
			{
				YPM::clearExpr(_message);
				auto functionCall = new FunctionExpr();
				functionCall->set_index(_rand());
				_message->set_allocated_funcexpr(functionCall);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Mutate expression to function call"
		);
	}
);

// Mutate expression to variable reference
static YPR<Expression> mutateExprToVarRef(
	[](Expression* _message, unsigned _seed)
	{
		YPM::functionWrapper<Expression>(
			[](Expression* _message, YulRandomNumGenerator& _rand)
			{
				YPM::clearExpr(_message);
				_message->set_allocated_varref(YPM::varRef(_rand));
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Mutate expression to a variable reference"
		);
	}
);

// Add varref to statement
static YPR<Statement> addVarRefToStmt(
	[](Statement* _message, unsigned _seed)
	{
		YPM::functionWrapper<Statement>(
			[](Statement* _message, YulRandomNumGenerator& _rand)
			{
				YPM::addArgs(_message, YPM::refExpression, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Make statement arguments variable references"
		);
	}
);

// Add varrefs to unset statement arguments recursively
static YPR<Statement> addVarRefToStmtRec(
	[](Statement* _message, unsigned _seed)
	{
		YPM::functionWrapper<Statement>(
			[](Statement* _message, YulRandomNumGenerator& _rand)
			{
				YPM::addArgsRec(
					_message,
					[](Expression* _expr, YulRandomNumGenerator& _rand)
					{
						_expr->set_allocated_varref(YPM::varRef(_rand));
					},
					_rand
				);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Make statement arguments variable references recursively"
		);
	}
);

// Add binary operations to unset statement arguments recursively
static YPR<Statement> addBinopToStmtRec(
	[](Statement* _message, unsigned _seed)
	{
		YPM::functionWrapper<Statement>(
			[](Statement* _message, YulRandomNumGenerator& _rand)
			{
				YPM::addArgsRec(
					_message,
					[](Expression* _expr, YulRandomNumGenerator& _rand)
					{
						auto tmp = YPM::binopExpression(_rand);
						_expr->CopyFrom(*tmp);
						delete tmp;
					},
					_rand
				);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Make statement arguments binary ops recursively"
		);
	}
);

// Add load operation to unset statement arguments recursively
static YPR<Statement> addLoadToStmtRec(
	[](Statement* _message, unsigned _seed)
	{
		YPM::functionWrapper<Statement>(
			[](Statement* _message, YulRandomNumGenerator& _rand)
			{
				YPM::addArgsRec(
					_message,
					[](Expression* _expr, YulRandomNumGenerator& _rand)
					{
						auto tmp = YPM::loadExpression(_rand);
						_expr->CopyFrom(*tmp);
						delete tmp;
					},
					_rand
				);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Make statement arguments load expression recursively"
		);
	}
);

// Add load from zero location ops to unset statement arguments recursively
static YPR<Statement> addLoadFromZeroToStmtRec(
	[](Statement* _message, unsigned _seed)
	{
		YPM::functionWrapper<Statement>(
			[](Statement* _message, YulRandomNumGenerator& _rand)
			{
				YPM::addArgsRec(
					_message,
					[](Expression* _expr, YulRandomNumGenerator& _rand)
					{
						auto tmp = YPM::loadFromZero(_rand);
						_expr->CopyFrom(*tmp);
						delete tmp;
					},
					_rand
				);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Make statement arguments loads from location zero recursively"
		);
	}
);

// Add binop expression to statement.
static YPR<Statement> addBinopToStmt(
	[](Statement* _message, unsigned _seed)
	{
		YPM::functionWrapper<Statement>(
			[](Statement* _message, YulRandomNumGenerator& _rand)
			{
				YPM::addArgs(_message, YPM::binopExpression, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Make statement arguments binary operations"
		);
	}
);

// Mutate varref
static YPR<VarRef> mutateVarRef(
	[](VarRef* _message, unsigned _seed)
	{
		YPM::functionWrapper<VarRef>(
			[](VarRef* _message, YulRandomNumGenerator& _rand)
			{
				_message->set_varnum(_rand());
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Mutate variable reference"
		);
	}
);

// Add load expression to statement
static YPR<Statement> addLoadToStmt(
	[](Statement* _message, unsigned _seed)
	{
		YPM::functionWrapper<Statement>(
			[](Statement* _message, YulRandomNumGenerator& _rand)
			{
				YPM::addArgs(_message, YPM::loadExpression, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Mutate statement arguments to a load expression"
		);
	}
);

// Add a randomly chosen statement to a statement block
static YPR<Block> addStmt(
	[](Block* _message, unsigned _seed)
	{
		YPM::functionWrapper<Block>(
			[](Block* _message, YulRandomNumGenerator& _rand)
			{
				YPM::addStmt(_message, _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add pseudo randomly chosen statement type to statement block"
		);
	}
);

///
static YPR<Expression> mutateUnsetExpr(
	[](Expression* _message, unsigned _seed)
	{
		YPM::functionWrapper<Expression>(
			[](Expression* _message, YulRandomNumGenerator& _rand)
			{
				switch (_rand() % 5)
				{
				case 0:
					YPM::unsetExprMutator(
						_message,
						_rand,
						[](Expression* _message, YulRandomNumGenerator& _r)
						{
							auto tmp = YPM::binopExpression(_r);
							_message->CopyFrom(*tmp);
							delete tmp;
						}
					);
					break;
				case 1:
					YPM::unsetExprMutator(
						_message,
						_rand,
						[](Expression* _message, YulRandomNumGenerator& _r)
						{
						  auto tmp = YPM::refExpression(_r);
						  _message->CopyFrom(*tmp);
						  delete tmp;
						}
					);
					break;
				case 2:
					YPM::unsetExprMutator(
						_message,
						_rand,
						[](Expression* _message, YulRandomNumGenerator& _r)
						{
						  auto tmp = YPM::loadExpression(_r);
						  _message->CopyFrom(*tmp);
						  delete tmp;
						}
					);
					break;
				case 3:
					YPM::unsetExprMutator(
						_message,
						_rand,
						[](Expression* _message, YulRandomNumGenerator& _r)
						{
						  auto tmp = YPM::litExpression(_r);
						  _message->CopyFrom(*tmp);
						  delete tmp;
						}
					);
					break;
				case 4:
					YPM::unsetExprMutator(
						_message,
						_rand,
						[](Expression* _message, YulRandomNumGenerator& _r)
						{
						  auto tmp = YPM::loadFromZero(_r);
						  _message->CopyFrom(*tmp);
						  delete tmp;
						}
					);
					break;
				}
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Mutate unset expression"
		);
	}
);

/// Add statement to function
static YPR<FunctionDef> addStmtToFunction(
	[](FunctionDef* _message, unsigned _seed)
	{
		YPM::functionWrapper<FunctionDef>(
			[](FunctionDef* _message, YulRandomNumGenerator& _rand)
			{
			  YPM::addStmt(_message->mutable_block(), _rand);
			},
			_message,
			_seed,
			YPM::s_highIP,
			"Add pseudo randomly chosen statement type to function block"
		);
	}
);

void YPM::addArgs(
	Statement *_stmt,
	std::function<Expression *(YulRandomNumGenerator&)> _func,
	YulRandomNumGenerator& _rand
)
{
	switch (_stmt->stmt_oneof_case())
	{
	case Statement::kDecl:
		if (!_stmt->decl().has_expr() || !isSet(_stmt->decl().expr()))
			_stmt->mutable_decl()->set_allocated_expr(_func(_rand));
		break;
	case Statement::kAssignment:
		if (!_stmt->assignment().has_expr() || !isSet(_stmt->assignment().expr()))
			_stmt->mutable_assignment()->set_allocated_expr(_func(_rand));
		if (!_stmt->assignment().has_ref_id() || _stmt->assignment().ref_id().varnum() == 0)
			_stmt->mutable_assignment()->set_allocated_ref_id(varRef(_rand));
		break;
	case Statement::kIfstmt:
		if (!_stmt->ifstmt().has_cond() || !isSet(_stmt->ifstmt().cond()))
			_stmt->mutable_ifstmt()->set_allocated_cond(_func(_rand));
		break;
	case Statement::kStorageFunc:
		if (!_stmt->storage_func().has_loc() || !isSet(_stmt->storage_func().loc()))
			_stmt->mutable_storage_func()->set_allocated_loc(_func(_rand));
		if (!_stmt->storage_func().has_val() || !isSet(_stmt->storage_func().val()))
			_stmt->mutable_storage_func()->set_allocated_val(_func(_rand));
		break;
	case Statement::kBlockstmt:
		break;
	case Statement::kForstmt:
		if (!_stmt->forstmt().has_for_cond() || !isSet(_stmt->forstmt().for_cond()))
			_stmt->mutable_forstmt()->set_allocated_for_cond(_func(_rand));
		break;
	case Statement::kBoundedforstmt:
		break;
	case Statement::kSwitchstmt:
		if (!_stmt->switchstmt().has_switch_expr() || !isSet(_stmt->switchstmt().switch_expr()))
			_stmt->mutable_switchstmt()->set_allocated_switch_expr(_func(_rand));
		break;
	case Statement::kBreakstmt:
		break;
	case Statement::kContstmt:
		break;
	case Statement::kLogFunc:
		if (!_stmt->log_func().has_pos() || !isSet(_stmt->log_func().pos()))
			_stmt->mutable_log_func()->set_allocated_pos(_func(_rand));
		if (!_stmt->log_func().has_size() || !isSet(_stmt->log_func().size()))
			_stmt->mutable_log_func()->set_allocated_size(_func(_rand));
		if (!_stmt->log_func().has_t1() || !isSet(_stmt->log_func().t1()))
			_stmt->mutable_log_func()->set_allocated_t1(_func(_rand));
		if (!_stmt->log_func().has_t2() || !isSet(_stmt->log_func().t2()))
			_stmt->mutable_log_func()->set_allocated_t2(_func(_rand));
		if (!_stmt->log_func().has_t3() || !isSet(_stmt->log_func().t3()))
			_stmt->mutable_log_func()->set_allocated_t3(_func(_rand));
		if (!_stmt->log_func().has_t4() || !isSet(_stmt->log_func().t4()))
			_stmt->mutable_log_func()->set_allocated_t4(_func(_rand));
		break;
	case Statement::kCopyFunc:
		if (!_stmt->copy_func().has_target() || !isSet(_stmt->copy_func().target()))
			_stmt->mutable_copy_func()->set_allocated_target(_func(_rand));
		if (!_stmt->copy_func().has_source() || !isSet(_stmt->copy_func().source()))
			_stmt->mutable_copy_func()->set_allocated_source(_func(_rand));
		if (!_stmt->copy_func().has_size() || !isSet(_stmt->copy_func().size()))
			_stmt->mutable_copy_func()->set_allocated_size(_func(_rand));
		break;
	case Statement::kExtcodeCopy:
		if (!_stmt->extcode_copy().has_addr() || !isSet(_stmt->extcode_copy().addr()))
			_stmt->mutable_extcode_copy()->set_allocated_addr(_func(_rand));
		if (!_stmt->extcode_copy().has_target() || !isSet(_stmt->extcode_copy().target()))
			_stmt->mutable_extcode_copy()->set_allocated_target(_func(_rand));
		if (!_stmt->extcode_copy().has_source() || !isSet(_stmt->extcode_copy().source()))
			_stmt->mutable_extcode_copy()->set_allocated_source(_func(_rand));
		if (!_stmt->extcode_copy().has_size() || !isSet(_stmt->extcode_copy().size()))
			_stmt->mutable_extcode_copy()->set_allocated_size(_func(_rand));
		break;
	case Statement::kTerminatestmt:
		break;
	case Statement::kFunctioncall:
		if (!_stmt->functioncall().has_in_param1() || !isSet(_stmt->functioncall().in_param1()))
			_stmt->mutable_functioncall()->set_allocated_in_param1(_func(_rand));
		if (!_stmt->functioncall().has_in_param2() || !isSet(_stmt->functioncall().in_param2()))
			_stmt->mutable_functioncall()->set_allocated_in_param2(_func(_rand));
		if (!_stmt->functioncall().has_in_param3() || !isSet(_stmt->functioncall().in_param3()))
			_stmt->mutable_functioncall()->set_allocated_in_param3(_func(_rand));
		if (!_stmt->functioncall().has_in_param4() || !isSet(_stmt->functioncall().in_param4()))
			_stmt->mutable_functioncall()->set_allocated_in_param4(_func(_rand));
		if (!_stmt->functioncall().has_out_param1() || _stmt->functioncall().out_param1().varnum() == 0)
			_stmt->mutable_functioncall()->set_allocated_out_param1(varRef(_rand));
		if (!_stmt->functioncall().has_out_param2() || _stmt->functioncall().out_param2().varnum() == 0)
			_stmt->mutable_functioncall()->set_allocated_out_param2(varRef(_rand));
		if (!_stmt->functioncall().has_out_param3() || _stmt->functioncall().out_param3().varnum() == 0)
			_stmt->mutable_functioncall()->set_allocated_out_param3(varRef(_rand));
		if (!_stmt->functioncall().has_out_param4() || _stmt->functioncall().out_param4().varnum() == 0)
			_stmt->mutable_functioncall()->set_allocated_out_param4(varRef(_rand));
		break;
	case Statement::kFuncdef:
		break;
	case Statement::kPop:
		if (!_stmt->pop().has_expr() || !isSet(_stmt->pop().expr()))
			_stmt->mutable_pop()->set_allocated_expr(_func(_rand));
		break;
	case Statement::kLeave:
		break;
	case Statement::kMultidecl:
		break;
	case Statement::STMT_ONEOF_NOT_SET:
		break;
	}
}

void YPM::addArgsRec(
	Statement *_stmt,
	std::function<void(Expression*, YulRandomNumGenerator& _rand)> _mutator,
	YulRandomNumGenerator& _rand
)
{
	switch (_stmt->stmt_oneof_case())
	{
	case Statement::kDecl:
		_mutator(_stmt->mutable_decl()->mutable_expr(), _rand);
		break;
	case Statement::kAssignment:
		_stmt->mutable_assignment()->mutable_ref_id()->set_varnum(_rand());
		_mutator(_stmt->mutable_assignment()->mutable_expr(), _rand);
		break;
	case Statement::kIfstmt:
		_mutator(_stmt->mutable_ifstmt()->mutable_cond(), _rand);
		break;
	case Statement::kStorageFunc:
		_mutator(_stmt->mutable_storage_func()->mutable_loc(), _rand);
		_mutator(_stmt->mutable_storage_func()->mutable_val(), _rand);
		break;
	case Statement::kBlockstmt:
		break;
	case Statement::kForstmt:
		_mutator(_stmt->mutable_forstmt()->mutable_for_cond(), _rand);
		break;
	case Statement::kBoundedforstmt:
		break;
	case Statement::kSwitchstmt:
		_mutator(_stmt->mutable_switchstmt()->mutable_switch_expr(), _rand);
		break;
	case Statement::kBreakstmt:
		break;
	case Statement::kContstmt:
		break;
	case Statement::kLogFunc:
		_mutator(_stmt->mutable_log_func()->mutable_pos(), _rand);
		_mutator(_stmt->mutable_log_func()->mutable_size(), _rand);
		_mutator(_stmt->mutable_log_func()->mutable_t1(), _rand);
		_mutator(_stmt->mutable_log_func()->mutable_t2(), _rand);
		_mutator(_stmt->mutable_log_func()->mutable_t3(), _rand);
		_mutator(_stmt->mutable_log_func()->mutable_t4(), _rand);
		break;
	case Statement::kCopyFunc:
		_mutator(_stmt->mutable_copy_func()->mutable_target(), _rand);
		_mutator(_stmt->mutable_copy_func()->mutable_source(), _rand);
		_mutator(_stmt->mutable_copy_func()->mutable_size(), _rand);
		break;
	case Statement::kExtcodeCopy:
		_mutator(_stmt->mutable_extcode_copy()->mutable_addr(), _rand);
		_mutator(_stmt->mutable_extcode_copy()->mutable_target(), _rand);
		_mutator(_stmt->mutable_extcode_copy()->mutable_source(), _rand);
		_mutator(_stmt->mutable_extcode_copy()->mutable_size(), _rand);
		break;
	case Statement::kTerminatestmt:
		if (_stmt->terminatestmt().term_oneof_case() == TerminatingStmt::kRetRev)
		{
			_mutator(_stmt->mutable_terminatestmt()->mutable_ret_rev()->mutable_pos(), _rand);
			_mutator(_stmt->mutable_terminatestmt()->mutable_ret_rev()->mutable_size(), _rand);
		}
		else if (_stmt->terminatestmt().term_oneof_case() == TerminatingStmt::kSelfDes)
			_mutator(_stmt->mutable_terminatestmt()->mutable_self_des()->mutable_addr(), _rand);
		break;
	case Statement::kFunctioncall:
		_mutator(_stmt->mutable_functioncall()->mutable_in_param1(), _rand);
		_mutator(_stmt->mutable_functioncall()->mutable_in_param2(), _rand);
		_mutator(_stmt->mutable_functioncall()->mutable_in_param3(), _rand);
		_mutator(_stmt->mutable_functioncall()->mutable_in_param4(), _rand);
		_stmt->mutable_functioncall()->set_allocated_out_param1(varRef(_rand));
		_stmt->mutable_functioncall()->set_allocated_out_param2(varRef(_rand));
		_stmt->mutable_functioncall()->set_allocated_out_param3(varRef(_rand));
		_stmt->mutable_functioncall()->set_allocated_out_param4(varRef(_rand));
		break;
	case Statement::kFuncdef:
		break;
	case Statement::kPop:
		_mutator(_stmt->mutable_pop()->mutable_expr(), _rand);
		break;
	case Statement::kLeave:
		break;
	case Statement::kMultidecl:
		break;
	case Statement::STMT_ONEOF_NOT_SET:
		break;
	}
}

void YPM::addStmt(Block* _block, YulRandomNumGenerator& _rand)
{
	auto stmt = _block->add_statements();
	switch ((_rand() / 17) % 19)
	{
	case 0:
		stmt->set_allocated_decl(new VarDecl());
		break;
	case 1:
		stmt->set_allocated_assignment(new AssignmentStatement());
		break;
	case 2:
		stmt->set_allocated_ifstmt(new IfStmt());
		break;
	case 3:
		stmt->set_allocated_storage_func(new StoreFunc());
		break;
	case 4:
		stmt->set_allocated_blockstmt(new Block());
		break;
	case 5:
		stmt->set_allocated_forstmt(new ForStmt());
		break;
	case 6:
		stmt->set_allocated_switchstmt(new SwitchStmt());
		break;
	case 7:
		stmt->set_allocated_breakstmt(new BreakStmt());
		break;
	case 8:
		stmt->set_allocated_contstmt(new ContinueStmt());
		break;
	case 9:
		stmt->set_allocated_log_func(new LogFunc());
		break;
	case 10:
		stmt->set_allocated_copy_func(new CopyFunc());
		break;
	case 11:
		stmt->set_allocated_extcode_copy(new ExtCodeCopy());
		break;
	case 12:
		stmt->set_allocated_terminatestmt(new TerminatingStmt());
		break;
	case 13:
		stmt->set_allocated_functioncall(new FunctionCall());
		break;
	case 14:
		stmt->set_allocated_boundedforstmt(new BoundedForStmt());
		break;
	case 15:
		stmt->set_allocated_funcdef(new FunctionDef());
		break;
	case 16:
		stmt->set_allocated_pop(new PopStmt());
		break;
	case 17:
		stmt->set_allocated_leave(new LeaveStmt());
		break;
	case 18:
		stmt->set_allocated_multidecl(new MultiVarDecl());
		break;
	}
}

Literal* YPM::intLiteral(unsigned _value)
{
	auto lit = new Literal();
	lit->set_intval(_value);
	return lit;
}

Expression* YPM::litExpression(YulRandomNumGenerator& _rand)
{
	auto expr = new Expression();
	expr->set_allocated_cons(intLiteral(_rand()));
	return expr;
}

VarRef* YPM::varRef(YulRandomNumGenerator& _rand)
{
	auto varref = new VarRef();
	varref->set_varnum(_rand());
	return varref;
}

Expression* YPM::refExpression(YulRandomNumGenerator& _rand)
{
	auto refExpr = new Expression();
	refExpr->set_allocated_varref(varRef(_rand));
	return refExpr;
}

void YPM::configureCall(FunctionCall *_call, YulRandomNumGenerator& _rand)
{
	_call->set_func_index(_rand());
	configureCallArgs(_call, _rand);
}

void YPM::configureCallArgs(
	FunctionCall *_call,
	YulRandomNumGenerator& _rand
)
{
	_call->set_allocated_out_param4(YPM::varRef(_rand));
	_call->set_allocated_out_param3(YPM::varRef(_rand));
	_call->set_allocated_out_param2(YPM::varRef(_rand));
	_call->set_allocated_out_param1(YPM::varRef(_rand));
	auto inArg4 = new Expression();
	inArg4->set_allocated_varref(YPM::varRef(_rand));
	_call->set_allocated_in_param4(inArg4);

	auto inArg3 = new Expression();
	inArg3->set_allocated_varref(YPM::varRef(_rand));
	_call->set_allocated_in_param3(inArg3);

	auto inArg2 = new Expression();
	inArg2->set_allocated_varref(YPM::varRef(_rand));
	_call->set_allocated_in_param2(inArg2);

	auto inArg1 = new Expression();
	inArg1->set_allocated_varref(YPM::varRef(_rand));
	_call->set_allocated_in_param1(inArg1);
}

template <typename T>
T YPM::EnumTypeConverter<T>::validEnum(unsigned _seed)
{
	auto ret = static_cast<T>(_seed % (enumMax() - enumMin() + 1) + enumMin());
	if constexpr (std::is_same_v<std::decay_t<T>, StoreFunc_Storage>)
		yulAssert(StoreFunc_Storage_IsValid(ret), "Yul proto mutator: Invalid enum");
	else if constexpr (std::is_same_v<std::decay_t<T>, NullaryOp_NOp>)
		yulAssert(NullaryOp_NOp_IsValid(ret), "Yul proto mutator: Invalid enum");
	else if constexpr (std::is_same_v<std::decay_t<T>, BinaryOp_BOp>)
		yulAssert(BinaryOp_BOp_IsValid(ret), "Yul proto mutator: Invalid enum");
	else if constexpr (std::is_same_v<std::decay_t<T>, UnaryOp_UOp>)
		yulAssert(UnaryOp_UOp_IsValid(ret), "Yul proto mutator: Invalid enum");
	else if constexpr (std::is_same_v<std::decay_t<T>, LowLevelCall_Type>)
		yulAssert(LowLevelCall_Type_IsValid(ret), "Yul proto mutator: Invalid enum");
	else if constexpr (std::is_same_v<std::decay_t<T>, Create_Type>)
		yulAssert(Create_Type_IsValid(ret), "Yul proto mutator: Invalid enum");
	else if constexpr (std::is_same_v<std::decay_t<T>, UnaryOpData_UOpData>)
		yulAssert(UnaryOpData_UOpData_IsValid(ret), "Yul proto mutator: Invalid enum");
	else
		static_assert(AlwaysFalse<T>::value, "Yul proto mutator: non-exhaustive visitor.");
	return ret;
}

template <typename T>
int YPM::EnumTypeConverter<T>::enumMax()
{
	if constexpr (std::is_same_v<std::decay_t<T>, StoreFunc_Storage>)
		return StoreFunc_Storage_Storage_MAX;
	else if constexpr (std::is_same_v<std::decay_t<T>, NullaryOp_NOp>)
		return NullaryOp_NOp_NOp_MAX;
	else if constexpr (std::is_same_v<std::decay_t<T>, BinaryOp_BOp>)
		return BinaryOp_BOp_BOp_MAX;
	else if constexpr (std::is_same_v<std::decay_t<T>, UnaryOp_UOp>)
		return UnaryOp_UOp_UOp_MAX;
	else if constexpr (std::is_same_v<std::decay_t<T>, LowLevelCall_Type>)
		return LowLevelCall_Type_Type_MAX;
	else if constexpr (std::is_same_v<std::decay_t<T>, Create_Type>)
		return Create_Type_Type_MAX;
	else if constexpr (std::is_same_v<std::decay_t<T>, UnaryOpData_UOpData>)
		return UnaryOpData_UOpData_UOpData_MAX;
	else
		static_assert(AlwaysFalse<T>::value, "Yul proto mutator: non-exhaustive visitor.");
}

template <typename T>
int YPM::EnumTypeConverter<T>::enumMin()
{
	if constexpr (std::is_same_v<std::decay_t<T>, StoreFunc_Storage>)
		return StoreFunc_Storage_Storage_MIN;
	else if constexpr (std::is_same_v<std::decay_t<T>, NullaryOp_NOp>)
		return NullaryOp_NOp_NOp_MIN;
	else if constexpr (std::is_same_v<std::decay_t<T>, BinaryOp_BOp>)
		return BinaryOp_BOp_BOp_MIN;
	else if constexpr (std::is_same_v<std::decay_t<T>, UnaryOp_UOp>)
		return UnaryOp_UOp_UOp_MIN;
	else if constexpr (std::is_same_v<std::decay_t<T>, LowLevelCall_Type>)
		return LowLevelCall_Type_Type_MIN;
	else if constexpr (std::is_same_v<std::decay_t<T>, Create_Type>)
		return Create_Type_Type_MIN;
	else if constexpr (std::is_same_v<std::decay_t<T>, UnaryOpData_UOpData>)
		return UnaryOpData_UOpData_UOpData_MIN;
	else
		static_assert(AlwaysFalse<T>::value, "Yul proto mutator: non-exhaustive visitor.");
}

Expression* YPM::loadExpression(YulRandomNumGenerator& _rand)
{
	auto unop = new UnaryOp();
	unop->set_allocated_operand(refExpression(_rand));
	switch (_rand() % 3)
	{
	case 0:
		unop->set_op(UnaryOp::MLOAD);
		break;
	case 1:
		unop->set_op(UnaryOp::SLOAD);
		break;
	case 2:
		unop->set_op(UnaryOp::CALLDATALOAD);
		break;
	}
	auto expr = new Expression();
	expr->set_allocated_unop(unop);
	return expr;
}

Expression* YPM::loadFromZero(YulRandomNumGenerator& _rand)
{
	auto unop = new UnaryOp();
	unop->set_allocated_operand(litExpression(_rand));
	switch (_rand() % 3)
	{
	case 0:
		unop->set_op(UnaryOp::MLOAD);
		break;
	case 1:
		unop->set_op(UnaryOp::SLOAD);
		break;
	case 2:
		unop->set_op(UnaryOp::CALLDATALOAD);
		break;
	}
	auto expr = new Expression();
	expr->set_allocated_unop(unop);
	return expr;
}

void YPM::clearExpr(Expression* _expr)
{
	switch (_expr->expr_oneof_case())
	{
	case Expression::kVarref:
		delete _expr->release_varref();
		_expr->clear_varref();
		break;
	case Expression::kCons:
		delete _expr->release_cons();
		_expr->clear_cons();
		break;
	case Expression::kBinop:
		delete _expr->release_binop();
		_expr->clear_binop();
		break;
	case Expression::kUnop:
		delete _expr->release_unop();
		_expr->clear_unop();
		break;
	case Expression::kTop:
		delete _expr->release_top();
		_expr->clear_top();
		break;
	case Expression::kNop:
		delete _expr->release_nop();
		_expr->clear_nop();
		break;
	case Expression::kLowcall:
		delete _expr->release_lowcall();
		_expr->clear_lowcall();
		break;
	case Expression::kCreate:
		delete _expr->release_create();
		_expr->clear_create();
		break;
	case Expression::kUnopdata:
		delete _expr->release_unopdata();
		_expr->clear_unopdata();
		break;
	case Expression::kFuncexpr:
		delete _expr->release_funcexpr();
		_expr->clear_funcexpr();
		break;
	case Expression::EXPR_ONEOF_NOT_SET:
		break;
	}
}

Expression* YPM::binopExpression(YulRandomNumGenerator& _rand)
{
	auto binop = new BinaryOp();
	binop->set_allocated_left(refExpression(_rand));
	binop->set_allocated_right(refExpression(_rand));
	binop->set_op(
		YPM::EnumTypeConverter<BinaryOp_BOp>{}.enumFromSeed(_rand())
	);
	auto expr = new Expression();
	expr->set_allocated_binop(binop);
	return expr;
}

void YPM::unsetExprMutator(
	Expression* _expr,
	YulRandomNumGenerator& _rand,
	std::function<void(Expression*, YulRandomNumGenerator&)> _mutateExprFunc
)
{
	switch (_expr->expr_oneof_case())
	{
	case Expression::kVarref:
		if (_expr->varref().varnum() == 0)
			_expr->mutable_varref()->set_varnum(_rand());
		break;
	case Expression::kCons:
		if (_expr->cons().literal_oneof_case() == Literal::LITERAL_ONEOF_NOT_SET)
			_expr->mutable_cons()->set_intval(_rand());
		break;
	case Expression::kBinop:
		if (!isSet(_expr->binop().left()))
			_mutateExprFunc(_expr->mutable_binop()->mutable_left(), _rand);
		else
			unsetExprMutator(_expr->mutable_binop()->mutable_left(), _rand, _mutateExprFunc);

		if (!isSet(_expr->binop().right()))
			_mutateExprFunc(_expr->mutable_binop()->mutable_right(), _rand);
		else
			unsetExprMutator(_expr->mutable_binop()->mutable_right(), _rand, _mutateExprFunc);
		break;
	case Expression::kUnop:
		if (!isSet(_expr->unop().operand()))
			_mutateExprFunc(_expr->mutable_unop()->mutable_operand(), _rand);
		else
			unsetExprMutator(_expr->mutable_unop()->mutable_operand(), _rand, _mutateExprFunc);
		break;
	case Expression::kTop:
		if (!isSet(_expr->top().arg1()))
			_mutateExprFunc(_expr->mutable_top()->mutable_arg1(), _rand);
		else
			unsetExprMutator(_expr->mutable_top()->mutable_arg1(), _rand, _mutateExprFunc);

		if (!isSet(_expr->top().arg2()))
			_mutateExprFunc(_expr->mutable_top()->mutable_arg2(), _rand);
		else
			unsetExprMutator(_expr->mutable_top()->mutable_arg2(), _rand, _mutateExprFunc);

		if (!isSet(_expr->top().arg3()))
			_mutateExprFunc(_expr->mutable_top()->mutable_arg3(), _rand);
		else
			unsetExprMutator(_expr->mutable_top()->mutable_arg3(), _rand, _mutateExprFunc);
		break;
	case Expression::kNop:
		break;
	case Expression::kLowcall:
		// Wei
		if (_expr->lowcall().callty() == LowLevelCall::CALLCODE || _expr->lowcall().callty() == LowLevelCall::CALL)
		{
			if (!isSet(_expr->lowcall().wei()))
				_mutateExprFunc(_expr->mutable_lowcall()->mutable_wei(), _rand);
			else
				unsetExprMutator(_expr->mutable_lowcall()->mutable_wei(), _rand,
				                 _mutateExprFunc);
		}

		// Gas
		if (!isSet(_expr->lowcall().gas()))
			_mutateExprFunc(_expr->mutable_lowcall()->mutable_gas(), _rand);
		else
			unsetExprMutator(_expr->mutable_lowcall()->mutable_gas(), _rand, _mutateExprFunc);

		// Addr
		if (!isSet(_expr->lowcall().addr()))
			_mutateExprFunc(_expr->mutable_lowcall()->mutable_addr(), _rand);
		else
			unsetExprMutator(_expr->mutable_lowcall()->mutable_addr(), _rand, _mutateExprFunc);

		// In
		if (!isSet(_expr->lowcall().in()))
			_mutateExprFunc(_expr->mutable_lowcall()->mutable_in(), _rand);
		else
			unsetExprMutator(_expr->mutable_lowcall()->mutable_in(), _rand, _mutateExprFunc);
		// Insize
		if (!isSet(_expr->lowcall().insize()))
			_mutateExprFunc(_expr->mutable_lowcall()->mutable_insize(), _rand);
		else
			unsetExprMutator(_expr->mutable_lowcall()->mutable_insize(), _rand,
			                 _mutateExprFunc);
		// Out
		if (!isSet(_expr->lowcall().out()))
			_mutateExprFunc(_expr->mutable_lowcall()->mutable_out(), _rand);
		else
			unsetExprMutator(_expr->mutable_lowcall()->mutable_out(), _rand, _mutateExprFunc);
		// Outsize
		if (!isSet(_expr->lowcall().outsize()))
			_mutateExprFunc(_expr->mutable_lowcall()->mutable_outsize(), _rand);
		else
			unsetExprMutator(_expr->mutable_lowcall()->mutable_outsize(), _rand,
			                 _mutateExprFunc);
		break;
	case Expression::kCreate:
		// Value
		if (_expr->create().createty() == Create_Type::Create_Type_CREATE2)
		{
			if (!isSet(_expr->create().value()))
				_mutateExprFunc(_expr->mutable_create()->mutable_value(), _rand);
			else
				unsetExprMutator(_expr->mutable_create()->mutable_value(), _rand,
				                 _mutateExprFunc);
		}
		// Wei
		if (!isSet(_expr->create().wei()))
			_mutateExprFunc(_expr->mutable_create()->mutable_wei(), _rand);
		else
			unsetExprMutator(_expr->mutable_create()->mutable_wei(), _rand, _mutateExprFunc);
		// Position
		if (!isSet(_expr->create().position()))
			_mutateExprFunc(_expr->mutable_create()->mutable_position(), _rand);
		else
			unsetExprMutator(_expr->mutable_create()->mutable_position(), _rand,
			                 _mutateExprFunc);
		// Size
		if (!isSet(_expr->create().size()))
			_mutateExprFunc(_expr->mutable_create()->mutable_size(), _rand);
		else
			unsetExprMutator(_expr->mutable_create()->mutable_size(), _rand, _mutateExprFunc);
		break;
	case Expression::kUnopdata:
		break;
	case Expression::kFuncexpr:
		if (_expr->funcexpr().index() == 0)
			_expr->mutable_funcexpr()->set_index(_rand());
		if (!isSet(_expr->funcexpr().in_param1()))
			_mutateExprFunc(_expr->mutable_funcexpr()->mutable_in_param1(), _rand);
		else
			unsetExprMutator(_expr->mutable_funcexpr()->mutable_in_param1(), _rand, _mutateExprFunc);
		if (!isSet(_expr->funcexpr().in_param2()))
			_mutateExprFunc(_expr->mutable_funcexpr()->mutable_in_param2(), _rand);
		else
			unsetExprMutator(_expr->mutable_funcexpr()->mutable_in_param2(), _rand, _mutateExprFunc);
		if (!isSet(_expr->funcexpr().in_param3()))
			_mutateExprFunc(_expr->mutable_funcexpr()->mutable_in_param3(), _rand);
		else
			unsetExprMutator(_expr->mutable_funcexpr()->mutable_in_param3(), _rand, _mutateExprFunc);
		if (!isSet(_expr->funcexpr().in_param4()))
			_mutateExprFunc(_expr->mutable_funcexpr()->mutable_in_param4(), _rand);
		else
			unsetExprMutator(_expr->mutable_funcexpr()->mutable_in_param4(), _rand, _mutateExprFunc);
		break;
	case Expression::EXPR_ONEOF_NOT_SET:
		_mutateExprFunc(_expr, _rand);
		break;
	}
}