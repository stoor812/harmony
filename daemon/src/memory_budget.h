#pragma once

#include <cstddef>
#include <mutex>
#include <string>

class MemoryBudget {
public:
    // 256KB cap - matches the spec
    static constexpr size_t MAX_BYTES = 256 * 1024;

    static MemoryBudget& instance();

    // Returns true if allocation was accepted, false if it would exceed budget
    bool allocate(const std::string& label, size_t bytes);
    void deallocate(const std::string& label, size_t bytes);

    size_t used() const;
    size_t remaining() const;
    float percent_used() const;

    void log_status() const;

private:
    MemoryBudget();

    size_t used_;
    mutable std::mutex mutex_;
};