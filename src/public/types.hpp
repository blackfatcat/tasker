#include <type_traits>
#include <tuple>

namespace tskr
{
    /// @tparam Ts a pack of types that will mark schedules for parallel execution
    template<typename... Ts>
    struct Parallel {};

    namespace impl {
        template <typename T>
        struct is_parallel : std::false_type {};

        template <typename ...Ts>
        struct is_parallel<Parallel<Ts...>> : std::true_type {};

        template <typename T>
        struct parallel_inner;

        template <typename... Ts>
        struct parallel_inner<Parallel<Ts...>>
        {
            using types = std::tuple<Ts...>;
        };
    }
} // namespace tskr
