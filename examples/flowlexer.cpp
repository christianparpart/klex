// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/regular/Lexable.h>
#include <fmt/format.h>
#include <fstream>
#include <iostream>

#include "token.h"  // generated via mklex

extern klex::regular::LexerDef lexerDef;  // generated via mklex

int main(int argc, const char* argv[])
{
	auto ls = argc == 2
				  ? klex::regular::Lexable<Token, Machine>{lexerDef, std::make_unique<std::ifstream>(argv[1])}
				  : klex::regular::Lexable<Token, Machine>{lexerDef, std::cin};

	for (const auto& token : ls)
	{
		std::cerr << fmt::format("[{}-{}]: token {} (\"{}\")\n",
								 token.offset,
								 token.offset + token.literal.length(),
								 lexerDef.tagName(static_cast<klex::regular::Tag>(token.token)),
								 token.literal);
	}

	return EXIT_SUCCESS;
}
