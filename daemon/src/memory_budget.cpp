#include "memory_budget.h"
#include "logger.h"

#include <stdexcept>

MemoryBudget& MemoryBudget::instance() {
    static MemoryBudget instance;
    return instance;
}

MemoryBudget::MemoryBudget() : used_(0) {
    Logger::instance().info("MEMORY",
        "Memory budget initialized - cap: " +
        std::to_string(MAX_BYTES / 1024) + "KB");
}

bool MemoryBudget::allocate(const std::string& label, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (used_ + bytes > MAX_BYTES) {
        Logger::instance().error("MEMORY",
            "BUDGET EXCEEDED: Cannot allocate " +
            std::to_string(bytes) + " bytes for [" + label +
            "] - used: " + std::to_string(used_) +
            "/" + std::to_string(MAX_BYTES));
        return false;
    }

    used_ += bytes;
    Logger::instance().info("MEMORY",
        "Allocated " + std::to_string(bytes) +
        " bytes for [" + label + "] - used: " +
        std::to_string(used_) + "/" + std::to_string(MAX_BYTES) +
        " (" + std::to_string(static_cast<int>(percent_used())) + "%)");
    return true;
}

void MemoryBudget::deallocate(const std::string& label, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (bytes > used_) used_ = 0;
    else used_ -= bytes;

    Logger::instance().info("MEMORY",
        "Freed " + std::to_string(bytes) +
        " bytes for [" + label + "] - used: " +
        std::to_string(used_) + "/" + std::to_string(MAX_BYTES));
}

size_t MemoryBudget::used() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return used_;
}

size_t MemoryBudget::remaining() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return MAX_BYTES - used_;
}

float MemoryBudget::percent_used() const {
    return (static_cast<float>(used_) / MAX_BYTES) * 100.0f;
}

void MemoryBudget::log_status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Logger::instance().info("MEMORY",
        "Status: " + std::to_string(used_) + "/" +
        std::to_string(MAX_BYTES) + " bytes (" +
        std::to_string(static_cast<int>((static_cast<float>(used_) / MAX_BYTES) * 100)) +
        "%) used");
}