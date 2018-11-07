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
/**
 * Interactive yul optimizer
 */

#include <libdevcore/CommonIO.h>
#include <libsolidity/inlineasm/AsmAnalysis.h>
#include <libsolidity/inlineasm/AsmAnalysisInfo.h>
#include <libsolidity/parsing/Scanner.h>
#include <libsolidity/parsing/Parser.h>
#include <libsolidity/inlineasm/AsmData.h>
#include <libsolidity/inlineasm/AsmParser.h>
#include <libsolidity/inlineasm/AsmPrinter.h>
#include <libsolidity/interface/SourceReferenceFormatter.h>
#include <libsolidity/interface/ErrorReporter.h>

#include <libyul/optimiser/BlockFlattener.h>
#include <libyul/optimiser/Disambiguator.h>
#include <libyul/optimiser/CommonSubexpressionEliminator.h>
#include <libyul/optimiser/NameCollector.h>
#include <libyul/optimiser/ExpressionSplitter.h>
#include <libyul/optimiser/FunctionGrouper.h>
#include <libyul/optimiser/FunctionHoister.h>
#include <libyul/optimiser/ExpressionInliner.h>
#include <libyul/optimiser/FullInliner.h>
#include <libyul/optimiser/MainFunction.h>
#include <libyul/optimiser/Rematerialiser.h>
#include <libyul/optimiser/ExpressionSimplifier.h>
#include <libyul/optimiser/UnusedPruner.h>
#include <libyul/optimiser/ExpressionJoiner.h>
#include <libyul/optimiser/RedundantAssignEliminator.h>
#include <libyul/optimiser/SSATransform.h>

#include <libdevcore/JSON.h>

#include <string>
#include <sstream>
#include <iostream>

using namespace std;
using namespace dev;
using namespace dev::solidity;
using namespace dev::solidity::assembly;
using namespace dev::yul;

class YulOpti
{
public:
	void printErrors(Scanner const& _scanner)
	{
		SourceReferenceFormatter formatter(cout, [&](string const&) -> Scanner const& { return _scanner; });

		for (auto const& error: m_errors)
			formatter.printExceptionInformation(
				*error,
				(error->type() == Error::Type::Warning) ? "Warning" : "Error"
			);
	}

	bool parse(string const& _input)
	{
		ErrorReporter errorReporter(m_errors);
		shared_ptr<Scanner> scanner = make_shared<Scanner>(CharStream(_input), "");
		m_ast = assembly::Parser(errorReporter, assembly::AsmFlavour::Strict).parse(scanner, false);
		if (!m_ast || !errorReporter.errors().empty())
		{
			cout << "Error parsing source." << endl;
			printErrors(*scanner);
			return false;
		}
		m_analysisInfo = make_shared<assembly::AsmAnalysisInfo>();
		AsmAnalyzer analyzer(
			*m_analysisInfo,
			errorReporter,
			EVMVersion::byzantium(),
			boost::none,
			AsmFlavour::Strict
		);
		if (!analyzer.analyze(*m_ast) || !errorReporter.errors().empty())
		{
			cout << "Error analyzing source." << endl;
			printErrors(*scanner);
			return false;
		}
		return true;
	}

	void runNonInteractive(string source)
	{
		if (!parse(source))
			return;

		*m_ast = boost::get<assembly::Block>(Disambiguator(*m_analysisInfo)(*m_ast));
		m_analysisInfo.reset();
		m_nameDispenser = make_shared<NameDispenser>(*m_ast);
		size_t source_len = source.length();

		switch (source_len % 12)
		{
			case 0:
				BlockFlattener{}(*m_ast);
				break;
			case 1:
				(CommonSubexpressionEliminator{})(*m_ast);
				break;
			case 2:
				ExpressionSplitter{*m_nameDispenser}(*m_ast);
				break;
			case 3:
				ExpressionJoiner::run(*m_ast);
				break;
			case 4:
				(FunctionGrouper{})(*m_ast);
				break;
			case 5:
				(FunctionHoister{})(*m_ast);
				break;
			case 6:
				ExpressionInliner{*m_ast}.run();
				break;
			case 7:
				FullInliner(*m_ast, *m_nameDispenser).run();
				break;
			case 8:
				ExpressionSimplifier::run(*m_ast);
				break;
			case 9:
				UnusedPruner::runUntilStabilised(*m_ast);
				break;
			case 10:
				SSATransform::run(*m_ast, *m_nameDispenser);
				break;
			case 11:
				RedundantAssignEliminator::run(*m_ast);
				break;
			default:
				cout << "Unknown option." << endl;
		}
		source = AsmPrinter{}(*m_ast);
	}

private:
	ErrorList m_errors;
	shared_ptr<assembly::Block> m_ast;
	shared_ptr<AsmAnalysisInfo> m_analysisInfo;
	shared_ptr<NameDispenser> m_nameDispenser;
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    string input(reinterpret_cast<const char*>(data), size);
    YulOpti{}.runNonInteractive(input);
    return 0;
}
