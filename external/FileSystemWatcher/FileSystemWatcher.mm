/*
  ==============================================================================

    FileSystemWatcher.cpp
    Created: 13 Jan 2018 10:31:15pm
    Author:  Roland Rabien

  ==============================================================================
*/

#import <Foundation/Foundation.h>

#include "FileSystemWatcher.h"

using namespace juce;

//==============================================================================
class FileSystemWatcher::Impl
{
public:
    Impl (FileSystemWatcher& o, File f) : owner (o), folder (f)
    {
        NSString* newPath = [NSString stringWithUTF8String:folder.getFullPathName().toRawUTF8()];
        
        paths = [[NSArray arrayWithObject:newPath] retain];
        context.version         = 0L;
        context.info            = this;
        context.retain          = nil;
        context.release         = nil;
        context.copyDescription = nil;
        
        stream = FSEventStreamCreate (kCFAllocatorDefault, callback, &context, (CFArrayRef)paths, kFSEventStreamEventIdSinceNow, 1.0, kFSEventStreamCreateFlagUseCFTypes);
        if (stream)
        {
            FSEventStreamScheduleWithRunLoop (stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
            FSEventStreamStart (stream);
        }
        
    }
    
    ~Impl()
    {
        if (stream)
        {
            FSEventStreamStop (stream);
            FSEventStreamInvalidate (stream);
            FSEventStreamRelease (stream);
        }
    }
    
    static void callback (ConstFSEventStreamRef streamRef, void* clientCallBackInfo, size_t numEvents, void* eventPaths, const FSEventStreamEventFlags* eventFlags,
                   const FSEventStreamEventId* eventIds)
    {
        Impl* impl = (Impl*)clientCallBackInfo;
        impl->owner.folderChanged (impl->folder);
    }
    
    FileSystemWatcher& owner;
    File folder;
    
    NSArray* paths;
    FSEventStreamRef stream;
    struct FSEventStreamContext context;
};

//==============================================================================
FileSystemWatcher::FileSystemWatcher()
{
}

FileSystemWatcher::~FileSystemWatcher()
{
}

void FileSystemWatcher::addFolder (const File& folder)
{
    watched.add (new Impl (*this, folder));
}

void FileSystemWatcher::removeFolder (const File& folder)
{
    for (int i = 0; --i >= 0;)
    {
        if (watched[i]->folder == folder)
        {
            watched.remove (i);
            break;
        }
    }
}



