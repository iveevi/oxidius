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

using Variable = bestd::variant <PureVariable, Reference>;
using Expression = bestd::variant <PureExpression, Reference>;
using Statement = bestd::variant <PureStatement, Reference>;

struct PurePredicateDomain {
	Variable var;
	Expression domain;
};

struct PurePredicates {
	std::vector <PurePredicateDomain> domains;
};

using Predicates = bestd::variant <PurePredicates, Reference>;

struct Import {
	std::string package;
	// TODO: vector
	std::string items;
};

// Variable values
// TODO: options(...)
struct Value : bestd::variant <PurePredicates, Statement> {};

struct Assignment {
	Reference destination;
	Value value;
};

struct Association {
	// TODO: vector
	Statement statement;
	Predicates predicates;
};

#define PARSER(F, T) static bestd::optional <T> F(const std::vector <Token> &tokens, size_t &i)

template <typename T, size_t ... Is, typename A>
T __construct(const A &arg)
{
	if constexpr (sizeof...(Is) > 0)
		return T(arg);
	else
		return T();
}

template <typename T, size_t ... Is, typename ... Args>
T __construct(const std::tuple <Args...> &arg)
{
	return T(std::get <Is> (arg)...);
}

template <typename T, size_t ... Is>
auto construct = [](const auto &arg)
{
	return __construct <T, Is...> (arg);
};

template <typename T, size_t ... Is>
auto feed_construct = [](const auto &arg)
{
	return arg.feed(construct <T, Is...>);
};

template <typename F, typename G>
constexpr auto compose(const F &f, const G &g)
{
	return [f, g](auto &&... args) -> decltype(auto) {
        	return std::invoke(f, std::invoke(g, std::forward <decltype(args)> (args)...));
	};
}

#define PRODUCTION(T) inline const std::function <bestd::optional <T> (const std::vector <Token> &, size_t &)>

struct Parser : TokenParser <Token> {
	static PRODUCTION(Reference) reference = compose(
		feed_construct <Reference, 0>,
		singlet <identifier>
	);
	
	static PRODUCTION(PureVariable) pure_variable = compose(
		feed_construct <PureVariable, 0>,
		singlet <identifier>
	);
	
	static PRODUCTION(PureFactor) pure_factor = compose(
		feed_construct <PureFactor>,
		pure_variable
	);
	
	static PRODUCTION(PureTerm) pure_term = compose(
		feed_construct <PureTerm>,
		pure_factor
	);

	static PRODUCTION(PureExpression) pure_expression = compose(
		feed_construct <PureExpression>,
		chain(pure_term, sym_plus(), pure_term)
	);
	
	static PRODUCTION(PureStatement) pure_statement = compose(
		feed_construct <PureStatement>,
		chain(pure_expression, sym_equals(), pure_expression)
	);

	PARSER(variable, Variable) {
		// TODO: options
		if (auto r = reference(tokens, i)) {
			std::string s = r.value();
			return Reference(s);
		} else if (auto r = chain(sym_dollar(), pure_variable)(tokens, i)) {
			auto v = r.value();
			return std::get <1> (v);
		}

		return std::nullopt;
	}

	static PRODUCTION(Expression) expression = compose(
		feed_construct <Expression, 0>,
		reference
	);

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

	static PRODUCTION(Import) import = compose(
		feed_construct <Import, 1, 3>,
		chain(kwd_from(), identifier(), kwd_use(), identifier())
	);

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

	static PRODUCTION(Assignment) assignment = compose(
		feed_construct <Assignment, 0, 2>,
		chain(reference, sym_equals(), value)
	);

	static PRODUCTION(Association) association = compose(
		feed_construct <Association, 0, 2>,
		chain(statement, sym_double_colon(), predicates)
	);
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

	fmt::println("index: {}; next={}", index, tokens[index]);
	
	Parser::assignment(tokens, index);
	
	fmt::println("index: {}, next={}", index, tokens[index]);
	
	Parser::assignment(tokens, index);
	
	fmt::println("index: {}, next={}", index, tokens[index]);
	
	Parser::association(tokens, index);
	
	fmt::println("index: {}, next={} (end={})", index, tokens[index], index == tokens.size());
}