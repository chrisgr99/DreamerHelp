/** SPEECH ON THE MAC, WITH THE VOICE KEPT LOADED.

A reading used to start `say` each time. Nearly all of the pause before it spoke was that: a new
process, and the system voice loaded from disk, about a second with a downloaded voice whatever
the length of the text — turning the words into sound is quick once the voice is in memory. So the
synthesiser is made once, in the plugin, and kept. It speaks in the system voice, the one `say`
uses, and a new reading stops the one before it.

IN A FILE OF ITS OWN because it is Objective-C++, compiled on the Mac only — see the Makefile. The
functions it offers are declared in Help.cpp. */
#import <AppKit/AppKit.h>
#include <string>

static NSSpeechSynthesizer* gSynth = nil;
static float gRate = 200.f;

static NSSpeechSynthesizer* synth() {
	if (!gSynth)
		gSynth = [[NSSpeechSynthesizer alloc] initWithVoice:nil];
	return gSynth;
}

void helpMacSilence() {
	@autoreleasepool {
		if (gSynth && [gSynth isSpeaking])
			[gSynth stopSpeaking];
	}
}

bool helpMacSpeaking() {
	@autoreleasepool {
		return gSynth && [gSynth isSpeaking];
	}
}

/** LOADS THE VOICE BEFORE IT IS NEEDED, so the first reading has no pause either: a word,
spoken at no volume. A space is not enough; the voice is only fully loaded once it has said
something. Making the synthesiser holds up the frame it is made in, a little under a second,
once, when Help mode is first on. */
void helpMacWarm(int wordsPerMinute) {
	@autoreleasepool {
		gRate = (float) wordsPerMinute;
		if (gSynth)
			return;
		NSSpeechSynthesizer* s = synth();
		[s setVolume:0.f];
		[s startSpeakingString:@"ready"];
	}
}

void helpMacSay(const std::string& text, int wordsPerMinute) {
	@autoreleasepool {
		gRate = (float) wordsPerMinute;
		NSSpeechSynthesizer* s = synth();
		if ([s isSpeaking])
			[s stopSpeaking];
		[s setVolume:1.f];
		[s setRate:gRate];
		NSString* words = [NSString stringWithUTF8String:text.c_str()];
		if (words)
			[s startSpeakingString:words];
	}
}
