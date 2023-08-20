#include <coroutine>
#include <stdio.h>

using namespace std;

struct Task {
  // 作为协程返回值的R，它必须有个嵌套类型名为promise_type
  struct promise_type {
    // 该类型必须有一个函数签名为R get_return_object()的函数，
    // get_return_object的返回值R将作为*协程函数*的返回值
    Task get_return_object() {
      return Task{
          .handle = std::coroutine_handle<promise_type>::from_promise(
              *this)}; // 固定模式？创建一个R，coroutine_handle用这个静态函数创建（获取）
    }
    suspend_never initial_suspend() { return {}; }
    suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_void() {}
  };

  coroutine_handle<promise_type> handle;
  operator coroutine_handle<>() const { return handle; }
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
  bool await_ready() { return false; } // 该函数返回true，则不会suspend函数
  void await_resume() {} // 如果不返回void而是一个值，该值作为co_await表达式的值
};

Task counter() {
  for (int i = 0;; i++) {
    co_await std::suspend_always{};
    printf("counter: %d\n", i);
  }
}

int main(int argc, char **argv) {
  coroutine_handle<> h =
      counter(); // Task在这行后会被销毁，但coroutine_handle更像一个指针，所以Task的析构不影响我们获取的coroutine_handle的使用
  for (int i = 0; i < 10; i++) {
    h();
  }
  h.destroy(); // 还需要手动释放空间
}
