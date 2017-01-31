#pragma once

template<class Task> struct ColIndex {
	using Key = Task;
	template<class TRow> Key operator()(const TRow& row) {
		return row.c<Task>();
	}
};
