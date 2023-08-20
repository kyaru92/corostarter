#include <coroutine>
#include <stdio.h>

using namespace std;

struct Task {
  struct promise_type {
    Task get_return_object() { return {}; }
    suspend_never initial_suspend() { return {}; }
    suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_void() {}
  };
};

struct Awaiter {
  coroutine_handle<> *handle_;
  void await_suspend(coroutine_handle<> handle) {
    printf("call await_suspend with coroutine_handle at %p\n",
           handle.address());
    if (handle_ != nullptr) {
      printf("compiler creates a coroutine handle and passes it to the method "
             "await_suspend()\n");
      *handle_ = handle;
      handle_ = nullptr;
    }
  }
  bool await_ready() { return false; }  // 该函数返回true，则不会suspend函数
  void await_resume() {}  // 如果不返回void而是一个值，该值作为co_await表达式的值
};

Task counter(coroutine_handle<> *handle) {
  Awaiter a{handle};
  for (int i = 0;; i++) {
    co_await a; // 每次co_await一个awaiter的时候
                // 都会把当前的“状态”打包成一个coroutine_handle传递给awaiter的await_suspend函数
                // coroutine_handle是一个可调用对象，它可以被安全的复制使用，调用的时候会继续执行这个函数
    printf("counter: %d\n", i);
  }
}

int main(int argc, char **argv) {
  coroutine_handle<> h;
  counter(&h);
  for (int i = 0; i < 10; i++) {
    h();
  }
  h.destroy();
}
