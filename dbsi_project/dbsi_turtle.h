#ifndef DBSI_TURTLE_H
#define DBSI_TURTLE_H


#include <memory>
#include <istream>
#include "dbsi_iterator.h"


namespace dbsi
{


/*
* Creates a Turtle file parser, for the given input file stream.
* The stream must be kept alive for the entire lifetime of this
* iterator, and is not managed by this iterator. However, nobody
* else should use it while the iterator is using it.
* 
* Behaviour on errors:
* If the file is detected to be corrupted, parsing will stop
* immediately, and the iterator will become invalid in a "controlled"
* way. As a user of the iterator, this will be indistinguishable
* from seeing an EOF. Restarting an iterator that has stopped
* due to an error is also perfectly acceptable.
*/
std::unique_ptr<ITripleIterator> create_turtle_file_parser(std::istream& in);


}  // namespace dbsi


#endif  // DBSI_TURTLE_H
