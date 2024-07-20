#include <iostream>
#include <unordered_map>
#include <functional>
#include <string>
#include <tuple>
#include <json.hpp>
using json = nlohmann::json;

template<typename T>
struct function_traits;
template<typename R, typename... Args>
struct function_traits<R(*)(Args...)> {
    using return_type = R;
    using args_tuple = std::tuple<Args...>;
};

class LocalRegister {
public:
    template<typename Func>
    static auto callFunction(Func func, const json& params);
private:
    template<typename Func, typename Tuple, size_t... Is>
    static auto callFunctionImpl(Func func, const json& params, std::index_sequence<Is...>);
public:
    static std::unordered_map<std::string, std::function<json(const json&)>> funcMap;
};
std::unordered_map<std::string, std::function<json(const json&)>> LocalRegister::funcMap;

//使用args_tuple元组，存储参数类型信息
template<typename Func>
auto LocalRegister::callFunction(Func func, const json &params) {
    using args_tuple = typename function_traits<Func>::args_tuple;
    constexpr auto size = std::tuple_size<args_tuple>::value;
    return callFunctionImpl<Func, args_tuple>(func, params, std::make_index_sequence<size>{});
}

//调用函数的时候运用...解包运算符，把元组中的参数类型提取出来，并且使用json中的get方法，把json中的数据转换成对应的类型传递给实际的函数
template<typename Func, typename Tuple, size_t... Is>
auto LocalRegister::callFunctionImpl(Func func, const json &params, std::index_sequence<Is...>) {
    return func(params[Is].get<std::tuple_element_t<Is, Tuple>>()...);
}




