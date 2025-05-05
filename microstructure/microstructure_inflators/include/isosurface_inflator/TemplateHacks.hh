#ifndef TEMPLATE_HACKS_HH
#define TEMPLATE_HACKS_HH
#include <type_traits>

template<bool b> using static_not = std::integral_constant<bool, !b>;

// Cool trick for checking if all booleans in a boolean parameter pack are true
template <bool...> struct bool_pack;
template <bool... v> using all_true  = std::is_same<bool_pack<true, v...>, bool_pack<v..., true>>;
template <bool... v> using all_false = std::is_same<bool_pack<false, v...>, bool_pack<v..., false>>;
template <bool... v> using any_true  = static_not<std::is_same<bool_pack<true, v...>, bool_pack<v..., true>>::value>;
template <bool... v> using any_false = static_not<std::is_same<bool_pack<false, v...>, bool_pack<v..., false>>::value>;

// Are all types of the parameters passed integral?
template <typename... T>
constexpr bool all_integer_parameters() { return all_true<std::is_integral<T>::value...>::value; }
template <typename... T>
constexpr bool all_floating_point_parameters() { return all_true<std::is_floating_point<T>::value...>::value; }

// Copy the cv qualifiers from _CVType to _NonCVType
template<class _CVType, class _NonCVType>
struct CopyCV : public
    std::conditional<std::is_const<_CVType>::value,
             typename std::conditional<std::is_volatile<_CVType>::value,
                                       typename std::   add_volatile<typename std::add_const<_NonCVType>::type>::type,
                                       typename std::remove_volatile<typename std::add_const<_NonCVType>::type>::type
                                      >::type,
             typename std::conditional<std::is_volatile<_CVType>::value,
                                       typename std::   add_volatile<typename std::remove_const<_NonCVType>::type>::type,
                                       typename std::remove_volatile<typename std::remove_const<_NonCVType>::type>::type
                                      >::type
                    >
{ };
template<class _CVType, class _NonCVType>
using CopyCV_t = typename CopyCV<_CVType, _NonCVType>::type;

#endif /* end of include guard: TEMPLATE_HACKS_HH */
