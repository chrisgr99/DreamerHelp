/** The module itself: no inputs, no outputs, one switch.

WHY THERE IS A SWITCH AT ALL, when the module could simply answer while it is in the patch.
Option-click is not ours. Other plugins use it, and Rack uses Option-drag to pan, so a mode that
silently claims the gesture for the whole rack is a mode that breaks somebody else's module with
no way to tell what did it. The switch is how you give it back.

The module is otherwise inert. It reads nothing, writes nothing, and takes no part in the audio
thread beyond a parameter read.
*/
#include "plugin.hpp"
#include "Help.hpp"


/** How many Help modules are in the patch, so the overlay goes in when the first one arrives
and comes out when the last one leaves. Counted on the widgets rather than the modules, because
the module browser builds a preview module with no widget on screen and that must not install
anything. */
static int gHelpCount = 0;
static WeakPtr<widget::Widget> gOverlay;


struct Help : Module {
	// APPENDED, NEVER INSERTED. Rack saves a parameter by its number, so a new one in the middle
	// would move every one after it and load somebody's saved patch wrong.
	enum ParamId { P_ON, P_SPEAK, PARAMS_LEN };
	enum LightId { L_ON, L_SPEAK, LIGHTS_LEN };

	Help() {
		config(PARAMS_LEN, 0, 0, LIGHTS_LEN);
		// LATCHING BUTTONS RATHER THAN LEVERS. Both are things to reach for while reading, and a
		// lit button says what is on from across the rack in a way a lever's position does not.
		configSwitch(P_ON, 0.f, 1.f, 1.f, "Help mode", {"Off", "On"});
		// ON THE PANEL RATHER THAN THE MENU, because it is a thing to reach for while reading
		// rather than a setting made once. It was a menu item to begin with, and Mac-only.
		configSwitch(P_SPEAK, 0.f, 1.f, 0.f, "Speak help on click", {"Off", "On"});
	}

	void process(const ProcessArgs& args) override {
		const bool on = params[P_ON].getValue() > 0.5f;
		// SPEECH CANNOT OUTLIVE HELP. There is nothing for it to read once the gesture is given
		// back, and a lit button beside a dark one would be describing a state that cannot
		// happen. Switching help off switches speech off with it; switching help back on leaves
		// speech off, because turning a voice on is a decision somebody makes deliberately.
		if (!on && params[P_SPEAK].getValue() > 0.5f)
			params[P_SPEAK].setValue(0.f);

		lights[L_ON].setBrightness(on ? 1.f : 0.f);
		lights[L_SPEAK].setBrightness(params[P_SPEAK].getValue() > 0.5f ? 1.f : 0.f);
	}

	/** Speech used to be a plain member saved here; it is a parameter now, which Rack saves
	itself. A patch written by the older build still carries the old key, so it is read once and
	turned into the parameter rather than being lost. */
	void dataFromJson(json_t* rootJ) override {
		if (json_t* j = json_object_get(rootJ, "speak"))
			params[P_SPEAK].setValue(json_is_true(j) ? 1.f : 0.f);
	}
};


/** Where the two buttons sit, and how far above one its label goes.

In pixels, which is what both the widgets and the lettering are placed in. Going through
millimetres for one and not the other is what put the labels on top of the buttons: two ways of
saying the same position agree only while the arithmetic is right. */
static const float HELP_BTN_Y = 312.f;
static const float SPEAK_BTN_Y = 354.f;
static const float LABEL_UP = 18.f;


struct HelpWidget : ModuleWidget {
	HelpWidget(Help* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Help.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(
			Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// VCVLightLatch holds its own state, so the button IS the setting and Rack saves it with
		// the patch — no separate flag to keep in step with what the panel shows.
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<GreenLight>>>(
			mm2px(Vec(7.62, 58.0)), module, Help::P_ON, Help::L_ON));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<GreenLight>>>(
			mm2px(Vec(7.62, 88.0)), module, Help::P_SPEAK, Help::L_SPEAK));
	}

	/** THE PANEL LETTERS ITSELF, because Rack draws panels with nanosvg and nanosvg ignores
	<text> entirely — a word in the SVG is simply not there. The alternative is converting the
	lettering to paths in a drawing program, which makes it uneditable by anyone who does not
	have that program and that font. */
	void draw(const DrawArgs& args) override {
		ModuleWidget::draw(args);
		std::shared_ptr<window::Font> font =
			APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
		// A HANDLE OF ZERO IS A VALID FONT. nanovg numbers fonts from zero and returns -1 for
		// failure, so testing the handle for truth throws away the first font loaded — which is
		// usually this one, and the panel then draws nothing at all with no error anywhere.
		if (!font || font->handle < 0)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		const float mid = box.size.x * 0.5f;

		nvgFontSize(args.vg, 13.f);
		nvgFillColor(args.vg, nvgRGB(0xcf, 0xd6, 0xdf));
		nvgTextLetterSpacing(args.vg, 1.2f);
		nvgText(args.vg, mid, mm2px(13.f), "HELP", NULL);
		nvgTextLetterSpacing(args.vg, 0.f);

		// WHAT TO CLICK, SAID PROPERLY. It read "option click anything", which was short enough
		// to fit and wrong: the panel between the controls answers nothing, and a reader who
		// tried there and got silence would conclude the module was broken rather than that
		// they had aimed at the one part with nothing to say.
		//
		// Three HP is about nine characters a line at this size, so the lines are short because
		// they have to be, not for effect.
		// WHITE, like the labels. It was grey while it read "option click anything" and was
		// scenery; now that it says which three things answer, it is the instruction and wants
		// reading. Four HP is about twelve characters a line at this size.
		nvgFontSize(args.vg, 8.f);
		nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		const char* said[] = {"option click", "the module", "title or any",
			"control or", "port for help"};
		for (int i = 0; i < 5; i++)
			nvgText(args.vg, mid, 78.f + i * 12.4f, said[i], NULL);

		// EACH LABEL IS PLACED OFF ITS OWN BUTTON, not off the panel. A label positioned
		// independently drifts away from the thing it names the moment either one moves, which
		// is how these came to be sitting on top of their buttons.
		nvgText(args.vg, mid, HELP_BTN_Y - LABEL_UP, "help", NULL);
		nvgText(args.vg, mid, SPEAK_BTN_Y - LABEL_UP - 10.f, "speak", NULL);
		nvgText(args.vg, mid, SPEAK_BTN_Y - LABEL_UP, "on click", NULL);

		nvgFontSize(args.vg, 7.f);
		nvgFillColor(args.vg, nvgRGB(0x5f, 0x9d, 0xd8));
		nvgText(args.vg, mid, 152.f, "Dreamer", NULL);

		// THE GREEN BORDER THE OTHER PLUGIN'S PANELS WEAR, in the same colour and the same
		// inset, so a rack holding both looks like one maker's work. Stroked last so nothing
		// drawn above can sit on top of it.
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 3.f, 3.f, box.size.x - 6.f, box.size.y - 6.f, 6.f);
		nvgStrokeColor(args.vg, nvgRGB(0x3d, 0xe0, 0x7a));
		nvgStrokeWidth(args.vg, 1.2f);
		nvgStroke(args.vg);
	}

	/** NOTHING IS INSTALLED WITHOUT A MODULE. The browser builds a preview widget whose module
	is null; installing a scene overlay from there would put the gesture into a rack the user has
	not asked anything of. */
	void step() override {
		ModuleWidget::step();
		if (!module)
			return;
		Help* m = dynamic_cast<Help*>(module);
		if (!m)
			return;
		helpStep(m->params[Help::P_ON].getValue() > 0.5f);
		helpSetSpeak(m->params[Help::P_SPEAK].getValue() > 0.5f);
	}

	void onAdd(const AddEvent& e) override {
		ModuleWidget::onAdd(e);
		if (!module)
			return;
		if (++gHelpCount == 1 && !gOverlay) {
			widget::Widget* o = createHelpOverlay();
			// Added to the scene, and last, so it is offered events before the rack and before
			// any open menu. See plugin.hpp.
			APP->scene->addChild(o);
			gOverlay = o;
		}
	}

	void onRemove(const RemoveEvent& e) override {
		if (module && --gHelpCount <= 0) {
			gHelpCount = 0;
			helpRemoveAll();
			// Ask the widget's OWN parent rather than assuming which it is, so this is safe
			// whether one module is being deleted from a live patch or the whole tree is coming
			// down around it.
			if (widget::Widget* o = gOverlay) {
				if (o->parent)
					o->parent->removeChild(o);
				delete o;
			}
			gOverlay = NULL;
		}
		ModuleWidget::onRemove(e);
	}

	// The speech switch is on the panel now, so there is nothing left for a menu to carry.
};


Model* modelHelp = createModel<Help, HelpWidget>("Help");
