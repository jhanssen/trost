#pragma once

#include <cstddef>
#include <cstring>
#include <utility>

namespace trost {

class String
{
public:
    String()
        : mSize(0), mIsSso(true)
    {
        mSsoData[0] = '\0';
    }

    String(const char* str)
    {
        initFromCStr(str);
    }

    String(const String& other);
    String(String&& other) noexcept;

    ~String()
    {
        if (!mIsSso) {
            delete[] mHeapData;
        }
    }

    String& operator=(const String& other);
    String& operator=(String&& other) noexcept;

    String& operator+=(const String& other);

    char& operator[](std::size_t index) { return dataPtr()[index]; }
    const char& operator[](std::size_t index) const { return dataPtr()[index]; }

    const char* c_str() const { return dataPtr(); }

    std::size_t size() const { return mSize; }

private:
    static constexpr std::size_t SSO_CAPACITY = 15;

    union {
        char* mHeapData;
        char mSsoData[SSO_CAPACITY + 1];  // +1 for null terminator
    };

    std::size_t mSize;
    bool mIsSso;

    char* dataPtr() { return mIsSso ? mSsoData : mHeapData; }
    const char* dataPtr() const { return mIsSso ? mSsoData : mHeapData; }

    void initFromCStr(const char* str);
};

} // namespace trost
