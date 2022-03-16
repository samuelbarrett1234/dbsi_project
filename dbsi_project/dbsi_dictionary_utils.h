#ifndef DBSI_DICTIONARY_UTILS_H
#define DBSI_DICTIONARY_UTILS_H


#include <memory>
#include "dbsi_types.h"
#include "dbsi_iterator.h"


namespace dbsi
{


class Dictionary;  // forward declaration


CodedTerm encode(Dictionary& dict, const Term& t);
Term decode(const Dictionary& dict, const CodedTerm& t);
CodedTriple encode(Dictionary& dict, const Triple& t);
Triple decode(const Dictionary& dict, const CodedTriple& t);
CodedTriplePattern encode(Dictionary& dict, const TriplePattern& t);
TriplePattern decode(const Dictionary& dict, const CodedTriplePattern& t);


/*
* Wrap an iterator with another iterator which automatically
* encodes its outputs. Subsumes management of the given input
* iterator. Warning: `dict` must remain alive for the entire
* duration of the returned iterator's lifetime.
*/
std::unique_ptr<ICodedTripleIterator> autoencode(
	Dictionary& dict, std::unique_ptr<ITripleIterator> iter);
std::unique_ptr<ITripleIterator> autodecode(
	const Dictionary& dict, std::unique_ptr<ICodedTripleIterator> iter);


}  // namespace dbsi


#endif  // DBSI_DICTIONARY_UTILS_H
