#ifndef __EXPRESSION_H__
#define __EXPRESSION_H__

template <class T>
class expr_var
{
  T& var;
  expr_var() = delete;
public:
  typedef T value_type;
  expr_var(const expr_var&) = default;
  expr_var(T& v) : var(v) {}
  operator T() const { return var; } 
};

template <class T>
class expr_val
{
  T val;
  expr_val() = delete;
public:
  typedef T value_type;
  constexpr expr_val(const expr_val&) = default;
  constexpr expr_val(T v) : val(v) {}
  constexpr operator T() const { return val; } 
};

template<class T, class U>
class expr_shr
{
  T t;
  U u;
public:
  typedef typename T::value_type value_type;
  expr_shr(T _t, U _u) : t(_t), u(_u) {}
  operator value_type() const { return value_type(t) >> u; } 
};


template<class T>
expr_var<T> expr(T& var) { return var; }
template<class T>
expr_val<T> constexpr expr_const(T val) { return val; }

template<class T, class U>
expr_shr<expr_var<T>, expr_val<U>> operator >> (expr_var<T> t, expr_val<U> u)
{
  return expr_shr<expr_var<T>, expr_val<U>>(t, u);
}

#endif //__EXPRESSION_H__

