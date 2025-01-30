#include <string>
#include <vector>
#include <optional>
#include <functional>

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

template <typename F>
struct __optional_returner : std::false_type {};

template <typename R, typename ... Args>
struct __optional_returner <std::function <std::optional <R> (Args...)>> : std::true_type {
	using result = R;
};

#define hacked(T) *reinterpret_cast <T *> ((void *) nullptr)

template <typename F>
using optional_returner = __optional_returner <decltype(std::function(hacked(F)))>;

template <typename F>
constexpr bool optional_returner_v = optional_returner <F> ::value;

template <typename F>
concept parser = optional_returner_v <F>
		&& requires(const F &f,
			    const std::vector <Token> &tokens,
			    size_t &i) {
	{ f(tokens, i) };
};

template <parser ... Fs>
struct parser_chain {
	using result_t = std::tuple <typename optional_returner <Fs> ::result...>;

	std::tuple <Fs...> fs;

	parser_chain(const Fs &... fs_) : fs(fs_...) {}

	template <size_t I>
	bool eval_i(result_t &result, const std::vector <Token> &tokens, size_t &i) const {
		if constexpr (I >= sizeof...(Fs)) {
			return true;
		} else {
			auto fv = std::get <I> (fs)(tokens, i);
			if (!fv)
				return false;

			if (!eval_i <I + 1> (result, tokens, i))
				return false;

			std::get <I> (result) = fv.value();

			return true;
		}
	}

	std::optional <result_t> operator()(const std::vector <Token> &tokens, size_t &i) const {
		size_t c = i;
		result_t result;

		if (!eval_i <0> (result, tokens, i)) {
			c = i;
			return std::nullopt;
		}

		return result;
	}
};

template <parser ... Fs>
auto chain(const Fs &... args)
{
	return parser_chain(std::function(args)...);
}

template <parser F, bool EmptyOk, typename D = void>
struct parser_loop {

};

// With a delimiter
template <parser F, bool EmptyOk, parser D>
struct parser_loop <F, EmptyOk, D> {
	using result_t = std::vector <typename optional_returner <F> ::result>;

	F f;
	D d;

	parser_loop(const F &f_, const D &d_) : f(f_), d(d_) {}

	std::optional <result_t> operator()(const std::vector <Token> &tokens, size_t &i) const {
		size_t c = i;
		result_t result;

		while (true) {
			auto fv = f(tokens, i);
			if (!fv) {
				if (result.empty() && EmptyOk)
					break;

				i = c;
				return std::nullopt;
			}

			result.push_back(fv.value());

			if (!d(tokens, i))
				break;
		}

		return result;
	}
};

template <bool EmptyOk, parser F, parser D>
auto loop(const F &f, const D &d)
{
	auto ff = std::function(f);
	auto fd = std::function(d);
	return parser_loop <decltype(ff), EmptyOk, decltype(fd)> (ff, fd);
}

template <typename T>
requires (Token::type_index <T> () >= 0)
std::optional <T> parse_token(const std::vector <Token> &tokens, size_t &i)
{
	if (!tokens[i].is <T> ())
		return std::nullopt;

	return tokens[i++].as <T> ();
}

std::optional <pure_variable> parse_pure_variable(const std::vector <Token> &tokens, size_t &i)
{
	if (!tokens[i].is <sym_dollar> ())
		return std::nullopt;

	if (!tokens[i + 1].is <identifier> ()) {
		howl_error("unexpected token '{}' in math mode", tokens[i + 1]);
		return std::nullopt;
	}

	std::string s = tokens[i + 1].as <identifier> ();
	i += 2;
	return pure_variable(s);
}

std::optional <Expression> parse_expression(const std::vector <Token> &tokens, size_t &i)
{
	if (auto r = parse_token <identifier> (tokens, i)) {
		std::string s = r.value();
		return reference(s);
	}

	return std::nullopt;
}

std::optional <predicate_variable_domain> parse_predicate_domain(const std::vector <Token> &tokens, size_t &i)
{
	if (auto r = chain(parse_pure_variable,
			   parse_token <kwd_in>,
			   parse_expression)(tokens, i)) {
		auto v = r.value();

		return predicate_variable_domain {
			.var = std::get <0> (v),
			.domain = std::get <2> (v),
		};
	}

	return std::nullopt;
}

std::optional <predicates> parse_predicates(const std::vector <Token> &tokens, size_t &i)
{
	if (auto r = chain(parse_token <sym_left_brace>,
			   loop <true> (parse_predicate_domain,
			   		parse_token <sym_comma>),
			   parse_token <sym_right_brace>)(tokens, i)) {
		auto v = r.value();

		return predicates {
			.domains = std::get <1> (v)
		};
	}

	return std::nullopt;
}

int main()
{
	auto tokens = lex(source);

	howl_info("tokens: ");
	for (auto &t : tokens)
		fmt::print("{} ", t);
	fmt::print("\n");

	size_t index = 0;
	auto predicates = parse_predicates(tokens, index);

	fmt::println("index: {}; next={}", index, tokens[index]);
	fmt::println("domains:");
	for (auto &pd : predicates->domains) {
		fmt::println("\t{} -> {}", pd.var, pd.domain.as <reference> ());
	}
}