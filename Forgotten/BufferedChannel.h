#pragma once

template<typename TBuffer, size_t SIZE>
struct BufferedChannel {
	BufferedChannel() {
		write_to = buffers.begin();
		tick();
	}
	void tick() {
		read_from = write_to;
		++write_to;
		write_to = write_to == buffers.end() ? buffers.begin() : write_to;
		write_to->clear();
	}
	const TBuffer &readFrom() {
		return *read_from;
	}
	TBuffer &writeTo() {
		return *write_to;
	}
private:
	std::array<TBuffer, SIZE> buffers;
	typename std::array<TBuffer, SIZE>::const_iterator read_from;
	typename std::array<TBuffer, SIZE>::iterator write_to;
};