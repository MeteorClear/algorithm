/*
 * The provided code defines two C++ template structures.
 * 
 * FixedArray<T, N>
 * General-purpose fixed-size container safe for any type,
 * including non-POD types with custom constructors/destructors.
 *
 * PodArray<T, N>
 * Specialized container for POD (Plain Old Data) types.
 * - Guarantees binary compatibility with C-style arrays (same memory layout, padding, alignment).
 * - Safe to use as a drop-in replacement for C arrays without ABI issues.
 *
 * Usage Sample:
 *   // char str[20];
 *   PodArray<char, 20> str;
 *   str = "Hello World!";
 *   // str[6] == 'W';
 */

#pragma once

#ifndef CPP_FIXED_ARRAY_HPP_INCLUDED
#define CPP_FIXED_ARRAY_HPP_INCLUDED

#include <cstring>              // For memset, memcpy
#include <cassert>              // For assert

#include <algorithm>            // For std::fill, std::copy
#include <initializer_list>     // For std::initializer_list
#include <type_traits>          // For is_trivially_copyable, etc.


template <typename T, size_t N>
/*
 * Fixed-size array container with custom initialization and assignment behavior.
 * T: The type of elements in the array.
 * N: The size of the array.
 */
struct FixedArray {

    // -------- Type Aliases --------
    using value_type = T;
    using size_type = size_t;
    using iterator = T*;
    using const_iterator = const T*;

    // C-style array for storage
    T data[N];

    // -------- Constructor --------
    /*
     * Default constructor.
     * Initializes all elements using default value construction (T{}).
     */
    FixedArray() {
        std::fill(data, data + N, T{});
    }

    /*
     * Initializer list constructor.
     * Delegates to the assignment operator for initialization.
     * list: The source initializer list.
     */
    FixedArray(std::initializer_list<T> list) {
        *this = list;
    }

    template <size_t M>
    /*
     * C-style array constructor.
     * Delegates to the assignment operator for initialization.
     * arr: The source C-style array.
     */
    FixedArray(const T(&arr)[M]) {
        *this = arr;
    }

    // -------- Operator --------
    /*
     * Assignment operator from an initializer list.
     * list: The initializer list.
     * return: Reference to the current object.
     */
    FixedArray& operator=(std::initializer_list<T> list) {
        assign(list.begin(), list.size());
        return *this;
    }

    template <size_t M>
    /*
     * Assignment operator from a C-style array.
     * arr: The C-style array.
     * return: Reference to the current object.
     */
    FixedArray& operator=(const T(&arr)[M]) {
        assign(arr, M);
        return *this;
    }

    /*
     * Non-const element access.
     */
    T& operator[](size_t index) {
        assert(index < N && "Index out of range");
        return data[index];
    }

    /*
     * Const element access.
     */
    const T& operator[](size_t index) const {
        assert(index < N && "Index out of range");
        return data[index];
    }

    /*
     * Row pointer access.
     */
    operator T* () {
        return data;
    }

    /*
     * Const row pointer access.
     */
    operator const T* () const {
        return data;
    }

    // -------- Assign Logic --------
    /*
     * Assigns content from a source array.
     * Copies up to N elements from src. 
     * Any remaining elements in the array (if len < N) are default initialized (T{}).
     * src: Pointer to the source elements.
     * len: The number of elements to copy from src.
     */
    void assign(const T* src, size_t len) {
        if (src == data) return;

        size_t copyLen = (len < N) ? len : N;

        if (copyLen > 0) {
            std::copy(src, src + copyLen, data);
        }

        if (copyLen < N) {
            std::fill(data + copyLen, data + N, T{});
        }
    }

    // -------- Methods --------
    constexpr size_t size() const { return N; }

    iterator begin() noexcept { return data; }
    const_iterator begin() const noexcept { return data; }

    iterator end() noexcept { return data + N; }
    const_iterator end() const noexcept { return data + N; }

    const_iterator cbegin() const noexcept { return data; }
    const_iterator cend() const noexcept { return data + N; }

    const void* raw_ptr() const { return data; }
    static constexpr size_t raw_size_bytes() { return sizeof(data); }
};


template <typename T, size_t N>
/*
 * Fixed-size array container specialized for Plain Old Data (POD) types
 * T: The element type (must be trivially copyable, standard layout, and trivially default constructible).
 * N: The size of the array.
 */
struct PodArray {

    // -------- Static Assertions --------
    static_assert(std::is_trivially_copyable<T>::value,
        "Type must be trivially copyable for safe memory operations (memcpy)");
    static_assert(std::is_standard_layout<T>::value,
        "Type must be standard layout for C-style struct compatibility/ABI safety");
    static_assert(std::is_trivially_default_constructible<T>::value,
        "Type must be trivially default constructible to guarantee safe zero-initialization via memset");

    // -------- Type Aliases --------
    using value_type = T;
    using size_type = size_t;

    using iterator = T*;
    using const_iterator = const T*;

    // C-style array for storage
    T data[N];

    // -------- Constructor --------
    /*
     * Default constructor.
     * Initializes all elements using default value construction (T{}).
     */
    PodArray() {
        std::memset(data, 0, sizeof(data));
    }

    /*
     * Initializer list constructor.
     * Delegates to the assignment operator for initialization.
     * list: The source initializer list.
     */
    PodArray(std::initializer_list<T> list) {
        assign(list.begin(), list.size());
    }

    template <size_t M>
    /*
     * C-style array constructor.
     * Delegates to the assignment operator for initialization.
     * arr: The source C-style array.
     */
    PodArray(const T(&arr)[M]) {
        assign(arr, M);
    }

    // -------- Operator --------
    /*
     * Assignment operator from an initializer list.
     * list: The initializer list.
     * return: Reference to the current object.
     */
    PodArray& operator=(std::initializer_list<T> list) {
        assign(list.begin(), list.size());
        return *this;
    }

    template <size_t M>
    /*
     * Assignment operator from a C-style array.
     * arr: The C-style array.
     * return: Reference to the current object.
     */
    PodArray& operator=(const T(&arr)[M]) {
        assign(arr, M);
        return *this;
    }

    /*
     * Non-const element access.
     */
    T& operator[](size_t index) {
        assert(index < N && "Index out of range");
        return data[index];
    }

    /*
     * Const element access.
     */
    const T& operator[](size_t index) const {
        assert(index < N && "Index out of range");
        return data[index];
    }

    /*
     * Row pointer access.
     */
    operator T* () {
        return data;
    }

    /*
     * Const row pointer access.
     */
    operator const T* () const {
        return data;
    }

    // -------- Assign Logic --------
    /*
     * Assigns content from a source array.
     * Copies up to N elements from src. 
     * Any remaining elements in the array (if len < N) are bit-wise zeroing.
     * src: Pointer to the source elements.
     * len: The number of elements to copy from src.
     */
    void assign(const T* src, size_t len) {
        if (src == data) return;

        size_t copyLen = (len < N) ? len : N;

        if (copyLen > 0) {
            std::memcpy(data, src, copyLen * sizeof(T));
        }

        if (copyLen < N) {
            std::memset(data + copyLen, 0, (N - copyLen) * sizeof(T));
        }
    }

    // -------- Methods --------
    constexpr size_t size() const { return N; }

    iterator begin() noexcept { return data; }
    const_iterator begin() const noexcept { return data; }

    iterator end() noexcept { return data + N; }
    const_iterator end() const noexcept { return data + N; }

    const_iterator cbegin() const noexcept { return data; }
    const_iterator cend() const noexcept { return data + N; }

    const void* raw_ptr() const { return data; }
    static constexpr size_t raw_size_bytes() { return sizeof(data); }
};

#endif // CPP_FIXED_ARRAY_HPP_INCLUDED

/*
 * Usage Guide
 *
 * 1. Replacing C-style arrays
 *    Simply replace the type declaration. The memory layout remains compatible.
 *    - char str[30];  =>  PodArray<char, 30> str;
 *
 * 2. Automatic Zero-Initialization
 *    Default constructor automatically zeros out the memory (no manual memset needed).
 *    - [C-Style]
 *      char str[30];
 *      memset(str, 0, sizeof(str));
 *    - [PodArray]
 *      PodArray<char, 30> str; // All bytes are 0
 *
 * 3. Initialization
 *    Supports string literals and initializer lists.
 *    Remaining space is automatically zero-filled.
 *    - PodArray<char, 30> str = "Hello World!";
 *    - PodArray<char, 30> str = { "Hello World!" };
 *    - PodArray<char, 30> str{ "Hello World!" };
 *    - PodArray<char, 30> str = { 'H', 'e', 'l', 'l', 'o' };
 *
 * 4. Assignment
 *    Supports assignment from string literals, initializer lists, and other C-arrays.
 *    Safely handles size differences (truncates if too long, zero-pads if short).
 *    - [C-Style]
 *      memset(str, 0, sizeof(str));
 *      strncpy(str, "Hello", sizeof(str)); // or memcpy
 *    - [PodArray]
 *      str = "Hello World!";
 *      str = { 'H', 'e', 'l', 'l', 'o' };
 *
 *      char sub[10] = "Test";
 *      str = sub; // Copies contents of 'sub' into 'str'
 */
