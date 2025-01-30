#include <string>
#include <vector>
#include <optional>

#include <fmt/format.h>
#include <fmt/printf.h>

#include <howler.hpp>

#include "common/variant.hpp"

std::string source = R"(
{ $a in Real, $b in Real } := $(a + b = b + a)
predicate = { $a in Real, $b in Real }
statement = $(a + b = b + a)
predicate := statement
)";

// Tokens
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

struct identifier : std::string {
	using std::string::string;
};

using Token = variant <
	kwd_in, sym_dollar, sym_colon_equals, sym_equals,
	sym_left_brace, sym_right_brace,
	sym_left_paren, sym_right_paren,
	sym_plus, sym_comma,
	identifier
>;

std::string format_as(const Token &token)
{
	switch (token.index()) {
	variant_case(Token, kwd_in):
		return "<kwd: 'in'>";
	variant_case(Token, sym_dollar):
		return "<sym: '$'>";
	variant_case(Token, sym_colon_equals):
		return "<sym: ':='>";
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

std::optional <Token> lex_special(const std::string &s, size_t &i, char c)
{
	switch (c) {
	case '{':
		return sym_left_brace();
	case '}':
		return sym_right_brace();
	case '(':
		return sym_left_paren();
	case ')':
		return sym_right_paren();
	case '$':
		return sym_dollar();
	case ',':
		return sym_comma();
	case ':':
		if (s[i] == '=') {
			i++;
			return sym_colon_equals();
		}

		break;
	case '=':
		return sym_equals();
	case '+':
		return sym_plus();
	default:
		break;
	}

	howl_error("unexpected character '{}'", c);

	return std::nullopt;
}

std::vector <Token> lex(const std::string &s)
{
	std::vector <Token> result;

	size_t i = 0;
	while (i < s.size()) {
		auto &c = s[i++];

		if (std::isalpha(c)) {
			identifier id(1, c);
			while (std::isalpha(s[i]))
				id += s[i++];

			if (id == "in")
				result.push_back(kwd_in());
			else
				result.push_back(id);
		} else if (std::isspace(c)) {
			continue;
		} else {
			auto t = lex_special(s, i, c);
			if (t)
				result.push_back(t.value());
			else
				break;
		}
	}

	return result;
}

struct variable : std::string {};
struct reference : std::string {};

int main()
{
	auto tokens = lex(source);

	howl_info("tokens: ");
	for (auto &t : tokens)
		fmt::print("{} ", t);
	fmt::print("\n");
}