#pragma once

#include "Channel.h"
#include "Join.h"
#include "Amend.h"
#include "Subtract.h"
#include "Transform.h"

struct ProcessHost;

template<typename TQuery>
struct QueryHolder : Channel
{
	using RowType = typename TQuery::RowType;
	using IndexType = typename TQuery::IndexType;
	using iterator = typename TQuery::iterator;

	QueryHolder(TQuery& query, vector<unique_ptr<Channel>> streams) : query_(query), streams(std::move(streams)) {}
	QueryHolder(QueryHolder&& other) : query_(other.query), streams(std::move(other.streams)) {}
	QueryHolder& operator=(QueryHolder&& other)
	{
		if (this != &other) {
			query_ = other.query_;
			streams = std::move(other.streams);
		}
		return *this;
	}

	iterator begin() const
	{
		return query_.begin();
	}
	iterator end() const
	{
		return query_.end();
	}
	template<typename TRow2>
	pair<iterator, iterator> equalRange(TRow2&& row) const
	{
		return query_.equalRange(std::forward(row));
	}
	TQuery& query() { return query_; }
private:
	TQuery& query_;
	vector<unique_ptr<Channel>> streams;
};


#ifndef NDEBUG
#define CHECK_AND_INVALIDATE do { assert(is_valid); is_valid = false; } while(false)
#else
#define CHECK_AND_INVALIDATE do {} while(false)
#endif
template<typename TChannel>
struct JoinBuilder
{
	JoinBuilder(TChannel& chan) : chan(chan) {}
	JoinBuilder(JoinBuilder&& other) : chan(other.chan), streams(std::move(other.streams)) {}
	JoinBuilder& operator=(JoinBuilder&& other)
	{
		if (this != &other) {
			chan = other.chan;
			streams = std::move(other.streams);
		}
		return *this;
	}

    template<typename TRight>
    JoinBuilder<JoinStream<TChannel, TRight>> join(TRight& right)
    {
		CHECK_AND_INVALIDATE;
		return make<JoinStream<TChannel, TRight>>(chan, right);
    }
    template<typename TRight>
    JoinBuilder<AmendStream<TChannel, TRight>> amend(TRight& right)
    {
		CHECK_AND_INVALIDATE;
		return make<AmendStream<TChannel, TRight>>(chan, right);
    }
    template<typename TRight>
    JoinBuilder<SubtractStream<TChannel, TRight>> subtract(TRight& right)
    {
		CHECK_AND_INVALIDATE;
		return make<SubtractStream<TChannel, TRight>>(chan, right);
    }
	template<typename... TTransforms>
	JoinBuilder<TransformStream<TChannel, TTransforms...>> map()
	{
		CHECK_AND_INVALIDATE;
		return make<TransformStream<TChannel, TTransforms...>>(chan);
	}
	QueryHolder<TChannel> select()
    {
		CHECK_AND_INVALIDATE;
		return QueryHolder<TChannel>{ chan, std::move(streams) };
    }
private:
	JoinBuilder(TChannel& chan, vector<unique_ptr<Channel>> streams) : chan(chan), streams(std::move(streams)) {}

	template<typename TStream, typename... TArgs>
	JoinBuilder<TStream> make(TArgs&&... args)
	{
		auto stream = std::make_unique<TStream>(std::forward<TArgs>(args)...);
		auto& ref = *stream;
		streams.push_back(std::move(stream));
		return JoinBuilder<TStream>(ref, streams);
	}

#ifndef NDEGUG
	bool is_valid;
#endif
    TChannel& chan;
	vector<unique_ptr<Channel>> streams;
};

struct QueryFactory
{
    template<typename TChannel>
    JoinBuilder<TChannel> from(TChannel& chan)
    {
        return JoinBuilder<TChannel>(chan, *this);
    }
};