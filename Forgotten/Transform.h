#pragma once

#include "Channel.h"

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
    using const_iterator = TransformIterator<RowType, typename TChannel::const_iterator, TTransforms...>;

    TransformStream(const TChannel& chan) : chan(chan) {}

    virtual void forEachProducer(function<void(Process&)> f) const override
    {
        chan.forEachProducer(f);
    }
    const_iterator begin() const
    {
        return const_iterator(chan.begin());
    }
    const_iterator end() const
    {
        return const_iterator(chan.end());
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(const TRow2& row) const
    {
        auto range = chan.equalRange(TransformHelper<TTransforms...>::doReverseTransform(row));
        return std::make_pair(const_iterator(range.first), const_iterator(range.second));
    }
private:
    const TChannel& chan;
};

template<typename TFrom, typename TTo, bool HasFrom>
struct RenameHelper;
template<typename TFrom, typename TTo>
struct RenameHelper<TFrom, TTo, true>
{
    template<typename TIn>
    using OutType = typename ConcatRows<typename RemoveExisting<TIn, TFrom, Row<>>::type, Row<TTo>>::type;
    template<typename TIn>
    static OutType<TIn> doRename(const TIn& in)
    {
        Row<TTo> renamed;
        static_cast<TTo&>(renamed).set(static_cast<const TFrom&>(in));
        auto result = OutType<TIn>(in, renamed);
        return result;
    }
};
template<typename TFrom, typename TTo>
struct RenameHelper<TFrom, TTo, false>
{
    template<typename TIn>
    static TIn doRename(const TIn& in)
    {
        return in;
    }
};

template<typename TFrom, typename TTo>
struct Rename
{
    template<typename TIn>
    static auto transform(const TIn& in) -> decltype(RenameHelper<TFrom, TTo, std::is_base_of<TFrom, TIn>::value>::doRename(in))
    {
        return RenameHelper<TFrom, TTo, std::is_base_of<TFrom, TIn>::value>::doRename(in);
    }
    template<typename TOut>
    static auto reverseTransform(const TOut& out) -> decltype(RenameHelper<TTo, TFrom, std::is_base_of<TTo, TOut>::value>::doRename(out))
    {
        return RenameHelper<TTo, TFrom, std::is_base_of<TTo, TOut>::value>::doRename(out);
    }
};