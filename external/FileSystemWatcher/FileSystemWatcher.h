/*
  ==============================================================================

    FileSystemWatcher.h
    Created: 13 Jan 2018 10:31:15pm
    Author:  Roland Rabien

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class FileSystemWatcher
{
public:
    FileSystemWatcher();
    ~FileSystemWatcher();
    
    void addFolder (const juce::File& folder);
    void removeFolder (const juce::File& folder);
    
    class Listener
    {
    public:
        virtual ~Listener()  {}

        virtual void folderChanged (const juce::File)    {}
    };
    
    void addListener (Listener* newListener);

    void removeListener (Listener* listener);
    
private:
    class Impl;
    
    void folderChanged (const juce::File& folder);
    
    juce::ListenerList<Listener> listeners;
    
    juce::OwnedArray<Impl> watched;
};
