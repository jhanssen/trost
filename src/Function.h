#pragma once

#include <utility>

namespace trost {

template<typename>
class Function;

template<typename R, typename... Args>
class Function<R(Args...)>
{
public:
    Function() = default;

    // Construct from any callable
    template<typename T>
    Function(T f)
    {
        using CallableType = CallableImpl<T>;

        mCallable = new CallableType(std::move(f));
        mInvoker = [](void* obj, Args... args) -> R {
            return static_cast<CallableType*>(obj)->call(args...);
        };
        mDeleter = [](void* obj) {
            delete static_cast<CallableType*>(obj);
        };
        mCloner = [](void* obj) -> void* {
            return new CallableType(*static_cast<CallableType*>(obj));
        };
    }

    // Copy constructor
    Function(const Function& other)
        : mInvoker(other.mInvoker), mDeleter(other.mDeleter), mCloner(other.mCloner)
    {
        mCallable = (other.mCallable && mCloner) ? mCloner(other.mCallable) : nullptr;
    }

    // Move constructor
    Function(Function&& other) noexcept
        : mCallable(other.mCallable),
          mInvoker(other.mInvoker),
          mDeleter(other.mDeleter),
          mCloner(other.mCloner)
    {
        other.mCallable = nullptr;
        other.mInvoker = nullptr;
        other.mDeleter = nullptr;
        other.mCloner = nullptr;
    }

    // Copy assignment
    Function& operator=(const Function& other)
    {
        if (this != &other) {
            destroy();
            mInvoker = other.mInvoker;
            mDeleter = other.mDeleter;
            mCloner = other.mCloner;
            mCallable = (other.mCallable && mCloner)
            ? mCloner(other.mCallable)
            : nullptr;
        }
        return *this;
    }

    // Move assignment
    Function& operator=(Function&& other) noexcept
    {
        if (this != &other) {
            destroy();
            mCallable = other.mCallable;
            mInvoker = other.mInvoker;
            mDeleter = other.mDeleter;
            mCloner = other.mCloner;

            other.mCallable = nullptr;
            other.mInvoker = nullptr;
            other.mDeleter = nullptr;
            other.mCloner = nullptr;
        }
        return *this;
    }

    // Destructor
    ~Function()
    {
        destroy();
    }

    // Call the stored callable
    R operator()(Args... args) const
    {
        return mInvoker(mCallable, std::forward<Args>(args)...);
    }

private:
    // Internal callable wrapper
    template<typename T>
    struct CallableImpl
    {
        T func;
        explicit CallableImpl(T f) : func(std::move(f)) {}
        R call(Args... args) {
            return func(std::forward<Args>(args)...);
        }
    };

    void destroy()
    {
        if (mCallable && mDeleter) {
            mDeleter(mCallable);
        }
        mCallable = nullptr;
        mInvoker = nullptr;
        mDeleter = nullptr;
        mCloner = nullptr;
    }

    void* mCallable = nullptr;
    R (*mInvoker)(void*, Args...) = nullptr;
    void (*mDeleter)(void*) = nullptr;
    void* (*mCloner)(void*) = nullptr;
};

} // namespace trost
