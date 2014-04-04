#pragma once

template<typename TBuffer>
struct TransientChannel {
	void tick() {
		buffer.clear();
	}
	const TBuffer &readFrom() {
		return buffer;
	}
	TBuffer &writeTo() {
		return buffer;
	}
private:
	TBuffer buffer;
};