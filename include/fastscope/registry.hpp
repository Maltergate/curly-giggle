#pragma once
/// @file registry.hpp
/// @brief Generic type registry mapping string keys to factory-created instances.
/// @ingroup core_interfaces

#include "fastscope/interfaces.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fastscope {

// ── Primary template ──────────────────────────────────────────────────────────

/// @brief Generic type registry mapping string keys to factory functions.
/// @tparam Interface The base interface type. All registered types must derive from it.
/// @details Maps string identifiers to factory functions that produce `unique_ptr<Interface>`.
///          Keys are stored in insertion order. Instances are created on demand via create().
template<typename Interface>
class Registry {
public:
    using FactoryFn = std::function<std::unique_ptr<Interface>()>;

    /// Register a concrete type T under the given key.
    /// T must be default-constructible.
    template<std::derived_from<Interface> T>
        requires std::default_initializable<T>
    void register_type(std::string_view key)
    {
        auto k = std::string(key);
        if (m_key_order.end() ==
            std::find(m_key_order.begin(), m_key_order.end(), k))
        {
            m_key_order.push_back(k);
        }
        m_factories[k] = []() -> std::unique_ptr<Interface> {
            return std::make_unique<T>();
        };
    }

    /// Register a custom factory function under the given key.
    void register_factory(std::string_view key, FactoryFn factory)
    {
        auto k = std::string(key);
        if (m_key_order.end() ==
            std::find(m_key_order.begin(), m_key_order.end(), k))
        {
            m_key_order.push_back(k);
        }
        m_factories[k] = std::move(factory);
    }

    /// Create an instance by key.  Returns nullptr for unknown keys.
    [[nodiscard]] std::unique_ptr<Interface> create(std::string_view key) const
    {
        auto it = m_factories.find(std::string(key));
        return it != m_factories.end() ? it->second() : nullptr;
    }

    /// Returns all registered keys in insertion order.
    [[nodiscard]] const std::vector<std::string>& keys() const noexcept
    {
        return m_key_order;
    }

    /// Returns true if the key is registered.
    [[nodiscard]] bool contains(std::string_view key) const
    {
        return m_factories.contains(std::string(key));
    }

    /// Number of registered types.
    [[nodiscard]] std::size_t size() const noexcept { return m_factories.size(); }

    /// True if no types are registered.
    [[nodiscard]] bool empty() const noexcept { return m_factories.empty(); }

private:
    std::unordered_map<std::string, FactoryFn> m_factories;
    std::vector<std::string>                   m_key_order;   // preserves insertion order
};

// ── Convenience aliases ────────────────────────────────────────────────────────

/// @brief Registry specialisation for IPlotType.
using PlotRegistry      = Registry<IPlotType>;
/// @brief Registry specialisation for ISignalOperation.
using OperationRegistry = Registry<ISignalOperation>;
/// @brief Registry specialisation for IVisualizationTool.
using ToolRegistry      = Registry<IVisualizationTool>;

} // namespace fastscope
