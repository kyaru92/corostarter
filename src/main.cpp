#include <concepts>
#include <coroutine>
#include <exception>
#include <stdio.h>
#include <string>

using namespace std;

template <typename T> struct Generator {
  struct promise_type;
  using HandleType = coroutine_handle<promise_type>;
  struct promise_type {
    Generator get_return_object() {
      return Generator{HandleType::from_promise(*this)};
    }
    suspend_always initial_suspend() { return {}; }
    suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { exception = current_exception(); }
    template <convertible_to<T> From> suspend_always yield_value(From &&from) {
      value = static_cast<From &&>(from);
      return {};
    }
    void return_void() {}

    T value;
    exception_ptr exception;
  };

  HandleType handle;

  explicit Generator(HandleType h) : handle(h) {}
  Generator(const Generator &) = delete;
  ~Generator() { handle.destroy(); }
  explicit operator bool() {
    fetch();
    return !handle.done();
  }
  T operator()() {
    fetch();
    valid_ = false;
    return std::move(handle.promise().value);
  }

private:
  bool valid_;
  void fetch() {
    if (!valid_) {
      handle();
      if (handle.promise().exception) {
        rethrow_exception(handle.promise().exception);
      }
      valid_ = true;
    }
  }
};

Generator<string> split(std::string sentence, char c) {
  size_t beg = 0;

  while (true) {
    size_t pos = sentence.find(c, beg);
    co_yield sentence.substr(beg, pos - beg);
    if (pos == string::npos) {
      break;
    }
    beg = pos + 1;
  }
}

int main(int argc, char **argv) {
  auto words_coro = split("Hello, C++ coroutine!", ' ');
  while (words_coro) {
    printf("%s\n", words_coro().c_str());
  }
}
