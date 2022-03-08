#ifndef _BURST_H_
#define _BURST_H_

#include <cstdint>

template<typename ID_TYPE>
class Burst {
public:
	uint32_t start_window;
	uint32_t end_window;
	ID_TYPE id;

	Burst(){};
	Burst(uint32_t start, uint32_t end, ID_TYPE _id)
	{
		start_window = start;
		end_window = end;
		id = _id;
	}
};

#endif
