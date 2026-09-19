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

The database is data/help/<PluginSlug>/<ModuleSlug>.json, which ships with the plugin and is read
one module at a time; what it was worked out from is in data/research and does not ship. A maker's
own help files and a user's are read the same way and come first — see HelpData.cpp and
design/help-database.md.

NOT ON EVERY MODULE, AND THAT IS VISIBLE. A plugin with no entry yet gets a panel saying so
rather than a guess. Nothing here is inferred: an entry exists because somebody read the source
or the manual, and every fact in it cites the place they read.

ONE MODULE FOR THE WHOLE RACK, AND A SWITCH TO GIVE THE GESTURE BACK. Help is a thing a rack has
once and it acts on every module, so it is a single module with no ports rather than something
each panel carries. The switch matters because Option-click is not ours: other plugins use it,
Rack uses Option-drag to pan, and a mode that silently claims a gesture across somebody else's
panel is one that breaks their module with no way to tell what did it. */
#include "plugin.hpp"

#include <map>
#include <string>
#include <vector>


/** What kind of thing was clicked. In the order a help file lists them, so the numbers can index
the per-kind tables below. */
enum HelpKind { HELP_INPUT, HELP_OUTPUT, HELP_PARAM, HELP_LIGHT, HELP_KINDS };

/** WHERE A PIECE OF TEXT CAME FROM — see design/help-database.md. In order of precedence: the
maker's own help file, then a user's, then this plugin's database. */
enum HelpSource { HELP_FROM_NONE, HELP_FROM_MAKER, HELP_FROM_USER, HELP_FROM_DATABASE,
	HELP_SOURCES };
const char* helpSourceName(HelpSource s);

struct HelpText {
	std::string text;
	HelpSource from = HELP_FROM_NONE;
	HelpText() {}
	HelpText(const std::string& t, HelpSource f) : text(t), from(f) {}
};

/** EVERYTHING KNOWN ABOUT ONE MODULE, the three sources already laid over each other: for each
item, the text of the first source that has any. */
struct HelpModuleData {
	HelpText description;
	std::vector<std::string> notes;
	HelpSource notesFrom = HELP_FROM_NONE;
	/** Settings with no knob, reached from the module's right-click menu. */
	std::vector<std::string> menu;
	HelpSource menuFrom = HELP_FROM_NONE;
	/** Per HelpKind, control number to text. */
	std::map<int, HelpText> items[HELP_KINDS];
	/** WHAT EACH JACK EXPECTS — range, shape, polyphony — for inputs and then outputs. Shown under
	the jack's own line: the line says what it is for, this says what to send it. */
	std::map<int, HelpText> expects[2];
	/** The file each source was read from, where there was one. */
	std::string files[HELP_SOURCES];
	/** Files that were there and would not parse, said on the card. */
	std::vector<std::string> problems;
};

/** The help for a module, read on first asking and kept until helpReload. */
const HelpModuleData& helpData(const std::string& plugin, const std::string& model);
/** Forgets everything read, so an edited file is read again on the next card. */
void helpReload();
/** The database's own text for a module, without the maker's or a user's laid over it. */
HelpModuleData helpDatabaseOnly(const std::string& plugin, const std::string& model);

/** Where each source's file for a module is, whether or not it exists. */
std::string helpMakerFile(const std::string& plugin, const std::string& model);
std::string helpUserFile(const std::string& plugin, const std::string& model);
std::string helpUserFolder(const std::string& plugin);
std::string helpDatabaseFile(const std::string& plugin, const std::string& model);

/** Writes a starting help file for every module of a plugin into a folder — see the export in
design/help-database.md. Existing files are left alone. Returns a sentence saying what was done. */
std::string helpExport(plugin::Plugin* p, const std::string& folder);

/** THE GESTURE THAT ASKS FOR HELP, chosen in the module's menu; option-click by default. */
struct HelpGesture {
	int mods;
	int button;
	/** What the panel and the note call it, on this platform. */
	const char* name;
};
extern const HelpGesture HELP_GESTURES[];
extern const int HELP_GESTURE_COUNT;
int helpGestureIndex();
void helpSetGesture(int index);
const HelpGesture& helpGesture();

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
