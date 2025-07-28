#include "DB.h"
#include <App.h>

using namespace trost;

/*
  The database is designed as follows

  In the app directory there's a directory called "db" which contains one file per
  starting character (0.idx, A.idx - Z.idx) where each file contains a sorted list
  of names where each name is followed by an index into a separate data.idx file that
  contains an on disk linked list of entries. the first entry in the data.idx file
  is a header that points to the first entry and each entry points to the next entry
  in the list. Bitmaps are stored as .iff files in "db/bitmaps" with the name of the
  bitmap being the same as the name of the entry in the data.idx file.

  This allows for fast lookups of entries by name, the find() function will return
  the first entry that matches the name, or nullptr if no entry is found.
  the Entry structure intially just contains the index and the app has to call the
  hydrate() function to load the entry (and optionally a count of subsequent entries)
  this will load the entry name, path and optionally a BitMap. BitMaps are cached
  separately in an LRU data structure so that going back and forth between pages
  don't incur loading the BitMap again.
*/

DB::DB(const String& dir)
    : mDir(dir)
{
}

void DB::createIndex()
{
}

SharedPtr<Entry> DB::all()
{
}

SharedPtr<DB::Entry> DB::find(const String& name)
{
}

void DB::hydrate(const SharedPtr<Entry>& entry, int count)
{
}

void DB::dispose(const SharedPtr<Entry>& entry, int count)
{
}
