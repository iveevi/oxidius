#pragma once

#include <nabu/nabu.hpp>

// Tokens
struct kwd_in {};
struct kwd_use {};
struct kwd_from {};
struct sym_dollar {};
struct sym_double_colon {};
struct sym_equals {};
struct sym_plus {};
struct sym_left_brace {};
struct sym_right_brace {};
struct sym_left_paren {};
struct sym_right_paren {};
struct sym_comma {};

struct identifier : std::string {
	using std::string::string;

	identifier(const std::string &s) : std::string(s) {}
};

// TODO: specify by regexes
// i.e. \w for whitespace
// i.e \a for alpha characters
inline bestd::optional <nabu::null> lex_space(const std::string &source, size_t &i)
{
	if (std::isspace(source[i])) {
		i++;
		return nabu::null();
	}

	return std::nullopt;
}

inline bestd::optional <identifier> lex_identifier(const std::string &source, size_t &i)
{
	size_t j = i;

	std::string id;
	while (std::isalpha(source[j])) {
		id += source[j++];
	}

	if (id.empty())
		return std::nullopt;

	i = j;

	return identifier(id);
}

static const auto oxidius_lexer = lexer(lex_space,
	nabu::raw <"{", sym_left_brace>,
	nabu::raw <"}", sym_right_brace>,
	nabu::raw <"(", sym_left_paren>,
	nabu::raw <")", sym_right_paren>,
	nabu::raw <"$", sym_dollar>,
	nabu::raw <"::", sym_double_colon>,
	nabu::raw <"=", sym_equals>,
	nabu::raw <"+", sym_plus>,
	nabu::raw <",", sym_comma>,
	nabu::raw <"in", kwd_in>,
	nabu::raw <"use", kwd_use>,
	nabu::raw <"from", kwd_from>,
	lex_identifier);

using Token = typename decltype(oxidius_lexer)::element_t;

inline std::string format_as(const Token &token)
{
	switch (token.index()) {
	variant_case(Token, kwd_in):
		return "<kwd: 'in'>";
	variant_case(Token, kwd_use):
		return "<kwd: 'use'>";
	variant_case(Token, kwd_from):
		return "<kwd: 'from'>";
	variant_case(Token, sym_dollar):
		return "<sym: '$'>";
	variant_case(Token, sym_double_colon):
		return "<sym: '::'>";
	variant_case(Token, sym_equals):
		return "<sym: '='>";
	variant_case(Token, sym_left_brace):
		return "<sym: '{'>";
	variant_case(Token, sym_right_brace):
		return "<sym: '}'>";
	variant_case(Token, sym_left_paren):
		return "<sym: '('>";
	variant_case(Token, sym_right_paren):
		return "<sym: ')'>";
	variant_case(Token, sym_plus):
		return "<sym: '+'>";
	variant_case(Token, sym_comma):
		return "<sym: ','>";
	variant_case(Token, identifier):
		return "<identifier: '" + token.as <identifier> () + "'>";
	}

	return "?";
}