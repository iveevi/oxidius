#include <string>
#include <vector>
#include <optional>
#include <functional>

#include <fmt/format.h>
#include <fmt/printf.h>

#include <howler.hpp>

#include "common/variant.hpp"
#include "common/nabu.hpp"

std::string source = R"(
{ $a in Real, $b in Real } := $(a + b = b + a)
predicate = { $a in Real, $b in Real }
statement = $(a + b = b + a)
predicate := statement
)";

// Tokens
struct null {};
struct kwd_in {};
struct sym_dollar {};
struct sym_colon_equals {};
struct sym_equals {};
struct sym_plus {};
struct sym_left_brace {};
struct sym_right_brace {};
struct sym_left_paren {};
struct sym_right_paren {};
struct sym_comma {};

struct misc_identifier : std::string {
	using std::string::string;

	misc_identifier(const std::string &s) : std::string(s) {}
};

std::optional <null> space(const std::string &source, size_t &i)
{
	if (std::isspace(source[i])) {
		i++;
		return {};
	}

	return std::nullopt;
}

std::optional <misc_identifier> identifier(const std::string &source, size_t &i)
{
	size_t j = i;

	std::string id;
	while (std::isalpha(source[j])) {
		id += source[j++];
	}

	if (id.empty())
		return std::nullopt;

	i = j;

	return misc_identifier(id);
}

static const auto oxidius_lexer = lexer(space,
	raw <"{", sym_left_brace>,
	raw <"}", sym_right_brace>,
	raw <"(", sym_right_paren>,
	raw <")", sym_left_paren>,
	raw <"$", sym_dollar>,
	raw <":=", sym_colon_equals>,
	raw <"=", sym_equals>,
	raw <"+", sym_plus>,
	raw <"in", kwd_in>,
	raw <",", sym_comma>,
	identifier);

using OxidiusToken = typename decltype(oxidius_lexer)::element_t;

std::string format_as(const OxidiusToken &token)
{
	switch (token.index()) {
	variant_case(OxidiusToken, kwd_in):
		return "<kwd: 'in'>";
	variant_case(OxidiusToken, sym_dollar):
		return "<sym: '$'>";
	variant_case(OxidiusToken, sym_colon_equals):
		return "<sym: ':='>";
	variant_case(OxidiusToken, sym_equals):
		return "<sym: '='>";
	variant_case(OxidiusToken, sym_left_brace):
		return "<sym: '{'>";
	variant_case(OxidiusToken, sym_right_brace):
		return "<sym: '}'>";
	variant_case(OxidiusToken, sym_left_paren):
		return "<sym: '('>";
	variant_case(OxidiusToken, sym_right_paren):
		return "<sym: ')'>";
	variant_case(OxidiusToken, sym_plus):
		return "<sym: '+'>";
	variant_case(OxidiusToken, sym_comma):
		return "<sym: ','>";
	variant_case(OxidiusToken, misc_identifier):
		return "<identifier: '" + token.as <misc_identifier> () + "'>";
	}

	return "?";
}

// Mathematical variable
struct pure_variable : std::string {};

struct pure_expression {};

struct pure_statement {
	pure_expression lhs;
	pure_expression rhs;
};

// Programming variable
struct reference : std::string {};

using Expression = variant <pure_expression, reference>;

struct predicate_variable_domain {
	pure_variable var;
	Expression domain;
};

struct predicates {
	std::vector <predicate_variable_domain> domains;
};

std::optional <pure_variable> parse_pure_variable(const std::vector <OxidiusToken> &tokens, size_t &i)
{
	if (!tokens[i].is <sym_dollar> ())
		return std::nullopt;

	if (!tokens[i + 1].is <misc_identifier> ()) {
		howl_error("unexpected token '{}' in math mode", tokens[i + 1]);
		return std::nullopt;
	}

	std::string s = tokens[i + 1].as <misc_identifier> ();
	i += 2;
	return pure_variable(s);
}

std::optional <Expression> parse_expression(const std::vector <OxidiusToken> &tokens, size_t &i)
{
	if (auto r = parse_token <OxidiusToken, misc_identifier> (tokens, i)) {
		std::string s = r.value();
		return reference(s);
	}

	return std::nullopt;
}

std::optional <predicate_variable_domain> parse_predicate_domain(const std::vector <OxidiusToken> &tokens, size_t &i)
{
	if (auto r = chain <OxidiusToken> (parse_pure_variable,
			   parse_token <OxidiusToken, kwd_in>,
			   parse_expression)(tokens, i)) {
		auto v = r.value();

		return predicate_variable_domain {
			.var = std::get <0> (v),
			.domain = std::get <2> (v),
		};
	}

	return std::nullopt;
}

std::optional <predicates> parse_predicates(const std::vector <OxidiusToken> &tokens, size_t &i)
{
	if (auto r = chain <OxidiusToken> (parse_token <OxidiusToken, sym_left_brace>,
			   loop <OxidiusToken, true> (parse_predicate_domain, parse_token <OxidiusToken, sym_comma>),
			   parse_token <OxidiusToken, sym_right_brace>)(tokens, i)) {
		auto v = r.value();

		return predicates {
			.domains = std::get <1> (v)
		};
	}

	return std::nullopt;
}

int main()
{
	size_t index;
	
	index = 0;
	auto tokens = oxidius_lexer(source, index).value();

	fmt::print("tokens: ");
	for (auto &t : tokens)
		fmt::print("{} ", t);
	fmt::print("\n");

	index = 0;
	auto predicates = parse_predicates(tokens, index);

	fmt::println("index: {}; next={}", index, tokens[index]);
	fmt::println("domains:");
	for (auto &pd : predicates->domains) {
		fmt::println("\t{} -> {}", pd.var, pd.domain.as <reference> ());
	}
}