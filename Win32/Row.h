#pragma once

template<class T, class TRow> T& col(TRow row) { return row.col<T>(); }

template<class... TValues> class Row {
	tuple<TValues&...> refs;
public:
	Row(TValues&... values) : refs(values...) {}

	template<class T>
	operator const T&() const { return col<T>(); }
	template<class T>
	operator T&() { return col<T>(); }
	template<class T>
	Row& operator |=(T x) { col<T>() = x; return *this; }

	template<class T> T& col() const {
		return get<T&>(refs);
	}
};
