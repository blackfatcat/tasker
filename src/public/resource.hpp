#pragma once

#include <unordered_map>
#include <memory>
#include <cassert>
#include <any>

namespace tskr
{
    /// @brief Wrapper around a tracked resource. Used as a task parameter to query and modify registered resources
    /// @tparam T Type of the inner pointer
    template<typename T>
    class Resource
    {
        T* m_Ptr;
    public:
        Resource() : m_Ptr(nullptr) {};
        Resource(T* ptr) : m_Ptr(ptr){};

        T* operator->() const noexcept { return m_Ptr; }
        T& operator*()  const noexcept { return *m_Ptr; }
    };
    
    /// @brief Storage for registered resources
    class ResourceStore
    {
    public:
        template<typename T>
        void insert(T&& value)
        {
            m_ResourceMap.emplace(typeid(T).hash_code(), std::make_unique<std::any>(std::move(value)));
        }

        template<typename T>
        Resource<T> get()
        {
            assert(m_ResourceMap.contains(typeid(T).hash_code()) && "Resource not registered.");

            T* ref = std::any_cast<T>(m_ResourceMap[typeid(T).hash_code()].get());

            return Resource(ref);
        }

        template<typename T>
        Resource<T> get(size_t type_id)
        {
            assert(m_ResourceMap.contains(type_id) && "Resource not registered.");

            T* ref = std::any_cast<T>(m_ResourceMap[type_id].get());

            return Resource(ref);
        }

    private:
        std::unordered_map<size_t, std::unique_ptr<std::any>> m_ResourceMap{};
    };
} // namespace tskr
