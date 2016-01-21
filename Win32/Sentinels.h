#pragma once

struct End {};

template<class TIter> bool isAt(const TIter& iter, const TIter& sentinel) {
	return iter == sentinel;
}
