#include <test/tools/fuzzer.h>
#include <test/tools/constopt.h>

int main(int argc, char** argv)
{
	po::options_description options(
		R"(solfuzzer, fuzz-testing binary for use with AFL.
Usage: solfuzzer [Options] < input
Reads a single source from stdin, compiles it and signals a failure for internal errors.

Allowed options)",
		po::options_description::m_default_line_length,
		po::options_description::m_default_line_length - 23);
	options.add_options()
		("help", "Show this help screen.")
		("quiet", "Only output errors.")
		(
			"standard-json",
			"Test via the standard-json interface, i.e. "
			"input is expected to be JSON-encoded instead of "
			"plain source file."
		)
		(
			"const-opt",
			"Run the constant optimizer instead of compiling. "
			"Expects a binary string of up to 32 bytes on stdin."
		)
		(
			"input-file",
			po::value<string>(),
			"input file"
		)
		(
			"without-optimizer",
			"Run without optimizations. Cannot be used together with standard-json."
		);

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
		input = readFileAsString(arguments["input-file"].as<string>());
	else
		input = readStandardInput();

	if (arguments.count("quiet"))
		quiet = true;

	if (arguments.count("help"))
		cout << options;
	else if (arguments.count("const-opt"))
		testConstantOptimizer(input);
	else if (arguments.count("standard-json"))
		testStandardCompiler(input);
	else
		testCompiler(input, !arguments.count("without-optimizer"));

	return 0;
}
