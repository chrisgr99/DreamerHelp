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
		configSwitch(P_ON, 0.f, 1.f, 1.f, "Help mode", {"Off", "On"});
		// ON THE PANEL RATHER THAN THE MENU, because it is a thing to reach for while reading
		// rather than a setting made once. It was a menu item to begin with, and Mac-only.
		configSwitch(P_SPEAK, 0.f, 1.f, 0.f, "Speak help on click", {"Off", "On"});
	}

	void process(const ProcessArgs& args) override {
		lights[L_ON].setBrightness(params[P_ON].getValue() > 0.5f ? 1.f : 0.f);
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


struct HelpWidget : ModuleWidget {
	HelpWidget(Help* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Help.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(
			Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CKSS>(mm2px(Vec(7.62, 56.0)), module, Help::P_ON));
		addChild(createLightCentered<MediumLight<GreenLight>>(
			mm2px(Vec(7.62, 66.0)), module, Help::L_ON));

		addParam(createParamCentered<CKSS>(mm2px(Vec(7.62, 86.0)), module, Help::P_SPEAK));
		addChild(createLightCentered<MediumLight<GreenLight>>(
			mm2px(Vec(7.62, 96.0)), module, Help::L_SPEAK));
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

		nvgFontSize(args.vg, 8.f);
		nvgFillColor(args.vg, nvgRGB(0x7f, 0x86, 0x92));
		const char* lines[] = {"option", "click", "anything"};
		for (int i = 0; i < 3; i++)
			nvgText(args.vg, mid, mm2px(27.f + i * 5.f), lines[i], NULL);

		nvgText(args.vg, mid, mm2px(49.f), "on", NULL);
		nvgText(args.vg, mid, mm2px(76.f), "speak", NULL);
		nvgText(args.vg, mid, mm2px(80.f), "on click", NULL);

		nvgFontSize(args.vg, 7.f);
		nvgFillColor(args.vg, nvgRGB(0x5f, 0x9d, 0xd8));
		nvgText(args.vg, mid, mm2px(118.f), "Dreamer", NULL);
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
