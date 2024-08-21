#include "power_ll.h"

// Fast Power Algorithm: (Base ^ Exponent) % Modular
long long power_ll(long long base_, long long exponent_, long long modular_) {
    long long result_ = 1;
    base_ = base_ % modular_;

    while (exponent_ > 0) {
        if (exponent_ & 1) {
            result_ = ((result_ % modular_) * (base_ % modular_)) % modular_;
        }
        base_ = ((base_ % modular_) * (base_ % modular_)) % modular_;
        exponent_ = exponent_ >> 1;
    }

    return result_;
}

// Fast Power Algorithm: Base ^ Exponent
long long power_ll(long long base_, long long exponent_) {
    long long result_ = 1;

    while (exponent_ > 0) {
        if (exponent_ & 1) {
            result_ = result_ * base_;
        }
        base_ = base_ * base_;
        exponent_ = exponent_ >> 1;
    }

    return result_;
}
