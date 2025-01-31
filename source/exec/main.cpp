#include <string>
#include <vector>
#include <optional>

#include <fmt/format.h>
#include <fmt/printf.h>

#include <howler.hpp>

#include "token.hpp"

// Parsing constructions

// Mathematical variable
struct PureVariable : std::string {};

struct PureFactor {};

struct PureTerm {};

struct PureExpression {};

struct PureStatement {
	PureExpression lhs;
	PureExpression rhs;
};

// Programming variable
struct Reference : std::string {};

using Variable = variant <PureVariable, Reference>;
using Expression = variant <PureExpression, Reference>;
using Statement = variant <PureStatement, Reference>;

struct PurePredicateDomain {
	Variable var;
	Expression domain;
};

struct PurePredicates {
	std::vector <PurePredicateDomain> domains;
};

using Predicates = variant <PurePredicates, Reference>;

struct Import {
	std::string package;
	// TODO: vector
	std::string items;
};

// Variable values
// TODO: options(...)
struct Value : variant <PurePredicates, Statement> {};

struct Assignment {
	Reference destination;
	Value value;
};

struct Association {
	// TODO: vector
	Statement statement;
	Predicates predicates;
};

#define PARSER(F, T) static std::optional <T> F(const std::vector <Token> &tokens, size_t &i)

struct Parser : TokenParser <Token> {
	PARSER(reference, Reference) {
		if (auto r = singlet <identifier> (tokens, i)) {
			auto v = r.value();
			return Reference(v);
		}

		return std::nullopt;
	}

	PARSER(pure_variable, PureVariable) {
		if (tokens[i].is <identifier> ()) {
			std::string s = tokens[i++].as <identifier> ();
			return PureVariable(s);
		}

		return std::nullopt;
	}

	PARSER(pure_factor, PureFactor) {
		if (auto r = pure_variable(tokens, i)) {
			return PureFactor();
		}

		return std::nullopt;
	}

	PARSER(pure_term, PureTerm) {
		if (auto r = pure_factor(tokens, i)) {
			return PureTerm();
		}

		return std::nullopt;
	}

	PARSER(pure_expression, PureExpression) {
		if (auto r = chain(pure_term, sym_plus(), pure_term)(tokens, i)) {
			return PureExpression();
		}

		return std::nullopt;
	}

	PARSER(pure_statement, PureStatement) {
		if (auto r = chain(pure_expression,
				   sym_equals(),
				   pure_expression)(tokens, i)) {
			auto v = r.value();

			return PureStatement {
				.lhs = std::get <0> (v),
				.rhs = std::get <2> (v),
			};
		}

		return std::nullopt;
	}

	PARSER(variable, Variable) {
		if (auto r = reference(tokens, i)) {
			std::string s = r.value();
			return Reference(s);
		} else if (auto r = chain(sym_dollar(), pure_variable)(tokens, i)) {
			auto v = r.value();
			return std::get <1> (v);
		}

		return std::nullopt;
	}
	
	PARSER(expression, Expression) {
		if (auto r = reference(tokens, i)) {
			std::string s = r.value();
			return Reference(s);
		}

		return std::nullopt;
	}

	PARSER(statement, Statement) {
		if (auto r = reference(tokens, i)) {
			std::string s = r.value();
			return Reference(s);
		} else if (auto r = chain(sym_dollar(),
					  sym_left_paren(),
					  pure_statement,
					  sym_right_paren())(tokens, i)) {
			auto v = r.value();
			return std::get <2> (v);
		}

		return std::nullopt;
	}

	PARSER(pure_predicate_domain, PurePredicateDomain) {
		if (auto r = chain(variable, kwd_in(), expression)(tokens, i)) {
			auto v = r.value();

			return PurePredicateDomain {
				.var = std::get <0> (v),
				.domain = std::get <2> (v),
			};
		}

		return std::nullopt;
	}

	PARSER(pure_predicates, PurePredicates) {
		// TODO: special optional (bestd) with .feed(...) -> Value
		if (auto r = chain(sym_left_brace(),
				loop <true> (pure_predicate_domain, sym_comma()),
				sym_right_brace())(tokens, i)) {
			auto v = r.value();

			return PurePredicates {
				.domains = std::get <1> (v)
			};
		}

		return std::nullopt;
	}

	PARSER(import, Import) {
		if (auto r = chain(kwd_from(),
				   identifier(),
				   kwd_use(),
				   identifier())(tokens, i)) {
			auto v = r.value();

			return Import {
				.package = std::get <1> (v),
				.items = std::get <3> (v),
			};
		}

		return std::nullopt;
	}

	PARSER(value, Value) {
		// TODO: options
		if (auto r = pure_predicates(tokens, i)) {
			auto v = r.value();
			return Value(v);
		} else if (auto r = statement(tokens, i)) {
			auto v = r.value();
			return Value(v);
		}

		return std::nullopt;
	}

	PARSER(predicates, Predicates) {
		if (auto r = reference(tokens, i)) {
			auto v = r.value();
			return Predicates(v);
		} else if (auto r = pure_predicates(tokens, i)) {
			auto v = r.value();
			return Predicates(v);
		}

		return std::nullopt;
	}

	PARSER(assignment, Assignment) {
		if (auto r = chain(reference, sym_equals(), value)(tokens, i)) {
			auto v = r.value();
			return Assignment {
				.destination = std::get <0> (v),
				.value = std::get <2> (v),
			};
		}

		return std::nullopt;
	}

	PARSER(association, Association) {
		if (auto r = chain(statement, sym_double_colon(), predicates)(tokens, i)) {
			auto v = r.value();
			return Association {
				.statement = std::get <0> (v),
				.predicates = std::get <2> (v),
			};
		}

		return std::nullopt;
	}
};

int main()
{
	std::string source = R"(
	from std use Real
	
	$(a + b = b + a) :: { $a in Real, $b in Real }

	statement = $(a + b = b + a)
	predicate = { $a in Real, $b in Real }

	statement :: predicate
	)";

	size_t index;
	
	index = 0;
	auto tokens = oxidius_lexer(source, index).value();

	fmt::print("tokens: ");
	for (auto &t : tokens)
		fmt::print("{} ", t);
	fmt::print("\n");

	index = 0;

	auto import = Parser::import(tokens, index);
	
	fmt::println("index: {}, next={}", index, tokens[index]);

	fmt::println("\timport: {} <- {}", import->package, import->items);

	Parser::statement(tokens, index);
	
	fmt::println("index: {}, next={}", index, tokens[index]);

	Parser::singlet <sym_double_colon> (tokens, index);

	fmt::println("index: {}, next={}", index, tokens[index]);

	auto predicates = Parser::pure_predicates(tokens, index);

	if (predicates.has_value()) {
		fmt::println("domains ({}):", predicates->domains.size());
		for (auto &pd : predicates->domains) {
			fmt::println("\t{} -> {}", pd.var.as <PureVariable> (), pd.domain.as <Reference> ());
		}
	}

	fmt::println("index={}; next={}", index, tokens[index]);
	
	Parser::assignment(tokens, index);
	
	fmt::println("index: {}, next={}", index, tokens[index]);
	
	Parser::assignment(tokens, index);
	
	fmt::println("index: {}, next={}", index, tokens[index]);
	
	Parser::association(tokens, index);
	
	fmt::println("index: {}, next={} (end={})", index, tokens[index], index == tokens.size());
}