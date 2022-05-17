#ifndef MIDI_H
#define MIDI_H

#import <AudioToolbox/AudioToolbox.h>
#import <Cocoa/Cocoa.h>
#import <TargetConditionals.h>

@interface MIDIChangeHandler : NSObject

- (id)init;
- (OSStatus)startMIDIWithReadProc:(MIDIReadProc)readProc refCon:(void *)refCon;

@end

#endif  // MIDI_H
