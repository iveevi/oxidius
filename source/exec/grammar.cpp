#include "grammar.hpp"

//////////////////////////////////
// Parser grammar specification //
//////////////////////////////////

constexpr auto from = singlet <kwd_from>;
constexpr auto use = singlet <kwd_use>;
constexpr auto in = singlet <kwd_in>;
constexpr auto ident = singlet <identifier>;
constexpr auto equals = singlet <sym_equals>;
constexpr auto comma = singlet <sym_comma>;
constexpr auto double_colon = singlet <sym_double_colon>;
constexpr auto plus = singlet <sym_plus>;
constexpr auto dollar = singlet <sym_dollar>;
constexpr auto lparen = singlet <sym_left_paren>;
constexpr auto rparen = singlet <sym_right_paren>;
constexpr auto lbrace = singlet <sym_left_brace>;
constexpr auto rbrace = singlet <sym_right_brace>;

// reference := identifier
production <Reference> reference = convert <ident, Reference>;

// pure_variable := identifier
production <PureVariable> pure_variable = convert <ident, PureVariable>;

// pure_variable := pure_variable
production <PureFactor> pure_factor = options <
	convert <chain <lparen, &pure_expression, rparen>, PureFactor, 1>,
	convert <&pure_variable, PureFactor>
>;

// pure_term := pure_factor
production <PureTerm> pure_term = convert <&pure_factor, PureTerm>;

// pure_statement := pure_expression '=' pure_expression
production <PureStatement> pure_statement = convert <chain <&pure_expression, equals, &pure_expression>, PureStatement, 0, 2>;

// variable := reference | '$' pure_variable
production <Variable> variable = options <
	convert <&reference, Variable>,
	convert <chain <dollar, &pure_variable>, Variable, 1>
>;

// expression := reference
production <Expression> expression = options <
	convert <chain <dollar, lparen, &pure_expression, rparen>, Expression, 2>,
	convert <&reference, Expression>
>;

// statement := reference | '$' '(' pure_statement ')'
production <Statement> statement = options <
	convert <chain <dollar, lparen, &pure_statement, rparen>, Statement, 2>,
	convert <&reference, Statement>
>;

// pure_predicate_domain := variable 'in' expression
production <PurePredicateDomain> pure_predicate_domain = convert <chain <&variable, in, &expression>, PurePredicateDomain, 0, 2>;

// pure_predicates := '{' ( pure_predicate_domain ','? )* '}'
production <PurePredicates> pure_predicates = convert <chain <lbrace, loop <&pure_predicate_domain, comma, true>, rbrace>, PurePredicates, 1>;

// predicates := reference | pure_predicates
production <Predicates> predicates = options <
	convert <&reference, Predicates>,
	convert <&pure_predicates, Predicates>
>;

// import := 'from' identifier 'use' identifier
production <Import> import = convert <chain <from, ident, use, ident>, Import, 1, 3>;

// association := statement '::' predicates
production <Association> association = convert <chain <&statement, double_colon, &predicates>, Association, 0, 2>;

// value := association | reference | pure_predicates | statement
production <Value> value = options <
	convert <&association, Value>,
	convert <&reference, Value>,
	convert <&predicates, Value>,
	convert <&statement, Value>
>;

// assignment := reference '=' value
production <Assignment> assignment = convert <chain <&reference, equals, &value>, Assignment, 0, 2>;

// instruction := import | assignment | value
production <Instruction> instruction = options <
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