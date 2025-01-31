#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/printf.h>

#include <howler.hpp>

#include "token.hpp"

/////////////////////////
// Parsing productions //
/////////////////////////

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
struct Association {
	// TODO: vector
	Statement statement;
	Predicates predicates;
};

struct Value : bestd::variant <PurePredicates, Statement, Association> {};

struct Assignment {
	Reference destination;
	Value value;
};

struct Instruction : bestd::variant <Import, Assignment, Value> {};

//////////////////////////////////
// Parser grammar specification //
//////////////////////////////////

#define PRODUCTION(T) inline const std::function <bestd::optional <T> (const std::vector <Token> &, size_t &)>

struct Parser : nabu::TokenParser <Token> {
	// reference := identifier
	static PRODUCTION(Reference) reference = constructor <Reference, 0> (singlet <identifier>);
	
	// pure_variable := identifier
	static PRODUCTION(PureVariable) pure_variable = constructor <PureVariable, 0> (singlet <identifier>);
	
	// pure_variable := pure_variable
	static PRODUCTION(PureFactor) pure_factor = constructor <PureFactor> (pure_variable);
	
	// pure_term := pure_factor
	static PRODUCTION(PureTerm) pure_term = constructor <PureTerm> (pure_factor);

	// pure_expression := pure_term '+' pure_term
	static PRODUCTION(PureExpression) pure_expression = constructor <PureExpression> (chain(pure_term, sym_plus(), pure_term));
	
	// pure_statement := pure_expression '=' pure_expression
	static PRODUCTION(PureStatement) pure_statement = constructor <PureStatement> (chain(pure_expression, sym_equals(), pure_expression));

	// variable := reference | pure_variable
	static PRODUCTION(Variable) __var_ref = constructor <Variable, 0> (reference);
	static PRODUCTION(Variable) __var_pure = constructor <Variable, 1> (chain(sym_dollar(), pure_variable));
	static PRODUCTION(Variable) variable = options(__var_ref, __var_pure);

	// expression := reference
	static PRODUCTION(Expression) expression = constructor <Expression, 0> (reference);

	// statement := reference | '$' '(' pure_statement ')'
	static PRODUCTION(Statement) __stmt_ref = constructor <Statement, 0> (reference);
	static PRODUCTION(Statement) __stmt_pure = constructor <Statement, 2> (chain(sym_dollar(), sym_left_paren(), pure_statement, sym_right_paren()));
	static PRODUCTION(Statement) statement = options(__stmt_ref, __stmt_pure);

	// pure_predicate_domain := variable 'in' expression
	static PRODUCTION(PurePredicateDomain) pure_predicate_domain = constructor <PurePredicateDomain, 0, 2> (chain(variable, kwd_in(), expression));

	// pure_predicates := '{' ( pure_predicate_domain ','? )* '}'
	static PRODUCTION(PurePredicates) pure_predicates = constructor <PurePredicates, 1> (chain(sym_left_brace(), loop <true> (pure_predicate_domain, sym_comma()), sym_right_brace()));
	
	// predicates := reference | pure_predicates
	static PRODUCTION(Predicates) __pred_ref = constructor <Predicates, 0> (reference);
	static PRODUCTION(Predicates) __pred_pure = constructor <Predicates, 0> (pure_predicates);
	static PRODUCTION(Predicates) predicates = options(__pred_ref, __pred_pure);

	// import := 'from' identifier 'use' identifier
	static PRODUCTION(Import) import = constructor <Import, 1, 3> (chain(kwd_from(), identifier(), kwd_use(), identifier()));

	// association := statement '::' predicates
	static PRODUCTION(Association) association = constructor <Association, 0, 2> (chain(statement, sym_double_colon(), predicates));

	// TODO: reference
	// value := association | pure_predicates | statement
	static PRODUCTION(Value) __value_association = constructor <Value, 0> (association);
	static PRODUCTION(Value) __value_pred = constructor <Value, 0> (pure_predicates);
	static PRODUCTION(Value) __value_statement = constructor <Value, 0> (statement);
	static PRODUCTION(Value) value = options(__value_association, __value_pred, __value_statement);

	// assignment := reference '=' value
	static PRODUCTION(Assignment) assignment = constructor <Assignment, 0, 2> (chain(reference, sym_equals(), value));

	// instruction := import | assignment | value
	static PRODUCTION(Instruction) __inst_import = constructor <Instruction, 0> (import);
	static PRODUCTION(Instruction) __inst_assignment = constructor <Instruction, 0> (assignment);
	static PRODUCTION(Instruction) __inst_value = constructor <Instruction, 0> (value);
	static PRODUCTION(Instruction) instruction = options(__inst_import, __inst_assignment, __inst_value);
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

	while (auto v = Parser::instruction(tokens, index))
		fmt::println("instruction[{}], index={}, next={}", v.value().index(), index, tokens[index]);
}