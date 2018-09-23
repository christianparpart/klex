#include <klex/Report.h>
#include <klex/cfg/GrammarParser.h>
#include <klex/cfg/LeftRecursionEliminator.h>
#include <klex/util/literals.h>
#include <klex/util/testing.h>

using namespace std;
using namespace klex;
using namespace klex::cfg;
using namespace klex::util::literals;

Grammar makeGrammar(string G)
{
	BufferedReport report;
	Grammar grammar = GrammarParser(move(G), &report).parse();
	ASSERT_FALSE(report.containsFailures());
	return move(grammar);
}

TEST(cfg_LeftRecursionEliminator, isLeftRecursive)
{
	BufferedReport report;

	// direct left-recursive
	const Grammar grammar = GrammarParser("A ::= A 'b' | 'a';", &report).parse();
	ASSERT_FALSE(report.containsFailures());
	ASSERT_TRUE(isLeftRecursive(grammar));

	// direct right recursive
	const Grammar right = GrammarParser("A ::= 'b' A | 'a';", &report).parse();
	ASSERT_FALSE(report.containsFailures());
	ASSERT_FALSE(isLeftRecursive(right));

	// neither left nor right
	const Grammar neinor = GrammarParser("A ::= 'b' | 'a';", &report).parse();
	ASSERT_FALSE(report.containsFailures());
	ASSERT_FALSE(isLeftRecursive(neinor));
}

TEST(cfg_LeftRecursionEliminator, simple)
{
	ConsoleReport report;
	Grammar grammar = GrammarParser(R"(`S ::= A;
									   `A ::= A 'b'
									   `    | 'a';
									   `)"_multiline,
									&report)
						  .parse();

	ASSERT_FALSE(report.containsFailures());
	ASSERT_TRUE(isLeftRecursive(grammar));

	LeftRecursionEliminator{grammar}.direct();

	grammar.finalize();
	logf("grammar: {}", grammar.dump());

	ASSERT_FALSE(isLeftRecursive(grammar));
}

TEST(cfg_LeftRecursionEliminator, ETF)
{
	BufferedReport report;
	Grammar grammar = GrammarParser(R"(`token {
									   `  Spacing(ignore) ::= [\s\t]+
									   `  Number          ::= [0-9]+
									   `}
									   `
									   `Start  ::= Expr;
									   `Expr   ::= Expr '+' Term
									   `         | Expr '-' Term
									   `         | Term ;
									   `Term   ::= Term '*' Factor
									   `         | Term '/' Factor
									   `         | Factor ;
									   `Factor ::= '(' Expr ')'
									   `         | Number
									   `         ;
									   `)"_multiline,
									&report)
						  .parse();

	ASSERT_FALSE(report.containsFailures());
	ASSERT_TRUE(isLeftRecursive(grammar));

	LeftRecursionEliminator{grammar}.direct();

	grammar.finalize();
	logf("grammar: {}", grammar.dump());

	ASSERT_FALSE(isLeftRecursive(grammar));
}
