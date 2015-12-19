#pragma once

#include "Channel.h"
#include "RowProxy.h"

template<typename... TTransforms>
struct TransformHelper;
template<typename TTransform>
struct TransformHelper<TTransform>
{
    template<typename TIn>
    static auto doTransform(const TIn& in) -> decltype(TTransform::transform(in))
    {
        return TTransform::transform(in);
    }
    template<typename TOut>
    static auto doReverseTransform(const TOut& out) -> decltype(TTransform::reverseTransform(out))
    {
        return TTransform::reverseTransform(out);
    }
};
template<typename TTransform, typename... TTransforms>
struct TransformHelper<TTransform, TTransforms...>
{
    template<typename TIn>
    static auto doTransform(const TIn& in) -> decltype(TransformHelper<TTransforms...>::doTransform(TTransform::transform(in)))
    {
        return TransformHelper<TTransforms...>::doTransform(TTransform::transform(in));
    }
    template<typename TOut>
    static auto doReverseTransform(const TOut& out) -> decltype(TransformHelper<TTransforms...>::doReverseTransform(TTransform::reverseTransform(out)))
    {
        return TransformHelper<TTransforms...>::doReverseTransform(TTransform::reverseTransform(out));
    }
};

template<typename TRow, typename TIterator, typename... TTransforms>
struct TransformIterator
{
    using RowType = TRow;
    TransformIterator() = default;
    TransformIterator(TIterator current) : current(current) {}
    TransformIterator<TRow, TIterator, TTransforms...>& operator++()
    {
        ++current;
        return *this;
    }
    RowType operator*() const
    {
        return TransformHelper<TTransforms...>::doTransform(*current);
    }
    bool operator==(const TransformIterator<TRow, TIterator, TTransforms...>& other) const
    {
        return current == other.current;
    }
    bool operator!=(const TransformIterator<TRow, TIterator, TTransforms...>& other) const
    {
        return !operator==(other);
    }
private:
    TIterator current;
};

template<typename TChannel, typename... TTransforms>
struct TransformStream : Channel
{
    using RowType = decltype(TransformHelper<TTransforms...>::doTransform(TChannel::RowType()));
    using IndexType = typename TChannel::IndexType;
    using iterator = TransformIterator<RowType, typename TChannel::const_iterator, TTransforms...>;

    TransformStream(TChannel& chan) : chan(chan) {}

	iterator begin() const
    {
        return iterator(chan.begin());
    }
	iterator end() const
    {
        return iterator(chan.end());
    }
    template<typename TRow2>
    pair<iterator, iterator> equalRange(const TRow2& row) const
    {
        auto range = chan.equalRange(TransformHelper<TTransforms...>::doReverseTransform(row));
        return std::make_pair(iterator(range.first), iterator(range.second));
    }
private:
    TChannel& chan;
};

template<typename TRow, typename TFrom, typename TTo, typename TResult>
struct ReplaceCol;
template<typename TFrom, typename TTo, typename... TSkipped>
struct ReplaceCol<Row<>, TFrom, TTo, Row<TSkipped...>>
{
	using type = Row<TSkipped...>;
};
template<typename TCol, typename... TCols, typename TFrom, typename TTo, typename... TSkipped>
struct ReplaceCol<Row<TCol, TCols...>, TFrom, TTo, Row<TSkipped...>>
{
	using type = typename std::conditional<std::is_same<TCol, TFrom>::value, Row<TSkipped..., TTo, TCols...>, typename ReplaceCol<Row<TCols...>, TFrom, TTo, Row<TSkipped..., TCol>>::type>::type;
};

template<typename TFrom, typename TTo>
struct RenameHelper
{
    template<typename TIn>
    static typename RowProxy<typename ReplaceCol<typename TIn::RowType, TFrom, TTo, Row<>>::type, typename ReplaceCol<typename TIn::ConstsType, TFrom, TTo, Row<>>::type> doRename(TIn& in)
    {
		using RenamedProxyType = RowProxy<ReplaceCol<TIn::RowType, TFrom, TTo, Row<>>::type, ReplaceCol<TIn::ConstsType, TFrom, TTo, Row<>>::type>;
		return RenamedProxyType::ConstructFromTwo(RenamedProxyType::template ColProxyType<TTo>(static_cast<TFrom&>(in)), in);
    }
};

template<typename TFrom, typename TTo>
struct Rename
{
    template<typename TIn>
    static auto transform(TIn& in) -> decltype(RenameHelper<TFrom, TTo>::doRename(in))
    {
        return RenameHelper<TFrom, TTo>::doRename(in);
    }
    template<typename TOut>
    static auto reverseTransform(TOut& out) -> decltype(RenameHelper<TTo, TFrom>::doRename(out))
    {
        return RenameHelper<TTo, TFrom>::doRename(out);
    }
};