#pragma once

template<class T> struct ColIndex {
	using Key = T;
	template<class TRow> Key operator()(const TRow& row) {
		return row.c<T>();
	}
};
