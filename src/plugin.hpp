#pragma once
#include <rack.hpp>

using namespace rack;

extern Plugin* pluginInstance;
extern Model* modelHelp;

/** The overlay that catches Option-click before the rack does.

It has to live on the SCENE rather than inside the rack, and that is not a preference. Rack's
ScrollWidget consumes Option-click ahead of its own children, because Option-drag pans the
rack — so a handler parented anywhere inside the rack never sees the gesture at all. Added to
the scene and kept as its last child, it is offered the event first.
*/
widget::Widget* createHelpOverlay();
