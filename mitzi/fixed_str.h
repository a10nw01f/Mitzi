#pragma once

#include <algorithm>
#include <array>
#include <span>
#include <string_view>

namespace mitzi {

template <size_t N>
class fixed_string {
 public:
  static_assert(N > 1,
                "N must be at least 2 to store metadata and one character");
  static_assert(N < 256, "N must fit in a single byte (0-255)");


  std::array<char, N> data_{};

  constexpr void set_remaining(size_t remaining) {
    data_[N - 1] = static_cast<char>(remaining);
  }

  constexpr size_t get_remaining() const {
    return static_cast<size_t>(static_cast<unsigned char>(data_[N - 1]));
  }


  constexpr fixed_string() { clear(); }

  constexpr fixed_string(std::string_view sv) { assign(sv); }

  template<auto NN>
  constexpr fixed_string(char const (&str)[NN]) { 
    assign(std::string_view(str, str + NN)); 
  }

  constexpr size_t buffer_size() const { return N; }

  constexpr size_t size() const { return (N - 1) - get_remaining(); }

  constexpr auto empty() const->bool { return size() == 0; }

  constexpr void assign(std::string_view sv) {
    const auto size = sv.size();
    if (size > (N - 1)) {
      throw "Fixed_string: input string_view size exceeds Fixed_string "
            "capacity";
    }
    std::copy_n(sv.data(), size, data_.begin());

    data_[size] = '\0';  // Null terminator for C-compatibility
    set_remaining((N - 1) - size);
  }

  constexpr void resize(size_t size) {
    if (size > (N - 1)) {
      throw "Fixed_string: resize size exceeds Fixed_string capacity";
    }
    data_[size] = '\0';  // Null terminator for C-compatibility
    set_remaining((N - 1) - size);
  }

  constexpr fixed_string& operator=(std::string_view sv) {
    assign(sv);
    return *this;
  }

  constexpr const char* c_str() const { return data_.data(); }

  constexpr char* data() { return data_.data(); }
  constexpr const char* data() const { return data_.data(); }

  constexpr void clear() {
    set_remaining(N - 1);
    data_[0] = '\0';
  }

  constexpr auto view() const {
    return std::string_view(data_.data(), size() - 1);
  }

  constexpr operator std::string_view() const { return view(); }
};

}
