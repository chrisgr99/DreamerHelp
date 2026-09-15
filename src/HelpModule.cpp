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
	enum ParamId { P_ON, PARAMS_LEN };
	enum LightId { L_ON, LIGHTS_LEN };

	/** Read aloud as well as shown. Saved with the patch. */
	bool speak = false;

	Help() {
		config(PARAMS_LEN, 0, 0, LIGHTS_LEN);
		configSwitch(P_ON, 0.f, 1.f, 1.f, "Help mode", {"Off", "On"});
	}

	void process(const ProcessArgs& args) override {
		const bool on = params[P_ON].getValue() > 0.5f;
		lights[L_ON].setBrightness(on ? 1.f : 0.f);
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "speak", json_boolean(speak));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		if (json_t* j = json_object_get(rootJ, "speak"))
			speak = json_is_true(j);
	}
};


struct HelpWidget : ModuleWidget {
	HelpWidget(Help* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Help.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(
			Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<CKSS>(mm2px(Vec(7.62, 60.0)), module, Help::P_ON));
		addChild(createLightCentered<MediumLight<GreenLight>>(
			mm2px(Vec(7.62, 72.0)), module, Help::L_ON));
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

		nvgText(args.vg, mid, mm2px(53.f), "on", NULL);

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
		helpSetSpeak(m->speak);
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

	void appendContextMenu(Menu* menu) override {
		Help* m = dynamic_cast<Help*>(module);
		if (!m)
			return;
		menu->addChild(new MenuSeparator);
#if defined ARCH_MAC
		menu->addChild(createBoolPtrMenuItem("Read the help aloud", "", &m->speak));
#endif
	}
};


Model* modelHelp = createModel<Help, HelpWidget>("Help");
