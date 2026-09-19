/** The one gesture this plugin owns: Option-click anywhere in the rack, or whichever other
combination is chosen in the Help module's menu — see HELP_GESTURES.

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

	/** Where an Option-press landed, and whether it was ours to answer.

	A CLICK AND A DRAG BEGIN IDENTICALLY, and Rack pans the rack on Option-drag — anywhere,
	including over a module. Consuming the press to answer a question therefore stopped panning
	dead for anyone with this plugin loaded: the drag could never begin, because the press that
	would have started it never reached the rack. Reported from the forum.

	So nothing is consumed on the way down. The press is noted, the event goes on to do whatever
	it would have done, and the question is answered on release — but only if the pointer stayed
	still, which is what separates a click from a drag. A pan that moved is a pan; a press that
	did not move is a question. */
	math::Vec pressedAt;
	bool pressedForHelp = false;

	/** How far the pointer may wander and still count as a click. A few pixels, because a hand
	holding a modifier is not a steady hand, and because a pan of two pixels is not a pan. */
	static constexpr float SLOP = 3.f;

	void onButton(const ButtonEvent& e) override {
		// AN ORDINARY CLICK PUTS THE NOTE AWAY. It is an answer to a question rather than a
		// window, so getting on with anything dismisses it. A click ON the note never arrives
		// here: the note is a child of the scene added after this overlay, so it is offered the
		// click first and consumes it to read itself aloud.
		//
		// NOT CONSUMED. Dismissing is a side effect of the click, not an answer to it.
		const HelpGesture& g = helpGesture();
		const bool asking = e.button == g.button && (e.mods & RACK_MOD_MASK) == g.mods;
		if (e.action == GLFW_PRESS && !asking)
			helpDismissNote();

		if (e.action == GLFW_PRESS && asking) {
			pressedAt = APP->scene->getMousePos();
			pressedForHelp = true;
			// Falls through: the rack must see this press, or it can never start a drag.
		}
		else if (e.action == GLFW_RELEASE && pressedForHelp) {
			pressedForHelp = false;
			if (APP->scene->getMousePos().minus(pressedAt).norm() <= SLOP) {
				// THE RACK'S OWN COORDINATES, not the scene's. helpClickAt walks the rack's
				// children, and the rack scrolls and zooms underneath the scene.
				if (helpClickAt(APP->scene->rack->getMousePos())) {
					e.consume(this);
					e.stopPropagating();
					return;
				}
			}
		}
		else if (e.action == GLFW_PRESS) {
			// Any other press abandons a pending one, so a middle-click mid-gesture cannot
			// leave the release below thinking it has a question to answer.
			pressedForHelp = false;
		}

		widget::Widget::onButton(e);
	}
};


widget::Widget* createHelpOverlay() {
	return new HelpOverlay;
}
