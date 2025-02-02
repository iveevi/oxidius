#include <string>
#include <vector>
#include <memory>

#include <fmt/format.h>
#include <fmt/printf.h>

#include <howler.hpp>

#include "token.hpp"

/////////////////////////
// Parsing productions //
/////////////////////////

// Forward declarations
struct PureExpression;

// Mathematical variable
struct PureVariable : std::string {};

using pure_factor_base = bestd::variant <PureVariable, std::shared_ptr <PureExpression>>;

struct PureFactor : pure_factor_base {
	PureFactor() = default;
	PureFactor(const PureVariable &var) : pure_factor_base(var) {}
	PureFactor(const PureExpression &);
};

struct PureTerm : bestd::variant <PureFactor> {};

struct PureExpression : bestd::variant <PureTerm> {};

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
struct Association {
	// TODO: vector
	Statement statement;
	Predicates predicates;
};

struct Value : bestd::variant <Reference, Predicates, Statement, Association> {};

struct Assignment {
	Reference destination;
	Value value;
};

struct Instruction : bestd::variant <Import, Assignment, Value> {};

PureFactor::PureFactor(const PureExpression &expr) : pure_factor_base(std::make_shared <PureExpression> (expr)) {}

//////////////////////////////////
// Parser grammar specification //
//////////////////////////////////

NABU_UTILITIES(Token)

const auto &from = singlet <kwd_from>;
const auto &use = singlet <kwd_use>;
const auto &in = singlet <kwd_in>;
const auto &ident = singlet <identifier>;
const auto &equals = singlet <sym_equals>;
constexpr auto &comma = singlet <sym_comma>;
constexpr auto &double_colon = singlet <sym_double_colon>;
constexpr auto &plus = singlet <sym_plus>;
constexpr auto &dollar = singlet <sym_dollar>;
constexpr auto &lparen = singlet <sym_left_paren>;
constexpr auto &rparen = singlet <sym_right_paren>;
constexpr auto &lbrace = singlet <sym_left_brace>;
constexpr auto &rbrace = singlet <sym_right_brace>;

// Forward declaring roots of recursive grammars
extern production <PureExpression> pure_expression;

// reference := identifier
auto reference = convert <ident, Reference>;

// pure_variable := identifier
auto pure_variable = convert <ident, PureVariable>;

// pure_variable := pure_variable
auto pure_factor = options <
	convert <chain <lparen, &pure_expression, rparen>, PureFactor, 1>,
	convert <&pure_variable, PureFactor>
>;

// pure_term := pure_factor
auto pure_term = convert <&pure_factor, PureTerm>;

// pure_statement := pure_expression '=' pure_expression
auto pure_statement = convert <chain <&pure_expression, equals, &pure_expression>, PureStatement, 0, 2>;

// variable := reference | '$' pure_variable
auto variable = options <
	convert <&reference, Variable>,
	convert <chain <dollar, &pure_variable>, Variable, 1>
>;

// expression := reference
auto expression = options <
	convert <chain <dollar, lparen, &pure_expression, rparen>, Expression, 2>,
	convert <&reference, Expression>
>;

// statement := reference | '$' '(' pure_statement ')'
auto statement = options <
	convert <chain <dollar, lparen, &pure_statement, rparen>, Statement, 2>,
	convert <&reference, Statement>
>;

// pure_predicate_domain := variable 'in' expression
auto pure_predicate_domain = convert <chain <&variable, in, &expression>, PurePredicateDomain, 0, 2>;

// pure_predicates := '{' ( pure_predicate_domain ','? )* '}'
auto pure_predicates = convert <chain <lbrace, loop <&pure_predicate_domain, comma>, rbrace>, PurePredicates, 1>;

// predicates := reference | pure_predicates
auto predicates = options <
	convert <&reference, Predicates>,
	convert <&pure_predicates, Predicates>
>;

// import := 'from' identifier 'use' identifier
auto import = convert <chain <from, ident, use, ident>, Import, 1, 3>;

// association := statement '::' predicates
auto association = convert <chain <&statement, double_colon, &predicates>, Association, 0, 2>;

// value := association | reference | pure_predicates | statement
auto value = options <
	convert <&association, Value>,
	convert <&reference, Value>,
	convert <&predicates, Value>,
	convert <&statement, Value>
>;

// assignment := reference '=' value
auto assignment = convert <chain <&reference, equals, &value>, Assignment, 0, 2>;

// instruction := import | assignment | value
auto instruction = options <
	convert <&import, Instruction>,
	convert <&assignment, Instruction>,
	convert <&value, Instruction>
>;

// pure_expression := pure_term '+' pure_term
//                   | pure_term
production <PureExpression> pure_expression = options <
	convert <chain <&pure_term, plus, &pure_term>, PureExpression, 0>,
	convert <&pure_term, PureExpression>
>;

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

	fmt::print("tokens: ");
	for (auto &t : tokens)
		fmt::print("{} ", t);
	fmt::print("\n");

	index = 0;
	while (auto v = instruction(tokens, index))
		fmt::println("instruction[{}], index={}, next={}", v.value().index(), index, tokens[index]);
}