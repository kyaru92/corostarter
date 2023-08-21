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
    suspend_never final_suspend() noexcept { return {}; }  // 改成suspend_never会core
    void unhandled_exception() {}
    void return_void() { printf("co_return\n"); }
    suspend_always yield_value(int v) {
      value = v;
      return {};
    }

    int value;
  };

  coroutine_handle<promise_type> handle;
  operator coroutine_handle<promise_type>() const { return handle; }
  operator coroutine_handle<>() const { return handle; }
};

template <typename PromiseType> struct Awaiter {
  PromiseType *promise;
  bool await_suspend(coroutine_handle<PromiseType> handle) {
    promise = &handle.promise();
    return false; // 返回false会阻止suspend，这样就能在获取promise指针后继续执行函数，在此期间我们可以给value第一次赋值
  }
  bool await_ready() { return false; } // 该函数返回true，则不会suspend函数
  PromiseType *
  await_resume() { // 如果不返回void而是一个值，该值作为co_await表达式的值
    return promise;
  }
};

Task counter() {
  for (int i = 0; i < 10; i++) {
    co_yield i;
  }
  co_return; // 返回空值调用p.return_void()，有值则为p.return_value(v)。当协程返回时，相当于隐式co_await
             // final_suspend()，如果final_suspend不suspend当前函数，会自动销毁保存的状态，否则需要手动调用destory()
}

int main(int argc, char **argv) {
  coroutine_handle<Task::promise_type> h =
      counter(); // Task在这行后会被销毁，但coroutine_handle更像一个指针，所以Task的析构不影响我们获取的coroutine_handle的使用
  while (!h.done()) {
    printf("%d\n", h.promise().value);
    h();
  }
  h.destroy(); // 还需要手动释放空间
}
