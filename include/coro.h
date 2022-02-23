#ifndef __CORO_H__
#define __CORO_H__

#if 1
#include <experimental/coroutine>
using namespace std::experimental;
#else
#include <coroutine>
using namespace std;
#endif


struct Coroutine {
  struct promise_type {
    suspend_always initial_suspend() { return {}; }
    suspend_always yield_value(int) { return {}; }
    suspend_never return_void() { return {}; }
    suspend_always final_suspend() noexcept(true) { return {}; }
    void unhandled_exception() const noexcept { }
    Coroutine get_return_object() { return Coroutine{coroutine_handle<promise_type>::from_promise(*this)}; }
  };
  coroutine_handle<promise_type> coro;
  Coroutine(coroutine_handle<promise_type> h): coro(h) { }
  ~Coroutine() { if(coro) coro.destroy(); }
  bool clock() { coro.resume(); return !coro.done(); }
};


#endif //__CORO_H__
