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


# Performance GOALS 

## GCC

GCC on my codebase with 
- no optimizations
- no linker 
- emitting just asm 
Takes:
> real    0m1.070s
> user    0m0.540s
> sys     0m0.513s
which is approx 0.485MB
That is approx **0.453MB / s**

## Current

As of writing this, the performance on Teeny is much faster. `big_file` is 95M and compiles in 5.75s is 16M/s

## Benchmark

To benchmark, we can test against the tiny basic compiler http://tinybasic.cyningstan.org.uk/ using a similar BENCHMARK.basic file compatable with their compiler. With absolutely 0 emission (just compilation checking)

Their compiler is much simpler and dumber, and makes 2.998s for a 55M is 18M/s

If we emit to C, it takes upwards of a minute-- we will not compare against this.

# PERFORMANCE HISTORY:
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


## AST traversal hotspot

### Array order traversal

In a quick test between the proper depth-first ordering of ast_traversal, and the array-order traversal:

- Proper Depth-first: 5.57, 5.56, 5.9, 5.9, 5.7, 5.8, 5.4, 5.5
- Array order (wrong): 5.61, 6.03, 5.51, 5.42, 5.2, 5.5

Average is pretty similar. This cache coherency isn't the issue

### Recursion

Using a dynamically allocated stack and avoiding recursion actually made it take ~1s longer. The recursive option is the best approach

## Hash Table Performance
- hash table re-allocations-- stbds_hm_find_slot
    - stb_ds is not good for this, we are trying khash
- 95M file

HASH TABLE IMPL COMPARISON:
KHASH:
    - 6.67s
STB 
    - 5.7s


## CPU Time

Looks like most of the time in the CPU is spend inside the ast methods-- specifically ast_traverse.

The self time is mostly septn in stbds_hm_find_slot and _ast_traverse_with_context. Time including children happens mostly in the semantic analyzer, the name table collect, etc.

To be more efficient, perhaps we do less passes of the tree. We can collect the name table as we parse the ast

Self: 
+   14.06%  teeny-perf  [.] stbds_hm_find_slot
+   11.11%  teeny-perf  [.] _ast_traverse_with_context

Children:
+   30.69%     0.95%  teeny-perf  [.] ast_traverse
+   28.90%    11.11%  teeny-perf  [.] _ast_traverse_with_context
+   25.24%     1.15%  teeny-perf  [.] _emit_statement
+   16.53%     0.00%  teeny-perf  [.] semantic_analyzer_check
+   14.32%     0.00%  teeny-perf  [.] name_table_collect_from_ast
+   14.06%    14.06%  teeny-perf  [.] stbds_hm_find_slot
+    8.55%     0.00%  teeny-perf  [.] ast_parse
+    8.37%     1.02%  teeny-perf  [.] _visit_token
+    8.30%     0.52%  teeny-perf  [.] _parse_statement_star_internal
+    7.68%     0.95%  teeny-perf  [.] _parse_statement
+    6.44%     0.46%  teeny-perf  [.] visit_token
