#pragma once

#include <cstddef>
#include <new>
#include <utility>

namespace trost {

template<typename T>
class Vector {
public:
    Vector() = default;

    // Copy constructor
    Vector(const Vector& other)
        : mData(nullptr), mSize(other.mSize), mCapacity(other.mCapacity)
    {
        if (mCapacity > 0) {
            mData = static_cast<T*>(operator new(mCapacity * sizeof(T)));
            for (std::size_t i = 0; i < mSize; ++i) {
                new (mData + i) T(other.mData[i]);
            }
        }
    }

    // Move constructor
    Vector(Vector&& other) noexcept
        : mData(other.mData), mSize(other.mSize), mCapacity(other.mCapacity)
    {
        other.mData = nullptr;
        other.mSize = 0;
        other.mCapacity = 0;
    }

    // Copy assignment
    Vector& operator=(const Vector& other)
    {
        if (this != &other) {
            clear();
            if (other.mCapacity > 0) {
                mData = static_cast<T*>(operator new(other.mCapacity * sizeof(T)));
                for (std::size_t i = 0; i < other.mSize; ++i) {
                    new (mData + i) T(other.mData[i]);
                }
                mSize = other.mSize;
                mCapacity = other.mCapacity;
            }
        }
        return *this;
    }

    // Move assignment
    Vector& operator=(Vector&& other) noexcept
    {
        if (this != &other) {
            clear();
            mData = other.mData;
            mSize = other.mSize;
            mCapacity = other.mCapacity;
            other.mData = nullptr;
            other.mSize = 0;
            other.mCapacity = 0;
        }
        return *this;
    }

    // Destructor
    ~Vector()
    {
        clear();
    }

    // Add element to the end
    void push_back(const T& value)
    {
        ensure_capacity();
        new (mData + mSize) T(value);
        ++mSize;
    }

    void push_back(T&& value)
    {
        ensure_capacity();
        new (mData + mSize) T(std::move(value));
        ++mSize;
    }

    // Remove element at index
    void remove_at(std::size_t index)
    {
        if (index >= mSize) return; // optional bounds check

        // Destroy the element at index
        mData[index].~T();

        // Move the rest one step to the left
        for (std::size_t i = index; i < mSize - 1; ++i) {
            new (mData + i) T(std::move(mData[i + 1]));
            mData[i + 1].~T();
        }

        --mSize;
    }

    // Index access
    T& operator[](std::size_t index)
    {
        return mData[index];
    }

    const T& operator[](std::size_t index) const
    {
        return mData[index];
    }

    // Getters
    std::size_t size() const { return mSize; }
    std::size_t capacity() const { return mCapacity; }

private:
    T* mData = nullptr;
    std::size_t mSize = 0;
    std::size_t mCapacity = 0;

    void ensure_capacity()
    {
        if (mSize >= mCapacity) {
            std::size_t new_capacity = mCapacity == 0 ? 1 : mCapacity * 2;
            T* new_data = static_cast<T*>(operator new(new_capacity * sizeof(T)));

            for (std::size_t i = 0; i < mSize; ++i) {
                new (new_data + i) T(std::move(mData[i]));
                mData[i].~T();
            }

            operator delete(mData);
            mData = new_data;
            mCapacity = new_capacity;
        }
    }

    void clear()
    {
        if (mData) {
            for (std::size_t i = 0; i < mSize; ++i) {
                mData[i].~T();
            }
            operator delete(mData);
        }
        mData = nullptr;
        mSize = 0;
        mCapacity = 0;
    }
};

} // namespace trost
