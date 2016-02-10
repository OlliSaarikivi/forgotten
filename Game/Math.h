#pragma once

#include "Row.h"

namespace impl {
	template<class TResult, class TRow, int Current> struct RowAtIndex;

	template<class TResult, int Current>
	struct RowAtIndex<TResult, Row<>, Current> {
		template<class TRow> TResult operator()(size_t i, TRow& row) {
			std::remove_reference_t<TResult>* result = nullptr;
			return *result;
		}
	};

	template<class TResult, class TValue1, class... TValues, int Current>
	struct RowAtIndex<TResult, Row<TValue1, TValues...>, Current> {
		template<class TRow> TResult operator()(size_t i, TRow& row) {
			if (i == Current)
				return static_cast<TResult>(row.unsafeCol<TValue1>());
			else
				return RowAtIndex<TResult, Row<TValues...>, Current + 1>()(i, row);
		}
	};
}

namespace cml {

template<class TRow, class... TCols>
struct VecCols {};

template<class Element, class TRow, class... TCols>
class CmlRowStorage
{
public:
	//typedef external<Size, -1> generator_type;

	typedef Element value_type;
	typedef Element* pointer;
	typedef Element& reference;
	typedef const Element& const_reference;
	typedef const Element* const_pointer;

	typedef value_type array_impl[sizeof...(TCols)];

	typedef external_memory_tag memory_tag;
	typedef fixed_size_tag size_tag;
	typedef not_resizable_tag resizing_tag;
	typedef oned_tag dimension_tag;

public:
	enum { array_size = sizeof...(TCols) };

public:
	CmlRowStorage(TRow row)
		: row(row) {}

public:
	size_t size() const { return size_t(array_size); }

	reference operator[](size_t i) {
		return impl::RowAtIndex<reference, Row<TCols...>, 0>()(i, row);
	}

	const_reference operator[](size_t i) const {
		return impl::RowAtIndex<const_reference, Row<TCols...>, 0>()(i, const_cast<TRow&>(row));
	}

	/** Return access to the data as a raw pointer. */
	//pointer data() { return m_data; }

	/** Return access to the data as a raw pointer. */
	//const_pointer data() const { return m_data; }

protected:
	TRow row;

private:
	/* Initialization without an argument isn't allowed: */
	CmlRowStorage();

private:
	CmlRowStorage& operator=(const CmlRowStorage&);
};

template<typename Element, class TRow, class... TCols>
class vector< Element, VecCols<TRow, TCols...> >
	: public CmlRowStorage<Element, TRow, TCols...>
{
public:
	enum { dimension = sizeof...(TCols) };

public:
	typedef VecCols<TRow> storage_type;
	typedef VecCols<TRow, TCols...> generator_type;
	typedef CmlRowStorage<Element, TRow, TCols...> array_type;
	typedef vector<Element, generator_type> vector_type;
	typedef Element coordinate_type;
	typedef vector_type expr_type;
	typedef vector<typename cml::remove_const<Element>::type,
		fixed<dimension> > temporary_type;
	/* Note: this ensures that an external vector is copied into the proper
	* temporary; external<> temporaries are not allowed.
	*/

	typedef typename temporary_type::subvector_type subvector_type;

	typedef typename array_type::value_type value_type;
	typedef typename array_type::reference reference;
	typedef typename array_type::const_reference const_reference;

	typedef vector_type& expr_reference;
	typedef const vector_type& expr_const_reference;

	typedef typename array_type::memory_tag memory_tag;
	typedef typename array_type::size_tag size_tag;
	typedef cml::et::vector_result_tag result_tag;
	typedef cml::et::assignable_tag assignable_tag;

public:
	/** Return square of the length. */
	value_type length_squared() const {
		return cml::dot(*this, *this);
	}

	/** Return the length. */
	value_type length() const {
		return std::sqrt(length_squared());
	}

	/** Normalize the vector. */
	vector_type& normalize() {
		return (*this /= length());
	}

	/** Set this vector to [0]. */
	vector_type& zero() {
		typedef cml::et::OpAssign<Element, Element> OpT;
		cml::et::UnrollAssignment<OpT>(*this, Element(0));
		return *this;
	}

	/** Set this vector to a cardinal vector. */
	vector_type& cardinal(size_t i) {
		zero();
		(*this)[i] = Element(1);
		return *this;
	}

	/** Pairwise minimum of this vector with another. */
	template<typename E, class AT>
	void minimize(const vector<E, AT>& v) {
		/* XXX This should probably use ScalarPromote: */
		for (size_t i = 0; i < this->size(); ++i) {
			(*this)[i] = std::min((*this)[i], v[i]);
		}
	}

	/** Pairwise maximum of this vector with another. */
	template<typename E, class AT>
	void maximize(const vector<E, AT>& v) {
		/* XXX This should probably use ScalarPromote: */
		for (size_t i = 0; i < this->size(); ++i) {
			(*this)[i] = std::max((*this)[i], v[i]);
		}
	}

	/** Fill vector with random elements. */
	void random(value_type min, value_type max) {
		for (size_t i = 0; i < this->size(); ++i) {
			(*this)[i] = cml::random_real(min, max);
		}
	}

	/** Return a subvector by removing element i.
	*
	* @internal This is horribly inefficient...
	*/
	subvector_type subvector(size_t i) const {
		subvector_type s;
		for (size_t m = 0, n = 0; m < this->size(); ++m)
			if (m != i) s[n++] = (*this)[m];
		return s;
	};

public:
	vector(TRow row) : array_type(row) {}

public:
	CML_ASSIGN_VEC_2
	CML_ASSIGN_VEC_3
	CML_ASSIGN_VEC_4

	CML_VEC_ASSIGN_FROM_VECTYPE

	/* Only assignment operators can be used to copy from other types: */
	CML_VEC_ASSIGN_FROM_VEC(=, cml::et::OpAssign)
	CML_VEC_ASSIGN_FROM_VEC(+=, cml::et::OpAddAssign)
	CML_VEC_ASSIGN_FROM_VEC(-=, cml::et::OpSubAssign)

	CML_VEC_ASSIGN_FROM_VECXPR(=, cml::et::OpAssign)
	CML_VEC_ASSIGN_FROM_VECXPR(+=, cml::et::OpAddAssign)
	CML_VEC_ASSIGN_FROM_VECXPR(-=, cml::et::OpSubAssign)

	CML_VEC_ASSIGN_FROM_SCALAR(*=, cml::et::OpMulAssign)
	CML_VEC_ASSIGN_FROM_SCALAR(/=, cml::et::OpDivAssign)
};

}