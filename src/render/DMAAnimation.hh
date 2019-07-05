// Playback of DMA files
#pragma once

#include "common.hh"
#include "animation.hh"
#include <vector>

class DMAAnimation {
private:
	struct TargetChannel {
		int curFrame;
		float curTime;

		std::vector<rw::DMorphAnimationChunk::Frame> frames;
	};

	std::vector<TargetChannel> targetChannels;
public:
	void loadFromChunk(rw::DMorphAnimationChunk* chunk);
	void reset();
	void advanceTime(float delta);

	// returns the current strength of a given target
	float getTargetValue(int target);
	int getTargetCount() {return (int) targetChannels.size();}
	void dumpDebug();
};
