#pragma once
/** WHAT A MODULE IS, WITHOUT LEAVING THE RACK.

A maker's manual is a web page, and reaching it means leaving what you were doing, finding the
right page among a hundred modules, and reading six hundred words to learn what three jacks do.
The question being answered here is smaller than that: what is this thing, and what goes in its
ports. A badge on the module answers it where the module is.

POINT FORM, FROM THE MAKER'S OWN MANUAL, IN OUR WORDS. Each entry is a first line saying what the
module is, then one line per port or control, each led by the name printed on the panel, with
menu options gathered at the end. Written by reading the manual rather than generated from the
module, because a generated sheet can only repeat the port names that are already on the panel —
which on the modules that most need explaining are numbers.

The text lives in data/plugins/<PluginSlug>.yaml and is flattened into HelpText.cpp by
tools/build.py. Edit the YAML, never the generated table.

NOT ON EVERY MODULE, AND THAT IS VISIBLE. A plugin with no entry yet gets a panel saying so
rather than a guess. Nothing here is inferred: an entry exists because somebody read the source
or the manual, and every fact in it cites the place they read.

ONE MODULE FOR THE WHOLE RACK, AND A SWITCH TO GIVE THE GESTURE BACK. Help is a thing a rack has
once and it acts on every module, so it is a single module with no ports rather than something
each panel carries. The switch matters because Option-click is not ours: other plugins use it,
Rack uses Option-drag to pan, and a mode that silently claims a gesture across somebody else's
panel is one that breaks their module with no way to tell what did it. */
#include "plugin.hpp"

#include <string>
#include <vector>

/** One module's text, in the generated table.

AND WHICH LINE BELONGS TO WHICH JACK. The lines are written one per control, so a click on a
control can be answered with the line that describes it — but only if something records that this
input, by number, is the one line four is about. That is what the three index tables are: one
entry per input, output and parameter, holding the line number that covers it, or -1 where
nothing does.

Written by hand in the JSON beside the text, because it cannot be derived: the makers who most
need explaining are exactly the ones who leave every port unnamed. */
struct HelpEntry {
	const char* plugin;
	const char* model;
	const char* const* lines;
	int count;
	const short* inputs;
	int inputCount;
	const short* outputs;
	int outputCount;
	const short* params;
	int paramCount;
	/** WHAT EACH JACK CARRIES, as a Palette.hpp FAM_ number, or -1 where we did not say.
	
	Read off the panels while the help was written, and consulted by anything that colours a
	jack — the same reading serving both. */
	const signed char* inFamilies;
	int inFamilyCount;
	const signed char* outFamilies;
	int outFamilyCount;
	/** WHAT EACH INPUT EXPECTS, as an index into HELP_PROP_TEXT, or -1 where nothing is known.

	A sensible voltage range, whether the signal is continuous or stepped, whether the port takes
	polyphony: the questions the Rack forum keeps answering with a scope and a test rig. Two of the
	three fall out of the family already recorded beside this, so they are worked out by
	tools/build.py and pooled. SHORT, NOT A BYTE: there were a handful of distinct phrases while the
	shapes were derived from families, and once agents began reading real ranges out of real source
	the pool went past 127 in an afternoon and the table stopped compiling. */
	const short* inProps;
	int inPropCount;
	/** THE SAME FOR OUTPUTS, where the question is what comes OUT rather than what to send in.
	Unipolar or bipolar is the one a patch usually turns on: a 0-10V envelope into something
	expecting ±5V is the commonest silent mistake in a rack. */
	const short* outProps;
	int outPropCount;
};

/** The pooled phrases the indices above point into. */
extern const char* const HELP_PROP_TEXT[];
extern const int HELP_PROP_TEXT_COUNT;

/** THE GESTURE'S NAME, AS SHORT AS THE PANEL NEEDS IT.

Symbols on a Mac, because a two-line row has about twelve characters to spend. DejaVuSans, which
is the panel font, carries U+2325 and U+21E7 — checked, not assumed. Windows and Linux have no
symbol anybody reads at a glance, so they get the words they are used to.

OPTION, AND WHERE IT IS HANDLED IS THE WHOLE POINT. Cmd+Shift is Rack's clone-the-top-cable on
a port, so a question there began a cable and took it back. Control cannot be used on a Mac at
all: Rack's mouse callback turns Control-click into a RIGHT click and Control-Shift-click into a
MIDDLE click, stripping the modifier, before any widget sees it.

Option looked impossible too, because ScrollWidget consumes option-click BEFORE its children so
that option-drag can pan the rack. It is not: that only defeats a handler parented to the rack.
This plugin's overlay is a child of the SCENE, added after the rack's scroll view, which is why
option-click has always opened the clip-on menu on a port. The help is answered from that same
overlay, so option reaches it untouched — and nothing of Rack's is claimed, since the only thing
Rack does with option is pan, which still works everywhere except over a control.

SPEECH IS HANDLED SEPARATELY, and has to be: `say` reads a symbol as nothing at all. helpSpeech
turns these back into words on the way to the voice, which is the same split every other piece of
panel shorthand gets — what is on screen matches what is printed, and the spoken copy is
computed from it. */
#if defined ARCH_MAC
	#define HELP_MOD_NAME "⌥"
#else
	#define HELP_MOD_NAME "Alt"
#endif

/** What kind of thing was clicked. */
enum HelpKind { HELP_INPUT, HELP_OUTPUT, HELP_PARAM };
extern const HelpEntry HELP[];
extern const int HELP_COUNT;

/** The lines for a module, or an empty vector if nobody has written any. */
std::vector<std::string> helpFor(const std::string& plugin, const std::string& model);

/** What the help entries say this jack carries, as a Palette.hpp FAM_ number, or -1 if they say
nothing. A port-colouring consumer asks this. */
int helpFamilyFor(const std::string& plugin, const std::string& model, bool isOutput, int port);

/** What this input expects — range, shape, polyphony — or empty where nothing is established.

SHOWN UNDER THE LINE, NOT INSTEAD OF IT. The line says what the jack is for, which is what
somebody asks first; this says what to send it, which is what they ask next and what Rack itself
has never told anybody. */
std::string helpPropsFor(const std::string& plugin, const std::string& model,
	bool isOutput, int port);

/** The one line covering this jack or knob, or empty if nothing does. */
std::string helpForControl(const std::string& plugin, const std::string& model,
	HelpKind kind, int index);

/** Keeps the click-catcher on the rack, and switches option-click help on or off.

HOW IT IS ASKED. Option-click any jack or knob — Alt elsewhere — and a note appears above the
pointer saying what that one control is. The same on the module's TITLE BAND, the top of its
panel, says what the module is. Anywhere else on the panel puts the note away, as does an
ordinary click anywhere at all, or Escape. Nothing is added to anybody's panel.

Cheap to call every frame. This now only keeps the note on the scene and watches for Escape: the
click itself arrives through the scene overlay, for the reason set out above. */
void helpStep(bool enabled);

/** Whether opening the panel also reads it out.

SPEECH IS THE ONLY WAY THE TEXT CAN BE HEARD. Rack publishes nothing to the accessibility API, so
no reader can find this text however it is selected or copied; the plugin says it itself, with
`say`, in the same voice the demo system uses. Mac only, deliberately — this is a personal tool
on a Mac, and everywhere else the panel is still there to be read with the eyes. */
void helpSetSpeak(bool on);

/** Whether the rack is in help mode at this moment.

FOR ANY OTHER GESTURE IN THE SAME RACK. A plugin that acts on a click of its own — carrying a
cable off a jack, most of all — runs from its own handler high in the scene, where this catcher
cannot get in front of it. A mode that answers a question about a jack while something else
picks a cable up off the same jack is not a mode. Anything that acts on a click should ask this
first and stand down. */
bool helpModeOn();

/** Answers a help click at this point, given in the RACK's coordinates.

CALLED FROM THE SCENE OVERLAY, because that is the only place an option-click arrives.
ScrollWidget consumes option-click before its children so that option-drag can pan the rack, so
anything parented to the rack never sees one — which is why the first attempt at this gesture
failed. The overlay that already answers option-click on a port is a child of the SCENE, above
that scroll view, and this is the same click asked of the same overlay.

Returns whether the click was taken. */
bool helpClickAt(math::Vec rackPos);

/** Puts the note away, for any ordinary click that is not a question. */
/** How much bigger than designed the note is drawn, and the limits the menu offers.

Kept in the user folder rather than in the patch: somebody who needs larger text needs it in
every patch, including the ones other people wrote. */
void helpSetScale(float scale);
float helpScale();
extern const float HELP_SCALE_MIN;
extern const float HELP_SCALE_MAX;

void helpDismissNote();

/** Puts away anything on the screen and stops anything being read. Called when the last module
that asked for help leaves. */
void helpRemoveAll();
