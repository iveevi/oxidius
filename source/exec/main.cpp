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

#define PRODUCTION(T) inline std::function <bestd::optional <T> (const std::vector <Token> &, size_t &)>

struct Parser : nabu::TokenParser <Token> {
	// reference := identifier
	static PRODUCTION(Reference) reference = nabu::constructor <Reference, 0> (singlet <identifier>);
	
	// pure_variable := identifier
	static PRODUCTION(PureVariable) pure_variable = nabu::constructor <PureVariable, 0> (singlet <identifier>);
	
	// pure_variable := pure_variable
	static PRODUCTION(PureFactor) pure_factor = nabu::constructor <PureFactor> (pure_variable);
	
	// pure_term := pure_factor
	static PRODUCTION(PureTerm) pure_term = nabu::constructor <PureTerm> (pure_factor);

	// pure_expression := pure_term '+' pure_term
	static PRODUCTION(PureExpression) pure_expression = nabu::constructor <PureExpression> (chain(pure_term, singlet <sym_plus>, pure_term));
	
	// pure_statement := pure_expression '=' pure_expression
	static PRODUCTION(PureStatement) pure_statement = nabu::constructor <PureStatement> (chain(pure_expression, singlet <sym_equals>, pure_expression));

	// variable := reference | pure_variable
	static PRODUCTION(Variable) __var_ref = nabu::constructor <Variable, 0> (reference);
	static PRODUCTION(Variable) __var_pure = nabu::constructor <Variable, 1> (chain(singlet <sym_dollar>, pure_variable));
	static PRODUCTION(Variable) variable = options(__var_ref, __var_pure);

	// expression := reference
	static PRODUCTION(Expression) expression = nabu::constructor <Expression, 0> (reference);

	// statement := reference | '$' '(' pure_statement ')'
	static PRODUCTION(Statement) __stmt_ref = nabu::constructor <Statement, 0> (reference);
	static PRODUCTION(Statement) __stmt_pure = nabu::constructor <Statement, 2> (chain(singlet <sym_dollar>, singlet <sym_left_paren>, pure_statement, singlet <sym_right_paren>));
	static PRODUCTION(Statement) statement = options(__stmt_ref, __stmt_pure);

	// pure_predicate_domain := variable 'in' expression
	static PRODUCTION(PurePredicateDomain) pure_predicate_domain = nabu::constructor <PurePredicateDomain, 0, 2> (chain(variable, singlet <kwd_in>, expression));

	// pure_predicates := '{' ( pure_predicate_domain ','? )* '}'
	static PRODUCTION(PurePredicates) pure_predicates = nabu::constructor <PurePredicates, 1> (chain(singlet <sym_left_brace>, loop <true> (pure_predicate_domain, singlet <sym_comma>), singlet <sym_right_brace>));
	
	// predicates := reference | pure_predicates
	static PRODUCTION(Predicates) __pred_ref = nabu::constructor <Predicates, 0> (reference);
	static PRODUCTION(Predicates) __pred_pure = nabu::constructor <Predicates, 0> (pure_predicates);
	static PRODUCTION(Predicates) predicates = options(__pred_ref, __pred_pure);

	// import := 'from' identifier 'use' identifier
	static PRODUCTION(Import) import = nabu::constructor <Import, 1, 3> (chain(singlet <kwd_from>, singlet <identifier>, singlet <kwd_use>, singlet <identifier>));

	// association := statement '::' predicates
	static PRODUCTION(Association) association = nabu::constructor <Association, 0, 2> (chain(statement, singlet <sym_double_colon>, predicates));

	// TODO: reference
	// value := association | pure_predicates | statement
	static PRODUCTION(Value) __value_association = nabu::constructor <Value, 0> (association);
	static PRODUCTION(Value) __value_pred = nabu::constructor <Value, 0> (pure_predicates);
	static PRODUCTION(Value) __value_statement = nabu::constructor <Value, 0> (statement);
	static PRODUCTION(Value) value = options(__value_association, __value_pred, __value_statement);

	// assignment := reference '=' value
	static PRODUCTION(Assignment) assignment = nabu::constructor <Assignment, 0, 2> (chain(reference, singlet <sym_equals>, value));

	// instruction := import | assignment | value
	static PRODUCTION(Instruction) __inst_import = nabu::constructor <Instruction, 0> (import);
	static PRODUCTION(Instruction) __inst_assignment = nabu::constructor <Instruction, 0> (assignment);
	static PRODUCTION(Instruction) __inst_value = nabu::constructor <Instruction, 0> (value);
	static PRODUCTION(Instruction) instruction = options(__inst_import, __inst_assignment, __inst_value);
};

int main()
{
	std::string source = R"(
	$(a + b = a + b)
	$(a + b = a + b)
	$(a + b = a + b)
	reference
	
	from std use Real

	$(a + b = a + b)
	
	$(a + b = b + a) :: { $a in Real, $b in Real }

	statement = $(a + b = b + a)
	predicate = { $a in Real, $b in Real }

	statement :: predicate
	)";

	size_t index;
	
	index = 0;
	auto token_bases = oxidius_lexer(source, index).value();

	auto tokens = std::vector <Token> ();
	for (auto &t : token_bases)
		tokens.push_back(t);

	fmt::print("tokens: ");
	for (auto &t : tokens)
		fmt::print("{} ", t);
	fmt::print("\n");

	index = 0;

	while (auto v = Parser::instruction(tokens, index))
		fmt::println("instruction[{}], index={}, next={}", v.value().index(), index, tokens[index]);
	
	// while (auto v = Parser::statement(tokens, index))
	// 	fmt::println("statement[{}], index={}, next={}", v.value().index(), index, tokens[index]);
}