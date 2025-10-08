#pragma once

#include <exception>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

#ifdef RESULT_ALLOW_EMPTY_STATE
template <typename T = std::monostate>
#else
template <typename T>
#endif
struct Ok {
  T value;

#ifdef RESULT_ALLOW_EMPTY_STATE
  explicit Ok() = default;
#endif

  explicit Ok(T&& val) : value(std::forward<T>(val)) {}
};

template <typename E>
struct Err {
  E error;

  explicit Err(E&& err) : error(std::forward<E>(err)) {}
};

struct ExceptionError {
  std::exception_ptr err;

  ExceptionError(std::exception_ptr&& ept) : err(std::move(ept)) {}
};

template <typename T, typename E = ExceptionError, typename Enable = void>
struct Result {
  using value_type = T;
  using error_type = E;

  template <typename U,
            typename = std::enable_if_t<
                ((std::is_constructible_v<T, U&&> ||
                  std::is_constructible_v<
                      T, const U&>)^(std::is_constructible_v<E, U&&> ||
                                     std::is_constructible_v<E, const U&>))>>
  Result(U&& val) {
    if constexpr (std::is_constructible_v<T, U&&> ||
                  std::is_constructible_v<T, const U&>) {
      if constexpr (std::is_move_constructible_v<T>) {
        data.template emplace<T>(std::forward<U>(val));
      } else {
        data.template emplace<value_type>(val);
      }
    } else if constexpr (std::is_constructible_v<E, U&&> ||
                         std::is_constructible_v<E, const U&>) {
      if constexpr (std::is_move_constructible_v<E>) {
        data.template emplace<E>(std::forward<U>(val));
      } else {
        data.template emplace<error_type>(val);
      }
    } else {
      static_assert(
          sizeof(U) == 0,
          "Could not deduce T or E use explicit tag types Ok and Err");
    }
  }

#ifdef RESULT_ALLOW_EMPTY_STATE
  // return {}
  Result() noexcept : data(std::monostate{}) {}

  // return Ok{}
  Result(Ok<std::monostate>&&) noexcept : data(std::monostate{}) {}
#endif

  // Converting constructors from Ok<U> and Err<F>
  template <typename U,
            typename = std::enable_if_t<std::is_constructible_v<T, U&&>>>
  Result(Ok<U>&& ok) noexcept(std::is_nothrow_constructible_v<T>)
      : data(std::in_place_type<T>, std::move(ok.value)) {}

  template <typename F,
            typename = std::enable_if_t<std::is_constructible_v<E, F&&>>>
  Result(Err<F>&& err) noexcept(std::is_nothrow_constructible_v<E>)
      : data(std::in_place_type<E>, std::move(err.error)) {}

  // Status checks
  constexpr bool has_value() const noexcept {
    return std::holds_alternative<T>(data);
  }

  constexpr bool is_ok() const noexcept {
    return std::holds_alternative<T>(data);
  }

  constexpr bool has_error() const noexcept {
    return std::holds_alternative<E>(data);
  }

  constexpr bool is_err() const noexcept {
    return std::holds_alternative<E>(data);
  }

  T& unwrap() {
    if (!std::holds_alternative<T>(data)) {
#ifdef RESULT_ALLOW_EMPTY_STATE
      if (std::holds_alternative<std::monostate>(data)) {
        throw std::runtime_error("Called unwrap() on empty Result");
      }
#endif
      throw std::runtime_error("Called unwrap() on Err value");
    }
    return std::get<T>(data);
  }

  T const& unwrap() const {
    if (!std::holds_alternative<T>(data)) {
#ifdef RESULT_ALLOW_EMPTY_STATE
      if (std::holds_alternative<std::monostate>(data)) {
        throw std::runtime_error("Called unwrap() on empty Result");
      }
#endif
      throw std::runtime_error("Called unwrap() on Err value");
    }
    return std::get<T>(data);
  }

  E& unwrap_err() {
    if (!std::holds_alternative<E>(data)) {
#ifdef RESULT_ALLOW_EMPTY_STATE
      if (std::holds_alternative<std::monostate>(data)) {
        throw std::runtime_error("Called unwrap_err() on empty Result");
      }
#endif
      throw std::runtime_error("Called unwrap_err() on Ok value");
    }
    return std::get<E>(data);
  }

  E const& unwrap_err() const {
    if (!std::holds_alternative<E>(data)) {
#ifdef RESULT_ALLOW_EMPTY_STATE
      if (std::holds_alternative<std::monostate>(data)) {
        throw std::runtime_error("Called unwrap_err() on empty Result");
      }
#endif
      throw std::runtime_error("Called unwrap_err() on Ok value");
    }
    return std::get<E>(data);
  }

#ifdef RESULT_ALLOW_EMPTY_STATE
  bool empty() const { return std::holds_alternative<std::monostate>(data); }

  bool is_empty() const { return std::holds_alternative<std::monostate>(data); }

#ifdef RESULT_ENABLE_OPTIONAL_COMPAT
  constexpr explicit operator bool() const noexcept { return has_value(); }

  T const& value() const { return std::get<T>(data); }
#endif

#endif

  T& value() { return std::get<T>(data); }

  T const& value() const { return std::get<T>(data); }

  E& error() { return std::get<E>(data); }

  E const& error() const { return std::get<E>(data); }

  std::optional<T> take_value() {
    if (!has_value())
      return std::nullopt;

    std::optional<T> result;

    if constexpr (std::is_move_constructible_v<T>) {
      result.emplace(std::move(std::get<T>(data)));
    } else if constexpr (std::is_copy_constructible_v<T>) {
      result.emplace(std::get<T>(data));
    } else {
      static_assert(
          std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>,
          "T must be move- or copy-constructible to take_value()");
    }

#ifdef RESULT_ALLOW_EMPTY_STATE
    data = std::monostate{};
#endif
    return result;
  }

  std::optional<E> take_error() {
    if (!has_error())
      return std::nullopt;

    std::optional<E> result;

    if constexpr (std::is_move_constructible_v<E>) {
      result.emplace(std::move(std::get<E>(data)));
    } else if constexpr (std::is_copy_constructible_v<E>) {
      result.emplace(std::get<E>(data));
    } else {
      static_assert(
          std::is_move_constructible_v<E> || std::is_copy_constructible_v<E>,
          "E must be move- or copy-constructible to take_error()");
    }

#ifdef RESULT_ALLOW_EMPTY_STATE
    data = std::monostate{};
#endif
    return result;
  }

 private:
#ifdef RESULT_ALLOW_EMPTY_STATE
  std::variant<std::monostate, T, E> data;
#else
  std::variant<T, E> data;
#endif
};
