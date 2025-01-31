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
T __construct(const bestd::tuple <Args...> &arg)
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

template <typename T, size_t ... Is, typename G>
constexpr auto constructor(const G &g)
{
	return compose(feed_construct <T, Is...>, g);
}

#define PRODUCTION(T) inline const std::function <bestd::optional <T> (const std::vector <Token> &, size_t &)>

struct Parser : nabu::TokenParser <Token> {
	static PRODUCTION(Reference) reference = constructor <Reference, 0> (singlet <identifier>);
	
	static PRODUCTION(PureVariable) pure_variable = constructor <PureVariable, 0> (singlet <identifier>);
	
	static PRODUCTION(PureFactor) pure_factor = constructor <PureFactor> (pure_variable);
	
	static PRODUCTION(PureTerm) pure_term = constructor <PureTerm> (pure_factor);

	static PRODUCTION(PureExpression) pure_expression = constructor <PureExpression> (chain(pure_term, sym_plus(), pure_term));
	
	static PRODUCTION(PureStatement) pure_statement = constructor <PureStatement> (chain(pure_expression, sym_equals(), pure_expression));

	static PRODUCTION(Variable) __var_ref = constructor <Variable, 0> (reference);
	static PRODUCTION(Variable) __var_pure = constructor <Variable, 1> (chain(sym_dollar(), pure_variable));
	static PRODUCTION(Variable) variable = options(__var_ref, __var_pure);

	static PRODUCTION(Expression) expression = constructor <Expression, 0> (reference);

	static PRODUCTION(Statement) __stmt_ref = constructor <Statement, 0> (reference);
	static PRODUCTION(Statement) __stmt_pure = constructor <Statement, 2> (chain(sym_dollar(), sym_left_paren(), pure_statement, sym_right_paren()));
	static PRODUCTION(Statement) statement = options(__stmt_ref, __stmt_pure);

	static PRODUCTION(PurePredicateDomain) pure_predicate_domain = constructor <PurePredicateDomain, 0, 2> (chain(variable, kwd_in(), expression));

	static PRODUCTION(PurePredicates) pure_predicates = constructor <PurePredicates, 1> (chain(sym_left_brace(), loop <true> (pure_predicate_domain, sym_comma()), sym_right_brace()));

	static PRODUCTION(Import) import = constructor <Import, 1, 3> (chain(kwd_from(), identifier(), kwd_use(), identifier()));

	static PRODUCTION(Value) __value_pred = constructor <Value, 0> (pure_predicates);
	static PRODUCTION(Value) __value_statement = constructor <Value, 0> (statement);
	static PRODUCTION(Value) value = options(__value_pred, __value_statement);
	
	static PRODUCTION(Predicates) __pred_ref = constructor <Predicates, 0> (reference);
	static PRODUCTION(Predicates) __pred_pure = constructor <Predicates, 0> (pure_predicates);
	static PRODUCTION(Predicates) predicates = options(__pred_ref, __pred_pure);

	static PRODUCTION(Assignment) assignment = constructor <Assignment, 0, 2> (chain(reference, sym_equals(), value));

	static PRODUCTION(Association) association = constructor <Association, 0, 2> (chain(statement, sym_double_colon(), predicates));
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