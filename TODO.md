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



PERFORMANCE HISTORY:
- Quick test, if we malloc each node, we go from 3s to 5.7s. We save almost 50% of the time
- In current implementation, there are only 176 mallocs for the big file
- malloc is not the problem

- Init run:
    - Had 23% cache-miss rate 
    - 3.1s user, 2.5 sys
- After capacity-fix: 
    - Had 24% cache-miss rate
    - 3.2 user, 2.5 sys 
    - Makes no difference
- DYNAMIC ARRAY SIZING IS NOT THE PROBLEM

- breadth-first traversal:
    - user time up to 5.1s with 20% cache-miss rate
- array-order traversal
    - did speed up the application by .5s, and slightly reduced cache misses


BIGGEST ISSUE:
- hash table re-allocations-- stbds_hm_find_slot
    - stb_ds is not good for this, we are trying khash
- 95M file

HASH TABLE IMPL COMPARISON:
KHASH:
    - 6.67s
STB 
    - 5.7s
