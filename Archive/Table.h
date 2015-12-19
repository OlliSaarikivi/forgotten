#pragma once

#include "Channel.h"
#include "RowProxy.h"

template<typename T>
struct FauxPointer
{
	FauxPointer(const T& value) : value(value) {}
	T& operator*() const
	{
		return value;
	}
	T* operator->() const
	{
		return &value;
	}
private:
	T value;
};

template<typename TIterator, typename TProxy>
struct ProxyIterator
{
	ProxyIterator() {}
	ProxyIterator(TIterator iter) : iter(iter) {}
	ProxyIterator(const ProxyIterator& other) : iter(other.iter) {}
	operator TIterator() const { return iter; }
	ProxyIterator& operator=(const ProxyIterator& other)
	{
		iter = other.iter;
		return *this;
	}
	ProxyIterator& operator++()
	{
		++iter;
		return *this;
	}
	ProxyIterator operator++(int)
	{
		++iter;
		return *this;
	}
	TProxy operator*() const
	{
		static_assert(std::is_reference<decltype(*iter)>::value, "Can only proxy iterators that return references");
		return TProxy::ConstructFromOne(const_cast<std::remove_const<TIterator::reference>::type>(*iter));
	}
	FauxPointer<TProxy> operator->() const
	{
		return FauxPointer<TProxy>{this->operator*()}
	}
	bool operator==(const ProxyIterator& other) const
	{
		return iter == other.iter;
	}
	bool operator!=(const ProxyIterator& other) const
	{
		return !operator==(other);
	}
private:
	TIterator iter;
};

template<typename TRow, typename TIndex = None>
struct Table : Channel
{
	using IndexType = TIndex;
	using RowType = typename ConcatRows<typename IndexType::KeyType::AsRow, TRow>::type;
	using ContainerType = typename TIndex::template ContainerType<RowType>;
	using ProxyType = RowProxy<RowType, typename IndexType::KeyType::AsRow>;
	using iterator = ProxyIterator<typename ContainerType::iterator, ProxyType>;

	iterator begin() const
	{
		return iterator{ container.begin() };
	}
	iterator end() const
	{
		return iterator{ container.end() };
	}
	template<typename TRow2>
	pair<iterator, iterator> equalRange(TRow2&& row) const
	{
		auto range = container.equal_range(MakeKeyRow<typename IndexType::KeyType, RowType>(std::forward<TRow2>(row)));
		return pair<iterator, iterator>{ iterator{ range.first }, iterator{ range.second } };
	}

	void clear()
	{
		container.clear();
	}
	void put(const RowType& row)
	{
		PutHelper<RowType, ContainerType>
			::tPut(container, row);
	}
	template<typename TRow2>
	typename ContainerType::size_type erase(const TRow2& row)
	{
		return EraseHelper<RowType, ContainerType>
			::tErase(container, MakeKeyRow<typename IndexType::KeyType, RowType>(row));
	}
	iterator erase(iterator first, iterator last)
	{
		return iterator{ container.erase(first, last) };
	}
	iterator erase(iterator position)
	{
		return iterator{ container.erase(position) };
	}
	void copyRowsFrom(const Table<TRow, TIndex>& other)
	{
		container = other.container;
	}
private:
	ContainerType container;
};

template<typename TRow, typename TContainer>
struct PutHelper
{
	template<typename TRow2>
	static void tPut(TContainer& buffer, TRow2&& row)
	{
		auto result = buffer.emplace(row);
		if (!result.second) {
			/* This is safe because result.first is guaranteed to be equal (in the ordering)
			* to the row to insert. */
			const_cast<TRow&>(*(result.first)) = std::forward<TRow2>(row);
		}
	}
};
template<typename TRow, typename TComparison>
struct PutHelper<TRow, flat_multiset<TRow, TComparison>>
{
	template<typename TRow2>
	static void tPut(flat_multiset<TRow, TComparison>& buffer, TRow2&& row)
	{
		buffer.emplace(std::forward<TRow2>(row));
	}
};
template<typename TRow>
struct PutHelper<TRow, vector<TRow>>
{
	template<typename TRow2>
	static void tPut(vector<TRow>& buffer, TRow2&& row)
	{
		buffer.emplace_back(std::forward<TRow2>(row));
	}
};

template<typename TRow, typename TContainer>
struct EraseHelper
{
	static typename TContainer::size_type tErase(TContainer& buffer, const TRow& row)
	{
		return buffer.erase(row);
	}
};
template<typename TRow>
struct EraseHelper<TRow, vector<TRow>>
{
	template<typename TRow2>
	static typename vector<TRow>::size_type tErase(vector<TRow>& buffer, TRow2&& row)
	{
		auto original_size = buffer.size();
		buffer.erase(std::remove_if(buffer.begin(), buffer.end(), [&](const TRow& other) {
			return row == other;
		}), buffer.end());
		return original_size - buffer.size();
	}
};