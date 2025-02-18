#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/printf.h>

#include <howler/howler.hpp>

#include "token.hpp"
#include "grammar.hpp"

int main()
{
	std::string source = R"(
	reference
	
	from std use Real

	$(a + b = a + (b + c))
	
	$(a + b = b + a) :: { $a in Real, $b in Real }

	statement = $(a + b = b + a)
	predicate = { $a in Real, $b in Real }

	statement :: predicate
	)";

	size_t index;
	
	index = 0;
	auto tokens = oxidius_lexer(source, index).value();

	howl_info("tokens: ");
	for (auto &t : tokens)
		fmt::print("{} ", t);
	fmt::print("\n");

	index = 0;
	while (auto v = instruction(tokens, index))
		howl_info("instruction[{}], index={}, next={}", v.value().index(), index, tokens[index]);
}