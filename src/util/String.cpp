#include "String.h"

using namespace trost;

String::String(const String& other)
    : mSize(other.mSize), mIsSso(other.mIsSso)
{
    if (mIsSso) {
        std::memcpy(mSsoData, other.mSsoData, mSize + 1);
    } else {
        mHeapData = new char[mSize + 1];
        std::memcpy(mHeapData, other.mHeapData, mSize + 1);
    }
}

String::String(String&& other) noexcept
    : mSize(other.mSize), mIsSso(other.mIsSso)
{
    if (mIsSso) {
        std::memcpy(mSsoData, other.mSsoData, mSize + 1);
    } else {
        mHeapData = other.mHeapData;
        other.mHeapData = nullptr;
    }

    other.mSize = 0;
    other.mIsSso = true;
    other.mSsoData[0] = '\0';
}

String& String::operator=(const String& other)
{
    if (this != &other) {
        if (!mIsSso) {
            delete[] mHeapData;
        }

        mSize = other.mSize;
        mIsSso = other.mIsSso;

        if (mIsSso) {
            std::memcpy(mSsoData, other.mSsoData, mSize + 1);
        } else {
            mHeapData = new char[mSize + 1];
            std::memcpy(mHeapData, other.mHeapData, mSize + 1);
        }
    }
    return *this;
}

String& String::operator=(String&& other) noexcept
{
    if (this != &other) {
        if (!mIsSso) {
            delete[] mHeapData;
        }

        mSize = other.mSize;
        mIsSso = other.mIsSso;

        if (mIsSso) {
            std::memcpy(mSsoData, other.mSsoData, mSize + 1);
        } else {
            mHeapData = other.mHeapData;
            other.mHeapData = nullptr;
        }

        other.mSize = 0;
        other.mIsSso = true;
        other.mSsoData[0] = '\0';
    }
    return *this;
}

String& String::operator+=(const String& other)
{
    if (other.mSize == 0) return *this;

    std::size_t newSize = mSize + other.mSize;

    if (newSize <= SSO_CAPACITY) {
        std::memcpy(dataPtr() + mSize, other.dataPtr(), other.mSize + 1);
        mSize = newSize;
    } else {
        char* newData = new char[newSize + 1];
        std::memcpy(newData, dataPtr(), mSize);
        std::memcpy(newData + mSize, other.dataPtr(), other.mSize + 1);

        if (!mIsSso) {
            delete[] mHeapData;
        }

        mHeapData = newData;
        mSize = newSize;
        mIsSso = false;
    }

    return *this;
}

void String::initFromCStr(const char* str)
{
    if (!str) {
        mSize = 0;
        mIsSso = true;
        mSsoData[0] = '\0';
        return;
    }

    mSize = std::strlen(str);
    if (mSize <= SSO_CAPACITY) {
        mIsSso = true;
        std::memcpy(mSsoData, str, mSize + 1);
    } else {
        mIsSso = false;
        mHeapData = new char[mSize + 1];
        std::memcpy(mHeapData, str, mSize + 1);
    }
}
