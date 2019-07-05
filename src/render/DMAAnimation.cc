
#include "common.hh"
#include "DMAAnimation.hh"

void DMAAnimation::loadFromChunk(rw::DMorphAnimationChunk* chunk) {
	for (auto& target : chunk->targets) {
		targetChannels.emplace_back();
		auto& channel = targetChannels.back();

		channel.curFrame = 0;
		channel.curTime = 0;

		for (auto& frame : target.frames) {
			channel.frames.push_back(frame);
		}
	}
}

void DMAAnimation::reset() {
	for (auto& channel : targetChannels) {
		channel.curFrame = 0;
		channel.curTime = 0;
	}
}

void DMAAnimation::advanceTime(float delta) {
	for (auto& channel : targetChannels) {
		channel.curTime += delta;
		auto& frame = channel.frames[channel.curFrame];
		if (channel.curTime > frame.duration) {
			channel.curTime -= frame.duration;
			channel.curFrame = frame.nextId;
			//frame = channel.frames[channel.curFrame];
		}
	}
}

float DMAAnimation::getTargetValue(int target) {
	auto& channel = targetChannels[target];
	auto& frame = channel.frames[channel.curFrame];

	float factor = channel.curTime / frame.duration;
	return frame.startValue + factor * (frame.endValue - frame.startValue);
}

void DMAAnimation::dumpDebug() {
//	printf("---\n");
//	for (auto& channel : targetChannels) {
//		printf("frame(%d) time(%f)\n", channel.curFrame, channel.curTime);
//	}
}