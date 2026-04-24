# Prompt: Add a New Signal Operation to FastScope

Use this prompt when you need to implement a mathematical transformation that creates a derived signal
from one or more input signals (e.g. subtraction, norm, derivative, low-pass filter, FFT, etc.).

---

## Context

FastScope signal operations implement `ISignalOperation` (`include/fastscope/interfaces.hpp`).
They transform N `SignalBuffer` inputs into one output `SignalBuffer`.
Built-in operations are in `include/fastscope/signal_ops.hpp` and `src/lib/signal_ops.cpp`.
They are registered in `src/lib/operation_registry.cpp`.

Key files to read before starting:
- `include/fastscope/interfaces.hpp` — ISignalOperation interface
- `include/fastscope/signal_ops.hpp` — existing operations (AddOp, SubtractOp, ScaleOp, MagnitudeOp)
- `src/lib/signal_ops.cpp` — implementations
- `src/lib/operation_registry.cpp` — registration
- `include/fastscope/signal_buffer.hpp` — SignalBuffer structure
- `include/fastscope/error.hpp` — Result<T>, ErrorCode, FASTSCOPE_TRY

---

## Step 1 — Add the class declaration

File: `include/fastscope/signal_ops.hpp`

Add your class **after the existing operations**:

```cpp
/// @brief [One-line description].
/// @details [Full description of what the operation computes].
///          Input count: [N or -1 for variadic].
class MyOp final : public ISignalOperation {
public:
    /// @brief Human-readable name shown in the derived-signal UI.
    [[nodiscard]] std::string_view name() const noexcept override { return "MyOp"; }

    /// @brief Unique registry key.
    [[nodiscard]] std::string_view id() const noexcept override { return "myop"; }

    /// @brief Number of required inputs. Returns -1 for variadic (any number ≥ 2).
    [[nodiscard]] int input_count() const noexcept override { return 2; }

    fastscope::Result<std::shared_ptr<SignalBuffer>>
    execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) override;
};
```

---

## Step 2 — Implement the operation

File: `src/lib/signal_ops.cpp`

```cpp
fastscope::Result<std::shared_ptr<SignalBuffer>>
MyOp::execute(std::span<const std::shared_ptr<SignalBuffer>> inputs) {
    // Validate input count
    if (inputs.size() != 2) {
        return std::unexpected(fastscope::Error{
            fastscope::ErrorCode::InvalidArgument,
            "MyOp requires exactly 2 inputs",
            std::format("got {}", inputs.size())
        });
    }

    // Validate inputs are non-null and same length
    if (!inputs[0] || !inputs[1]) {
        return std::unexpected(fastscope::Error{
            fastscope::ErrorCode::InvalidArgument, "Null input buffer", ""});
    }
    const auto& a = inputs[0]->values();
    const auto& b = inputs[1]->values();
    if (a.size() != b.size()) {
        return std::unexpected(fastscope::Error{
            fastscope::ErrorCode::DimensionMismatch,
            "Input buffers have different sizes",
            std::format("{} vs {}", a.size(), b.size())
        });
    }

    // Compute output
    auto result = std::make_shared<SignalBuffer>();
    result->time   = inputs[0]->time;   // inherit time axis from first input
    result->values.resize(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
        result->values[i] = /* your computation here */;
    }
    return result;
}
```

**Rules:**
- Never throw. Return `std::unexpected(...)` on all error paths.
- Use `FASTSCOPE_TRY(expr)` to propagate errors from sub-calls.
- The output `SignalBuffer::time` must always be set (copy from the reference input).
- For variadic ops, validate `inputs.size() >= 2` before iterating.

---

## Step 3 — Register the operation

File: `src/lib/operation_registry.cpp`

```cpp
#include "fastscope/signal_ops.hpp"

OperationRegistry create_operation_registry() {
    OperationRegistry reg;
    reg.register_type<AddOp>("add");
    // ... existing registrations ...
    reg.register_type<MyOp>("myop");   // ← add this line
    return reg;
}
```

---

## Step 4 — Add unit tests

File: `src/tests/test_signal_ops.cpp`

Add test cases for:
1. Normal operation with valid inputs
2. Error case: wrong number of inputs
3. Error case: mismatched sizes
4. Edge case: single-element buffers
5. Edge case: zero-length buffers (should return empty output, not crash)

```cpp
TEST_CASE("MyOp computes correctly", "[signal_ops]") {
    auto a = std::make_shared<SignalBuffer>();
    a->time   = {0.0, 1.0, 2.0};
    a->values = {1.0, 2.0, 3.0};

    auto b = std::make_shared<SignalBuffer>();
    b->time   = {0.0, 1.0, 2.0};
    b->values = {4.0, 5.0, 6.0};

    MyOp op;
    auto result = op.execute(std::array{a, b});
    REQUIRE(result.has_value());
    REQUIRE(result->get()->values == std::vector<double>{/* expected */});
}
```

---

## Step 5 — CMakeLists.txt (usually not needed)

`signal_ops.cpp` is already in `src/lib/CMakeLists.txt`. Only add a new `.cpp` file if you
split the operation into its own file.

---

## Acceptance Criteria

Before committing:
1. `cmake --build build -j$(sysctl -n hw.ncpu)` — zero errors
2. `./build/bin/fastscope_tests` — all tests pass including your new ones
3. Add `///` Doxygen comments to the class and all public members in the header
4. The operation appears in the derived-signal creation modal in the UI
