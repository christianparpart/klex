#include <klex/SourceLocation.h>
#include <fstream>

using namespace std;

namespace klex {

std::string SourceLocation::source() const // TODO
{
	ifstream ifs(filename);
	ifs.seekg(offset, ifs.beg);
	string code;
	code.resize(count);
	ifs.read(&code[0], count);
	return move(code);
}

}  // namespace klex
