Next Steps:

- [ ] I got a big file now, we should use perf for optimization
    - [ ] Optimize for cache coherency
    - [ ] Optimize for branch prediction
    - [ ] optimize hot paths w/ compiler attributes perchance
- [ ] Come up with black box testing methodology for testing emitter

Future Items:

- [ ] string pool (arena) should be interned using a hashmap inside the lexer to avoid duplicate identifiers being used
    - Make sure the API has const pointers and crap
- [ ] USE GPERF -- seems to be the standard for compiler hash tables

- [x] add comments to lexer
- [x] Create basic semantic analyzer for variables
    - Also need it for labels:
- [x] *IMPORTANT:* Looks like the next step is that we need proper semantic analysis for better error handling. We need:
    - Variable analysis (avoid use before defined)
    - Valid expression Analysis (for this, we need static typing)
    - Type analysis (variables will be statically typed to avoid overhead at runtime)
- [x] Add proper argument parsing for verbose option, and for outfile naming
- [x] maybe integrate gcc right in there idk
- [x] test on windows architecture
- [x] GET CROSS COMPILATION WORKING and support windows & linux
- [x] implement basic emitter
  - After some basic research, it seems like x86-64 assembly might be the best
- [x] add labels to lexer
- [x] Implement a proper error handling approach with error reporter that pushes errors
- [x] implement parser ast from tokenarray
- [x] make ast nodeIDs a linked list I think.
  - I don't necessarily need pointers, since I already have nodeIDs.
  - This means I can store the first_child and next_siblings of a node as NodeIDs
- [x] Reorganize directory structure for better maintainability
