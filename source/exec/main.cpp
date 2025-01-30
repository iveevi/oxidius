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

template <size_t N>
struct string_literal {
	char value[N];

	constexpr string_literal(const char (&str)[N]) {
		std::copy_n(str, N, value);
	}
};

template <size_t N>
std::optional <std::string> raw_expanded(const string_literal <N> &raw, const std::string &source, size_t &i)
{
	for (size_t n = 0; n < N - 1; n++) {
		if (source[n + i] != raw.value[n])
			return std::nullopt;
	}

	i += N - 1;

	return "?";
}

template <string_literal s, typename T>
std::optional <T> raw(const std::string &source, size_t &i)
{
	if (auto r = raw_expanded(s, source, i)) {
		if constexpr (std::is_constructible_v <T, std::string>)
			return T(r.value());
		else
			return T();
	}

	return std::nullopt;
}

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

template <typename F>
concept lexer_fn = optional_returner_v <F>
		&& requires(const F &f,
			    const std::string &source,
			    size_t &i) {
	{ f(source, i) };
};

template <typename T, typename... Rest>
constexpr bool has_duplicates()
{
	if constexpr (sizeof...(Rest) == 0)
		return false;
	else
		return (std::is_same_v <T, Rest> || ...) || has_duplicates <Rest...> ();
}

static_assert(has_duplicates <int, double, float, char> () == false);
static_assert(has_duplicates <int, double, int, char> () == true);

template <lexer_fn ... Fs>
constexpr bool valid_lexer_group()
{
	return !has_duplicates <typename optional_returner <Fs> ::result...> ();
}

template <lexer_fn ... Fs>
struct lexer_group {
	static_assert(valid_lexer_group <Fs...> (),
		"lexers in lexer_group(...) "
		"must produce distinct types");

	// TODO: variant...
	using element_t = variant <typename optional_returner <Fs> ::result...>;
	using result_t = std::vector <element_t>;

	std::tuple <Fs...> fs;

	lexer_group(const Fs &... fs_) : fs(fs_...) {}

	// TODO: compare element index...
	
	template <size_t I>
	bool eval_i(result_t &result, const std::string &source, size_t &i) const {
		if constexpr (I >= sizeof...(Fs)) {
			return false;
		} else {
			auto fv = std::get <I> (fs)(source, i);
			if (fv) {
				result.push_back(fv.value());
				return true;
			}

			return eval_i <I + 1> (result, source, i);
		}
	}

	std::optional <result_t> operator()(const std::string &s, size_t &i) const {
		size_t old = i;

		result_t result;

		while (eval_i <0> (result, s, i)) {}

		if (i == s.size())
			return result;

		i = old;

		return std::nullopt;
	}
};

template <lexer_fn ... Fs>
auto lexer(const Fs &... args)
{
	return lexer_group(std::function(args)...);
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

using Token = typename decltype(oxidius_lexer)::element_t;

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
	variant_case(Token, misc_identifier):
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

template <typename F, typename T>
concept parser_fn = optional_returner_v <F>
		&& requires(const F &f,
			    const std::vector <T> &tokens,
			    size_t &i) {
	{ f(tokens, i) };
};

template <parser_fn <Token> ... Fs>
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

template <parser_fn <Token> ... Fs>
auto chain(const Fs &... args)
{
	return parser_chain(std::function(args)...);
}

template <parser_fn <Token> F, bool EmptyOk, typename D = void>
struct parser_loop {

};

// With a delimiter
template <parser_fn <Token> F, bool EmptyOk, parser_fn <Token> D>
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

template <bool EmptyOk, parser_fn <Token> F, parser_fn <Token> D>
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

	if (!tokens[i + 1].is <misc_identifier> ()) {
		howl_error("unexpected token '{}' in math mode", tokens[i + 1]);
		return std::nullopt;
	}

	std::string s = tokens[i + 1].as <misc_identifier> ();
	i += 2;
	return pure_variable(s);
}

std::optional <Expression> parse_expression(const std::vector <Token> &tokens, size_t &i)
{
	if (auto r = parse_token <misc_identifier> (tokens, i)) {
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