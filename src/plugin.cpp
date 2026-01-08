#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	// This is the line that was likely missing or mismatched!
	p->addModel(modelChordCircle);
}