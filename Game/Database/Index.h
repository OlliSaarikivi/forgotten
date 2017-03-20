#pragma once

template<class TCol> struct ColIndex {
	using Key = TCol;
	template<class TRow> Key operator()(const TRow& row) {
		return row.c<TCol>();
	}
};
