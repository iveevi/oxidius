#pragma once

#include <memory>

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

inline PureFactor::PureFactor(const PureExpression &expr) : pure_factor_base(std::make_shared <PureExpression> (expr)) {}

NABU_UTILITIES(Token)

// reference := identifier
extern production <Reference> reference;

// pure_variable := identifier
extern production <PureVariable> pure_variable;

// pure_variable := pure_variable
extern production <PureFactor> pure_factor;

// pure_term := pure_factor
extern production <PureTerm> pure_term;

// pure_statement := pure_expression '=' pure_expression
extern production <PureStatement> pure_statement;

// variable := reference | '$' pure_variable
extern production <Variable> variable;

// expression := reference
extern production <Expression> expression;

// statement := reference | '$' '(' pure_statement ')'
extern production <Statement> statement;

// pure_predicate_domain := variable 'in' expression
extern production <PurePredicateDomain> pure_predicate_domain;

// pure_predicates := '{' ( pure_predicate_domain ','? )* '}'
extern production <PurePredicates> pure_predicates;

// predicates := reference | pure_predicates
extern production <Predicates> predicates;

// import := 'from' identifier 'use' identifier
extern production <Import> import;

// association := statement '::' predicates
extern production <Association> association;

// value := association | reference | pure_predicates | statement
extern production <Value> value;

// assignment := reference '=' value
extern production <Assignment> assignment;

// instruction := import | assignment | value
extern production <Instruction> instruction;

// pure_expression := pure_term '+' pure_term
//                   | pure_term
extern production <PureExpression> pure_expression;