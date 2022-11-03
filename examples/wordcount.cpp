// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/regular/Compiler.h>
#include <klex/regular/Lexable.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

int main(int argc, const char* argv[])
{
    klex::regular::Compiler cc;
    cc.parse(R"(
		Word  ::= [a-zA-Z]+
		LF    ::= \n
		Other ::= .
		Eof   ::= <<EOF>>
	)");

    size_t words = 0;
    size_t chars = 0;
    size_t lines = 0;

    auto ld = cc.compile();
    klex::regular::Lexable<int, int, false, false> lexer { ld, std::cin };
    for (const auto& ti: lexer)
    {
        switch (token(ti))
        {
            case 4: // EOF
                break;
            case 3: // Other
                chars++;
                break;
            case 2: // LF
                chars++;
                lines++;
                break;
            case 1: // Word
                words++;
                chars += literal(ti).size();
                break;
        }
    }

    std::cout << "newlines: " << lines << ", words: " << words << ", characters: " << chars << "\n";

    return EXIT_SUCCESS;
}
