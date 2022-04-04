# Database Systems Implementation Project

Candidate Number 1032514

Hilary Term 2022

## Project Structure

Please find the formal report in the root of this repository, `REPORT.pdf`.

Here is a summary of what is in each file in the `dbsi_project/` folder:
- `dbsi_project.cpp` : The main application entrypoint. This is a good file to start with as it references the high-level interfaces of many other areas of the project.
- `dbsi_types.h`, `dbsi_types.cpp` : definitions of types used throughout the project. Makes extensive use of C++17's `std::variant`.
- `dbsi_assert.h` : definitions of different types of assertions, used throughout all function implementations to check, and document, correctness. These can be enabled or disabled in this file, but should be disabled for performance benchmarking.
- `dbsi_dictionary.h`, `dbsi_dictionary.cpp`, `dbsi_dictionary_utils.h`, `dbsi_dictionary_utils.cpp` : contains the `Dictionary` class which implements various conversions to/from `Resource`s and `CodedResource`s.
- `dbsi_iterator.h` : Provides the `IIterator` interface, used as the basis for all kinds of iterator in this project.
- `dbsi_pattern_utils.h` : Functions to help deal with variable mappings.
- `dbsi_turtle.h`, `dbsi_turtle.cpp` : Implementation of the mechanism to read Turtle files. Again, this is done using iterators.
- `dbsi_rdf_index.h`, `dbsi_rdf_index.cpp` : Implementation of the RDF index, following the paper by Motik et al. Most of the work here is in defining an iterator class which automatically selects which index to use, to apply a given selection criterion.
- `dbsi_rdf_index_helper.h` : This file's purpose is purely to 'construct' the types used to store the table and index in `dbsi_rdf_index.h`. This is nontrivial because, the way I wanted to implement it, requires self-referential types. To achieve this I used the _curiously recurring template pattern_.
- `dbsi_query.h`, `dbsi_query.cpp` : Implementation of the query/command parser.
- `dbsi_nlj.h`, `dbsi_nlj.cpp` : Implementation of nested loop join, as well as the greedy join optimisation algorithm.
- `dbsi_parse_helper.h`, `dbsi_parse_helper.cpp` : Functions to help parse IRIs and Literals. Used in both query parsing and Turtle file loading. The function `parse_resource` is called millions of times in the loading process, so is performance critical.

## Command Line Usage

You can either invoke the executable with no arguments, which enters _interactive mode_, or you can instead use `-i` and `-f` to pass command input and command file input, respectively.
For example, using noninteractive mode you can write:
```./dbsi_project -i "LOAD family_guy.ttl" -i "SELECT ?X ?Z WHERE { ?X <hasAge> ?Y . ?Z <hasAge> ?Y . }"```
and it will print the answers and terminate.

Additional options: using `-L`, it will print the selected join plan for each query (represented as a list of 'triple pattern types', in their evaluation order).
Also, using `-P` is helpful for benchmarking experiments, as it alters the output to be more copy-paste-able into, say, a spreadsheet.

## Compilation

This project uses a very basic `CMakeLists.txt`, so if you know CMake, you can compile this yourself how you like.
The steps are essentially the same as those used in the Database Systems Implementation practicals.
But the precise steps are as follows.
Change directory to the *parent directory of* the root folder of this project, which you have unzipped, so that the project is at `dbsi_project/`.
Then create a folder `build/` (or called whatever you like) and change directory into it.
Run `cmake ../dbsi_project`, followed by `make`.
Both of these should work without errors.
The executable is then available in the new folder `dbsi_project`.

**Please note that this project requires C++17. CMake should already detect this.**
Also, note that `make` might issue warnings, but this is OK.

### Compilers Tested

This code compiles on the department lab machines (using GCC 11.2.1 20220127 (Red Hat 11.2.1-9)), and on Microsoft Visual Studio.
