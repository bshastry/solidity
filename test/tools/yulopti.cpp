#include "yulopti.h"

int main(int argc, char** argv)
{
	po::options_description options(
		R"(yulopti, yul optimizer exploration tool.
Usage: yulopti [Options] <file>
Reads <file> as yul code and applies optimizer steps to it,
interactively read from stdin.

Allowed options)",
		po::options_description::m_default_line_length,
		po::options_description::m_default_line_length - 23);
	options.add_options()
		(
			"input-file",
			po::value<string>(),
			"input file"
		)
		("help", "Show this help screen.");

	// All positional options should be interpreted as input files
	po::positional_options_description filesPositions;
	filesPositions.add("input-file", 1);

	po::variables_map arguments;
	try
	{
		po::command_line_parser cmdLineParser(argc, argv);
		cmdLineParser.options(options).positional(filesPositions);
		po::store(cmdLineParser.run(), arguments);
	}
	catch (po::error const& _exception)
	{
		cerr << _exception.what() << endl;
		return 1;
	}

	string input;
	if (arguments.count("input-file"))
		YulOpti{}.runInteractive(readFileAsString(arguments["input-file"].as<string>()));
	else
		cout << options;

	return 0;
}
