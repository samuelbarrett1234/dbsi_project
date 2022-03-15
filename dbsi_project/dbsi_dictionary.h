#ifndef DBSI_DICTIONARY_H
#define DBSI_DICTIONARY_H


#include "dbsi_types.h"


namespace dbsi
{


/*
* This class encodes/decodes resources to/from integers,
* to save memory.
* Resources are assigned new integer codes as they are
* encountered.
*/
class Dictionary
{
public:
	CodedResource encode(const Resource& r);
	Resource decode(CodedResource i) const;
};


}  // namespace dbsi


#endif  // DBSI_DICTIONARY_H
