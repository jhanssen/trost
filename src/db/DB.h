#pragma once

#include "util/String.h"
#include "util/SharedPtr.h"

struct BitMap;

namespace trost {

class DB
{
public:
    DB(const String& dir);

    void createIndex();

    struct Entry
    {
        String name;
        String path;
        SharedPtr<BitMap> bitmap;

        SharedPtr<Entry> next;

        UWORD offset;
    };

    SharedPtr<Entry> find(const String& name);
    SharedPtr<Entry> all();

    void hydrate(const SharedPtr<Entry>& entry, int count);
    void dispose(const SharedPtr<Entry>& entry, int count);

private:
    String mDir;
};

}; // namespace trost
