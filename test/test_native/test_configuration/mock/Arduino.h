#pragma once

#include <algorithm>

// Mock Arduino functions for testing
// Use inline functions instead of macros to avoid conflicts with std library
template<typename T>
inline T min(T a, T b) { return std::min(a, b); }

template<typename T>
inline T max(T a, T b) { return std::max(a, b); }
