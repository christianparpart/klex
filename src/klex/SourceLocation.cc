#include <klex/SourceLocation.h>
#include <fstream>

using namespace std;

namespace klex {

std::string SourceLocation::source() const // TODO
{
	string code;
	ifstream ifs(filename);
	ifs.seekg(offset, ifs.beg);
	code.resize(count);
	ifs.read(&code[0], count);
	return code;
}

}  // namespace klex
