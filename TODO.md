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

In a quick test between the proper depth-first ordering of ast_traversal, and the array-order traversal:

- Proper Depth-first: 5.57, 5.56, 5.9, 5.9, 5.7, 5.8, 5.4, 5.5
- Array order (wrong): 5.61, 6.03, 5.51, 5.42, 5.2, 5.5

Average is pretty similar. This cache coherency isn't the issue

## BIGGEST ISSUE:
- hash table re-allocations-- stbds_hm_find_slot
    - stb_ds is not good for this, we are trying khash
- 95M file

HASH TABLE IMPL COMPARISON:
KHASH:
    - 6.67s
STB 
    - 5.7s


Looks like most of the time in the CPU is spend inside the ast methods-- specifically ast_traverse:
```
#
# dso: teeny-perf
#
# Total Lost Samples: 0
#
# Samples: 24K of event 'cycles:P'
# Event count (approx.): 20985064890
#
# Children      Self  Command     Symbol
# ........  ........  ..........  ......................................
#
    20.25%     0.61%  teeny-perf  [.] ast_traverse
            |
            |--19.64%--ast_traverse
            |          |
            |           --19.09%--_ast_traverse_with_context
            |                     |
            |                      --16.18%--_ast_traverse_with_context
            |                                |
            |                                |--6.87%--_ast_traverse_with_context
            |                                |          |
            |                                |          |--4.19%--_ast_traverse_with_context
            |                                |          |          |
            |                                |          |           --3.19%--_ast_traverse_with_context
            |                                |          |                     |
            |                                |          |                      --2.25%--_ast_traverse_with_context
            |                                |          |                                |
            |                                |          |                                |--0.91%--_visit_token
            |                                |          |                                |          |
            |                                |          |                                |           --0.78%--stbds_hmget_key
            |                                |          |                                |                     |
            |                                |          |                                |                      --0.58%--stbds_hm_find_slot
            |                                |          |                                |
            |                                |          |                                 --0.77%--_ast_traverse_with_context
            |                                |          |                                           |
            |                                |          |                                            --0.57%--_visit_token
            |                                |          |                                                      |
            |                                |          |                                                       --0.51%--stbds_hmget_key
            |                                |          |
            |                                |          |--0.82%--_visit_token
            |                                |          |          |
            |                                |          |           --0.71%--stbds_hmget_key
            |                                |          |
            |                                |           --0.56%--visit_token
            |                                |
            |                                |--4.38%--visit_token
            |                                |          |
            |                                |          |--2.58%--stbds_hmget_key
            |                                |          |          |
            |                                |          |           --2.34%--stbds_hm_find_slot
            |                                |          |
            |                                |           --1.47%--stbds_hmput_key
            |                                |                     |
            |                                |                      --0.77%--stbds_make_hash_index
            |                                |                                |
            |                                |                                 --0.52%--asm_exc_page_fault
            |                                |
            |                                 --2.73%--_visit_token
            |                                           |
            |                                            --2.44%--stbds_hmget_key
            |                                                      |
            |                                                       --2.19%--stbds_hm_find_slot
            |
             --0.61%--_start
                       __libc_start_main@@GLIBC_2.34
                       __libc_start_call_main
                       main
                       compiler_execute

    19.10%     7.05%  teeny-perf  [.] _ast_traverse_with_context
            |
            |--12.05%--_ast_traverse_with_context
            |          |
            |           --11.37%--_ast_traverse_with_context
            |                     |
            |                     |--4.38%--visit_token
            |                     |          |
            |                     |          |--2.58%--stbds_hmget_key
            |                     |          |          |
            |                     |          |           --2.34%--stbds_hm_find_slot
            |                     |          |
            |                     |           --1.47%--stbds_hmput_key
            |                     |                     |
            |                     |                      --0.77%--stbds_make_hash_index
            |                     |                                |
            |                     |                                 --0.52%--asm_exc_page_fault
            |                     |
            |                     |--3.74%--_ast_traverse_with_context
            |                     |          |
            |                     |          |--2.07%--_ast_traverse_with_context
            |                     |          |          |
            |                     |          |           --1.84%--_ast_traverse_with_context
            |                     |          |                     |
            |                     |          |                      --1.68%--_ast_traverse_with_context
            |                     |          |                                |
            |                     |          |                                |--0.91%--_visit_token
            |                     |          |                                |          |
            |                     |          |                                |           --0.78%--stbds_hmget_key
            |                     |          |                                |                     |
```
