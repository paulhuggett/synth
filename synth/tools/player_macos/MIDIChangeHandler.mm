#include "MIDIChangeHandler.h"

#include <cinttypes>

@interface MIDIChangeHandler () {
  MIDIPortRef port_;
  MIDIEndpointRef source_;
}
@end

static NSString *midiTypeName (MIDIObjectType type) {
  switch (type) {
    case kMIDIObjectType_Other:
      return @"kMIDIObjectType_Other";
    case kMIDIObjectType_Device:
      return @"kMIDIObjectType_Device";
    case kMIDIObjectType_Entity:
      return @"kMIDIObjectType_Entity";
    case kMIDIObjectType_Source:
      return @"kMIDIObjectType_Source";
    case kMIDIObjectType_Destination:
      return @"kMIDIObjectType_Destination";

    case kMIDIObjectType_ExternalDevice:
      return @"kMIDIObjectType_ExternalDevice";
    case kMIDIObjectType_ExternalEntity:
      return @"kMIDIObjectType_ExternalEntity";
    case kMIDIObjectType_ExternalSource:
      return @"kMIDIObjectType_ExternalSource";
    case kMIDIObjectType_ExternalDestination:
      return @"kMIDIObjectType_ExternalDestination";
  }
  return @"unknown";
}

@implementation MIDIChangeHandler

- (id)init {
  self = [super init];
  if (self) {
    port_ = 0;
    source_ = 0;
  }
  return self;
}

- (OSStatus)disconnectFromMIDISource {
  OSStatus erc = ::MIDIPortDisconnectSource (self->port_, self->source_);
  if (erc != noErr) {
    return erc;
  }
  self->source_ = 0;
  return noErr;
}

- (OSStatus)connectToMIDIUniqueID:(MIDIUniqueID)uniqueID {
  OSStatus erc = 0;

  MIDIEndpointRef endpoint = 0;
  MIDIObjectType objectType = kMIDIObjectType_Other;
  erc = ::MIDIObjectFindByUniqueID (uniqueID, &endpoint, &objectType);
  if (erc != noErr) {
    NSLog (@"reconnectToMIDIUniqueID.MIDIObjectFindByUniqueID: returned %d", erc);
    return erc;
  }
  if (objectType != kMIDIObjectType_Source && objectType != kMIDIObjectType_ExternalSource) {
    NSLog (@"reconnectToMIDIUniqueID wrong endpoint type");
    return kMIDIWrongEndpointType;
  }

  if (self->source_ != 0) {
    // Cleanly disconnect from our existing source if there is one.
    erc = ::MIDIPortDisconnectSource (self->port_, self->source_);
    self->source_ = 0;
  }

  CFStringRef sourceName = Nil;
  erc = MIDIObjectGetStringProperty (endpoint, kMIDIPropertyName, &sourceName);
  if (erc != noErr) {
    NSLog (@"connectToMIDIUniqueID (child kMIDIPropertyName) failed %d", erc);
  } else {
    NSLog (@"Connecting to MIDI source '%@'", sourceName);
    CFRelease (sourceName);
    sourceName = Nil;
  }

  self->source_ = endpoint;
  return ::MIDIPortConnectSource (self->port_, endpoint, (__bridge void *)self /*connRefCon*/);
}

- (OSStatus)midiObjectRemovedNotificaton:(MIDIObjectAddRemoveNotification const *)n {
  OSStatus erc = noErr;
  NSLog (@"MIDI object removed: parent=%" PRIu32 ", parentType=%@, child=%" PRIu32 ", childType=%@",
         n->parent, midiTypeName (n->parentType), n->child, midiTypeName (n->childType));
  if (n->childType == kMIDIObjectType_Source && n->child == self->source_) {
    NSLog (@"Our MIDI source was removed!");
    erc = [self disconnectFromMIDISource];
  }
  return erc;
}

- (OSStatus)midiObjectAddedNotification:(MIDIObjectAddRemoveNotification const *)n {
  OSStatus erc = noErr;
  NSLog (@"MIDI object added: parent=%" PRIu32 ", parentType=%@, child=%" PRIu32 ", childType=%@",
         n->parent, midiTypeName (n->parentType), n->child, midiTypeName (n->childType));
  if (n->childType == kMIDIObjectType_Source && self->source_ == 0) {
    MIDIUniqueID uniqueID = 0;
    erc = ::MIDIObjectGetIntegerProperty (n->child, kMIDIPropertyUniqueID, &uniqueID);
    if (erc != noErr) {
      NSLog (@"midiChangedNotification.MIDIObjectGetIntegerProperty: returned %d", erc);
    } else {
      erc = [self connectToMIDIUniqueID:uniqueID];
    }
  }
  return erc;
}

- (void)midiChangedNotification:(MIDINotification const *)message {
  switch (message->messageID) {
    // Some aspect of the current MIDISetup has changed.  No data.  Should ignore this message if
    // the other change notification messages are handled.
    case kMIDIMsgSetupChanged:
      break;
    case kMIDIMsgObjectRemoved:
      if (message->messageSize != sizeof (MIDIObjectAddRemoveNotification)) {
        NSLog (@"MIDI Object removed: Message size was %u, expected %zu", message->messageSize,
               sizeof (MIDIObjectAddRemoveNotification));
      } else {
        [self
            midiObjectRemovedNotificaton:reinterpret_cast<MIDIObjectAddRemoveNotification const *> (
                                             message)];
      }
      break;
    case kMIDIMsgObjectAdded:
      if (message->messageSize != sizeof (MIDIObjectAddRemoveNotification)) {
        NSLog (@"MIDI Object Added: Message size was %u, expected %zu", message->messageSize,
               sizeof (MIDIObjectAddRemoveNotification));
      } else {
        [self
            midiObjectAddedNotification:reinterpret_cast<MIDIObjectAddRemoveNotification const *> (
                                            message)];
      }
      break;
    case kMIDIMsgPropertyChanged:
      NSLog (@"MIDI Property Changed");
      break;
    case kMIDIMsgThruConnectionsChanged:
      NSLog (@"MIDI Through Connections Changed");
      break;
    case kMIDIMsgSerialPortOwnerChanged:
      NSLog (@"MIDI Serial Port Owner Changed");
      break;
    case kMIDIMsgIOError:
      NSLog (@"MIDI IO Error");
      break;
    default:
      NSLog (@"MIDI Change Notified (unknown message ID)");
      break;
  }
}

static void midiChanged (MIDINotification const *const message, void *refCon) {
  if (MIDIChangeHandler *const c = (__bridge MIDIChangeHandler *)refCon) {
    [c midiChangedNotification:message];
  }
}

- (OSStatus)startMIDIWithReadProc:(MIDIReadProc)readProc refCon:(void *__nullable)refCon {
  MIDIClientRef client = 0;
  OSStatus erc = MIDIClientCreate (CFSTR ("MIDI Client"), midiChanged,
                                   (__bridge void *)self /*notifyRefCon*/, &client);
  if (erc != noErr) {
    NSLog (@"MIDIClientCreate: %u", erc);
  } else {
    erc = MIDIInputPortCreate (client, CFSTR ("MIDI Input Port"), readProc, refCon, &self->port_);
    if (erc != noErr) {
      NSLog (@"MIDIInputPortCreate: %u", erc);
    }
  }

  if (erc == noErr) {
    ItemCount const sources = MIDIGetNumberOfSources ();
    NSLog (@"There are %u sources", static_cast<unsigned> (sources));
    if (sources == 0) {
      NSLog (@"No MIDI source was available");
      return noErr;
    }

    self->source_ = MIDIGetSource (0);

    CFStringRef pname = Nil;
    MIDIObjectGetStringProperty (self->source_, kMIDIPropertyName, &pname);
    NSLog (@"Connected to: %@", pname);
    CFRelease (pname);

    erc = MIDIPortConnectSource (self->port_, self->source_, (__bridge void *)self /*connRefCon*/);
    if (erc != noErr) {
      NSLog (@"MIDIPortConnectSource: %d", erc);
    }
  }

  return erc;
}

@end
