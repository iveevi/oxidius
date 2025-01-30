#pragma once

#include <variant>

template <typename T, typename U, typename ... Args>
constexpr int variant_index(int i)
{
	if constexpr (std::same_as <T, U>)
		return i;

	if constexpr (sizeof...(Args))
		return variant_index <T, Args...> (i + 1);

	return -1;
}

template <typename ... Args>
struct variant : std::variant <Args...> {
	using std::variant <Args...> ::variant;

	template <typename T>
	inline bool is() const {
		return std::holds_alternative <T> (*this);
	}

	template <typename T>
	inline auto &as() {
		return std::get <T> (*this);
	}

	template <typename T>
	inline const auto &as() const {
		return std::get <T> (*this);
	}

	template <typename T>
	static constexpr int type_index() {
		return variant_index <T, Args...> (0);
	}
};

#define variant_case(T, K) case T::type_index <K> ()