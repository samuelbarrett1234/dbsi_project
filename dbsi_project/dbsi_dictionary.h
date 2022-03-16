#ifndef DBSI_DICTIONARY_H
#define DBSI_DICTIONARY_H


#include <vector>
#include <string>
#include <unordered_map>
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

private:
	std::unordered_map<Resource, CodedResource> m_encoder;

	/*
	* Store pointers here rather than the `Resource`, to prevent
	* duplication of memory by a factor of 2.
	* (note that `std::unordered_map` guarantees that elements'
	* pointers are never invalidated, even though their iterators
	* may be)
	*/
	std::vector<const Resource*> m_decoder;
};


}  // namespace dbsi


#endif  // DBSI_DICTIONARY_H
