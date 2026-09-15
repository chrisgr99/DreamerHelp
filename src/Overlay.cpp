/** The one gesture this plugin owns: Option-click anywhere in the rack.

WHY THIS IS NOT INSIDE THE RACK. Rack's ScrollWidget consumes Option-click before offering it
to its children, so that Option-drag can pan the view. A handler parented to the rack, or to a
module, therefore never sees the gesture — which is why this overlay is a child of the SCENE,
added last so that it is offered events ahead of anything in the rack and ahead of any open
menu. Everything else here follows from that one fact.
*/
#include "plugin.hpp"
#include "Help.hpp"


struct HelpOverlay : widget::Widget {

	/** Nothing is drawn. The overlay exists to be offered events before the rack is. */
	void draw(const DrawArgs& args) override {}

	void onButton(const ButtonEvent& e) override {
		// AN ORDINARY CLICK PUTS THE NOTE AWAY. It is an answer to a question rather than a
		// window, so getting on with anything dismisses it. A click ON the note never arrives
		// here: the note is a child of the scene added after this overlay, so it is offered the
		// click first and consumes it to read itself aloud.
		if (e.action == GLFW_PRESS && (e.mods & RACK_MOD_MASK) != GLFW_MOD_ALT)
			helpDismissNote();

		if (e.action != GLFW_PRESS || e.button != GLFW_MOUSE_BUTTON_LEFT
			|| (e.mods & RACK_MOD_MASK) != GLFW_MOD_ALT) {
			widget::Widget::onButton(e);
			return;
		}

		// THE RACK'S OWN COORDINATES, not the scene's. helpClickAt walks the rack's children to
		// find what was clicked, and the rack scrolls and zooms underneath the scene.
		const math::Vec rackPos = APP->scene->rack->getMousePos();
		if (helpClickAt(rackPos)) {
			e.consume(this);
			e.stopPropagating();
			return;
		}
		widget::Widget::onButton(e);
	}
};


widget::Widget* createHelpOverlay() {
	return new HelpOverlay;
}
