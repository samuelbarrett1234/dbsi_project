#ifndef DBSI_ITERATOR_H
#define DBSI_ITERATOR_H


#include "dbsi_types.h"
#include <optional>


namespace dbsi
{


/*
* This is an interface to an object which iterates over values
* of a generic type T.
* These iterators are initialised to be invalid.
* You can call `current` and `next` if and only if the iterator is `valid`.
* `start` can be called at any time to validate the iterator and restart it.
* Calling `next` will result in invalidating the iterator once the end is reached.
* These iterators may apply selection conditions, for example only returning
* triples in the database with a given predicate.
*/
template<typename T>
class IIterator
{
public:
	virtual ~IIterator() = default;

	virtual void start() = 0;  // post: `valid()`, and points to first tuple
	virtual T current() const = 0;  // pre: `valid()`
	virtual void next() = 0;  // pre: `valid()`
	virtual bool valid() const = 0;
};


/*
* These iterators usually, but do not necessarily, guarantee an order to their
* output. This can be queried with `order`.
*/
class IOrderedTripleIterator
{
public:
	virtual ~IOrderedTripleIterator() = default;
	virtual std::optional<TripleOrder> order() const = 0;
};


typedef IIterator<VarMap> IVarMapIterator;
typedef IIterator<Triple> ITripleIterator;
typedef IIterator<CodedTriple> ICodedTripleIterator;


}  // namespace dbsi


#endif  // DBSI_ITERATOR_H
