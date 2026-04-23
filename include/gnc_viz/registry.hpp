#pragma once

// ── Generic type registry ──────────────────────────────────────────────────────
//
// Registry<Interface, Concept> maps string keys to factory functions.
// Each factory creates a unique_ptr<Interface> from a default-constructible type.
//
// Usage:
//   Registry<IPlotType, PlotTypeConcept> plot_registry;
//   plot_registry.register_type<TimeSeriesPlot>("timeseries");
//   auto plot = plot_registry.create("timeseries");  // → unique_ptr<IPlotType>
//
// Three convenience aliases are pre-defined:
//   PlotRegistry, OperationRegistry, ToolRegistry
//
// Conventions:
//  - register_type<T>() is constexpr-friendly and always overwrites existing key.
//  - create() returns nullptr for unknown keys (callers must check).
//  - get_all_keys() returns keys in insertion order.

#include "gnc_viz/interfaces.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gnc_viz {

// ── Primary template ──────────────────────────────────────────────────────────

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

using PlotRegistry      = Registry<IPlotType>;
using OperationRegistry = Registry<ISignalOperation>;
using ToolRegistry      = Registry<IVisualizationTool>;

} // namespace gnc_viz
