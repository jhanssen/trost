#pragma once

#include <utility>

namespace trost {

template<typename T>
class SharedPtr
{
public:
    SharedPtr()
        : mPtr(nullptr), mRefCount(nullptr), mDeleter(nullptr)
    {
    }

    explicit SharedPtr(T* rawPtr)
        : mPtr(rawPtr), mRefCount(new std::size_t(1)), mDeleter(nullptr)
    {
    }

    template<typename Deleter>
    SharedPtr(T* rawPtr, Deleter deleter)
        : mPtr(rawPtr), mRefCount(new std::size_t(1)),
          mDeleter(new DeleterImpl<Deleter>(std::move(deleter)))
    {
    }

    SharedPtr(const SharedPtr& other)
        : mPtr(other.mPtr), mRefCount(other.mRefCount),
          mDeleter(other.mDeleter)
    {
        if (mRefCount) {
            ++(*mRefCount);
        }
    }

    SharedPtr(SharedPtr&& other) noexcept
        : mPtr(other.mPtr), mRefCount(other.mRefCount),
          mDeleter(other.mDeleter)
    {
        other.mPtr = nullptr;
        other.mRefCount = nullptr;
        other.mDeleter = nullptr;
    }

    SharedPtr& operator=(const SharedPtr& other)
    {
        if (this != &other) {
            release();
            mPtr = other.mPtr;
            mRefCount = other.mRefCount;
            mDeleter = other.mDeleter;
            if (mRefCount) {
                ++(*mRefCount);
            }
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept
    {
        if (this != &other) {
            release();
            mPtr = other.mPtr;
            mRefCount = other.mRefCount;
            mDeleter = other.mDeleter;
            other.mPtr = nullptr;
            other.mRefCount = nullptr;
            other.mDeleter = nullptr;
        }
        return *this;
    }

    ~SharedPtr()
    {
        release();
    }

    T* get() const { return mPtr; }
    T& operator*() const { return *mPtr; }
    T* operator->() const { return mPtr; }

    std::size_t useCount() const
    {
        return mRefCount ? *mRefCount : 0;
    }

    explicit operator bool() const
    {
        return mPtr != nullptr;
    }

private:
    T* mPtr;
    std::size_t* mRefCount;

    struct DeleterBase
    {
        virtual void destroy(T* ptr) = 0;
        virtual ~DeleterBase() = default;
    };

    template<typename Deleter>
    struct DeleterImpl : DeleterBase
    {
        Deleter mDeleter;

        DeleterImpl(Deleter deleter)
            : mDeleter(std::move(deleter))
        {
        }

        void destroy(T* ptr) override
        {
            mDeleter(ptr);
        }
    };

    DeleterBase* mDeleter;

    void release()
    {
        if (mRefCount) {
            if (--(*mRefCount) == 0) {
                if (mDeleter) {
                    mDeleter->destroy(mPtr);
                    delete mDeleter;
                } else {
                    delete mPtr;
                }
                delete mRefCount;
            }
        }
    }
};

} // namespace trost
