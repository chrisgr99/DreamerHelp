# Help files, for makers and users

Dreamer Help shows a note for any control, jack, light or module title in VCV Rack. The text comes from JSON help files, one per module. This page is for anyone writing one: a maker describing their own modules, or a user filling in help for a module whose maker has not.

## Where Help looks

For every item on a module, Help uses the first of these that has text for it:

1. **The maker's file**, in the `help` folder of the maker's installed plugin: `<plugin folder>/help/<ModuleSlug>.json`.
2. **A user's file**, in the Rack user folder: `<Rack user folder>/DreamerHelp/help/<PluginSlug>/<ModuleSlug>.json`.
3. **Dreamer Help's own database**, which ships inside Dreamer Help.

A file can describe part of a module; the rest comes from the next place. Each note says whether its text came from the maker's help file or from yours.

## The file

```json
{
  "plugin": "MyPlugin",
  "module": "MyModule",
  "description": "What the module is, in one or two sentences.",
  "params": [
    {"ids": [0], "name": "Tempo", "text": "Sets the tempo when no clock is patched."},
    {"ids": [1, 2, 3, 4], "name": "Level", "text": "Sets that channel's level."}
  ],
  "inputs": [
    {"ids": [0], "name": "Clock", "text": "Advances one step on each rising edge."}
  ],
  "outputs": [
    {"ids": [0], "name": "Out", "text": "Carries the mix."}
  ],
  "lights": [
    {"ids": [0], "name": "Clip", "text": "Lights when the mix goes above 10V."}
  ],
  "notes": ["Something true of the whole module rather than of one control."],
  "menu": ["A setting in the module's right-click menu, and what it does."],
  "expects": {
    "inputs": {"0": "0 to 10V · stepped · polyphonic"}
  }
}
```

- **`plugin` and `module`** are the slugs in your `plugin.json`, and are the only required fields.
- **`description`** is shown when the module's title is clicked, followed by the `notes` as points and the `menu` settings under a heading saying where they are.
- **`params`, `inputs`, `outputs`, `lights`**: each entry names the controls it covers by their numbers in your module's lists, and gives their text. One entry can cover several controls, so a row of sixteen inputs that share a sentence is written once.
- **`ids` decide which control an entry belongs to.** `name` is for whoever reads the file; Help does not use it.
- **`expects`** (optional) is a short phrase shown under a jack's own text, saying what to send it or what comes out: range, stepped or continuous, polyphony.
- **An entry with empty text counts as not written**, so the next place's text shows for it.
- **A field named `database` is ignored.** An exported file uses it to show the current text beside each entry.

A file that will not parse is reported on the note, with the file, the line and the column.

## For makers

Put the files in a `help` folder at the top of your plugin's repository, one per module, and add it to your Makefile so it ships in your plugin package:

```make
DISTRIBUTABLES += help
```

Build and install your plugin as usual, then choose **Reload help files** from the Help module's right-click menu, and click a control to see your text. Edit in your repository, never in the installed plugin folder, which the library replaces on every update.

### Writing help in your source code

Instead of writing the JSON by hand, you can write the help in your source, directly after the line that configures each control, which is where its tooltip is defined. A tooltip is best kept short; the help is where the detail goes.

```cpp
configParam(TEMPO_PARAM, 30.f, 300.f, 120.f, "Tempo", " BPM");
//? Sets the tempo when no clock is patched, 30 to 300 beats per minute.
//? With a clock patched, shows the tempo measured from the clock.

for (int i = 0; i < 8; i++)
	configInput(STEP_INPUTS + i, string::f("Step %d", i + 1));
//? Replaces the step's voltage while a cable is patched.

//?module Plays a chord chart as an MPX stream and as pitch voltages.
//?note A new module has no chart in it and plays nothing until a song is chosen.
```

- **`//?`** lines directly after a `configParam`, `configSwitch`, `configButton`, `configInput`, `configOutput` or `configLight` call are that control's help. Plain comments and blank lines between them are allowed; any other code ends it.
- **`//?module`** is the module's description, and **`//?note`** starts a note. Further `//?` lines continue whichever came last. Put them directly above the module's `struct`, or inside its code.
- Lines are joined with a space, so wrap them to any width.
- A call inside a loop covers every control the loop configures: an `ENUMS` run plus a counter, or a name plus the loop's counter.

Then run [`tools/helpscan.py`](../tools/helpscan.py) from your plugin's folder:

```
python3 helpscan.py src/*.cpp
```

It writes `help/<ModuleSlug>.json` for each module that has help, reading the slugs from your `createModel` calls and the plugin slug from `plugin.json`. It uses only Python's standard library — the same Python the Rack SDK's `helper.py` needs. Anything it cannot place, it reports with the file and line instead of guessing. `--check` reports without writing, and `--out` writes somewhere other than `help`.

## For users

If a module's maker has not written help, you can. Your files live in the Rack user folder, so no update of either plugin touches them.

1. Put a module from the plugin in your rack.
2. In the Help module's right-click menu, choose **Export help files for** and the plugin. It offers your own help folder for that plugin first; accept it.
3. Open a module's file. Every control is listed with its name, an empty `text`, and the current text beside it under `database`. Write your own text in `text` for anything you can describe better, and leave the rest empty.
4. Choose **Reload help files**, and click a control to see your words.

To share what you have written, open the module's note by clicking its title, and click **Send this help to Dreamer Help**. Your file goes on the clipboard and a GitHub issue opens in your browser, ready for you to paste it. It is reviewed and folded into the database for everybody.

## Exporting a starting file

**Export help files for** writes one file per module of the chosen plugin into a folder you pick: your user help folder by default, or, for a maker, the `help` folder of your repository. Files already in the folder are never written over.
