#pragma once

struct RenderSettings;
struct InputState;

namespace menu {
	void initRenderSettings(uint32_t drawableWidth, uint32_t drawableHeight, RenderSettings& renderSettings);
	
	void updateAndDraw(uint32_t drawableWidth, uint32_t drawableHeight, const InputState& is, bool& startGame, bool& quit);
}
