#ifndef DBSI_PATTERN_UTILS_H
#define DBSI_PATTERN_UTILS_H


#include "dbsi_types.h"


namespace dbsi
{


bool pattern_matches(const Term& t, const Resource& r);
bool pattern_matches(const CodedTerm& t, const CodedResource& r);
bool pattern_matches(const TriplePattern& pat, const Triple& t);
bool pattern_matches(const CodedTriplePattern& pat, const CodedTriple& t);


}  // namespace dbsi


#endif  // DBSI_PATTERN_UTILS_H
