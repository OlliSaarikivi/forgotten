#pragma once

#include "Row.h"
#include "Utils.h"

struct NoRow {};
static const NoRow none;

template<class TRow> class MaybeRow {
	union Holder {
		TRow row;

		Holder() {}
	} holder;
	bool isSome;

public:
	MaybeRow() : isSome(false) {}
	MaybeRow(const TRow& row) : isSome(true) {
		new (&holder.row) TRow(row);
	}
	MaybeRow(NoRow) : isSome(false) {}
	MaybeRow(const MaybeRow& other) : isSome(other.isSome) {
		if (isSome) new (&holder.row) TRow(other.holder.row);
	}

	operator bool() const {
		return isSome;
	}

	TRow operator*() {
		return holder.row;
	}
	FauxPointer<TRow> operator->() const {
		return FauxPointer<TRow>{ this->operator*() };
	}
};

template<class... TValues, class TCol> auto& operator>>(MaybeRow<Row<TValues...>> someRow, const TCol& name) {
	return (*someRow) >> name;
}
