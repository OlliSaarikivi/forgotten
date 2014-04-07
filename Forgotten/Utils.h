#pragma once

template<typename FwdIt, typename T, typename P>
FwdIt binarySearch(FwdIt begin, FwdIt end, T const &val, P pred) {
	FwdIt result = end;
	while (begin != end) {
		FwdIt middle(begin);
		std::advance(middle, std::distance(begin, end) >> 1);
		FwdIt middle_plus_one(middle);
		++middle_plus_one;
		auto &middle_value = *middle;
		bool const gt_middle = pred(middle_value, val);
		bool const lt_middle = pred(val, middle_value);
		begin = gt_middle ? middle_plus_one : begin;
		end = gt_middle ? end : (lt_middle ? middle : begin);
		result = !gt_middle && !lt_middle ? middle : result;
	}
	return result;
}