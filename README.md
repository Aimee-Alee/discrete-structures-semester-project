# Discrete Structures Semester Project

Semester project for Discrete Structures (Fall 2026). Two console programs in C++17, no external libraries.

- `Question1_Logic.cpp`: propositional logic
- `Question2_Graphs.cpp`: graph theory

## Build and run

```bash
g++ -std=c++17 -Wall -Wextra -o q1 Question1_Logic.cpp
g++ -std=c++17 -Wall -Wextra -o q2 Question2_Graphs.cpp
./q1
./q2
```

## Question 1: Propositional logic

Menu options:

1. Simplify an equation step by step using the 13 logic laws, then check the result with a truth table (part a)
2. Contrapositive law demo
3. Apply each of the 13 laws separately
4. Test an argument you type in (part b)
5. Run the 6 required test cases
6. Test the 9 rules of inference
7. Run my own argument

Notes:

- Input: up to 4 variables (single letters), 10 to 20 tokens. `~` or `!`, `&`, `|`, `->` and `<->` can be typed instead of the Unicode symbols.
- The parser is recursive descent and builds an expression tree.
- Laws are tried in a fixed priority order, and the loop stops after 100 steps or when a form repeats.
- Contrapositive is not in the automatic order (it would loop with the conditional law), so it is only reachable from options 2 and 3.
- The simplifier does not always give the smallest form.
- Part b builds the full truth table, marks critical rows and prints a counterexample for invalid arguments.

## Question 2: Graph theory

Start with your own graph or one of the two test graphs from the assignment, then:

1. Check if the graph is bipartite (BFS colouring). If not, find the odd cycle and fix it by subdividing an edge with a new vertex.
2. Find a Hamiltonian path and circuit by backtracking. If there is no circuit, say why and add one edge that keeps the graph bipartite.
3. Add weights, connect the graph if needed, and run Kruskal's algorithm with Union-Find.

The graph is kept in an adjacency matrix and a weight matrix, so each part continues from the result of the previous one.
