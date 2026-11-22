// =====================================================================
// Discrete Structures - Semester Project (Fall 2026)
// Question 2: Graph Theory (one menu-driven program)
//   Part (a): Bipartite check (BFS 2-colouring) and fix by edge subdivision
//   Part (b): Hamiltonian path / circuit (backtracking) and fix by adding an edge
//   Part (c): Kruskal's algorithm (Union-Find) and MST, after making the graph
//             weighted and connected
//
// Compile:  g++ -std=c++17 -Wall -Wextra -o Question2_Graphs Question2_Graphs.cpp
// Run:      ./Question2_Graphs
//
// The graph is stored in ONE adjacency matrix (adj) plus a matching weight
// matrix (weight). Every modification changes these stored matrices, so the
// result of Part (a) is really the input of Part (b), and Part (b) is the
// input of Part (c).
// =====================================================================

#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>   // isatty(): echo piped input so the output reads like a session

using namespace std;

// ---------------------------------------------------------------------
// 1. THE GRAPH
// ---------------------------------------------------------------------
struct Graph {
    int n = 0;                       // number of vertices
    vector<string> names;            // vertex labels, e.g. "A", "S1"
    vector<vector<int>> adj;         // adj[i][j] = 1 if edge i-j exists, else 0
    vector<vector<int>> weight;      // weight[i][j] = edge weight (0 = no weight yet)
};

struct Edge {
    int u, v, w;                     // endpoints (u < v) and weight
};

// Adds a new vertex and grows both matrices. Returns its index.
int addVertex(Graph& g, const string& name) {
    g.names.push_back(name);
    g.n++;
    for (auto& row : g.adj) row.push_back(0);
    for (auto& row : g.weight) row.push_back(0);
    g.adj.push_back(vector<int>(g.n, 0));
    g.weight.push_back(vector<int>(g.n, 0));
    return g.n - 1;
}

void addEdge(Graph& g, int u, int v) { g.adj[u][v] = g.adj[v][u] = 1; }

void removeEdge(Graph& g, int u, int v) {
    g.adj[u][v] = g.adj[v][u] = 0;
    g.weight[u][v] = g.weight[v][u] = 0;
}

int findVertex(const Graph& g, const string& name) {
    for (int i = 0; i < g.n; i++)
        if (g.names[i] == name) return i;
    return -1;
}

int degreeOf(const Graph& g, int v) {
    int d = 0;
    for (int j = 0; j < g.n; j++) d += g.adj[v][j];
    return d;
}

int countEdges(const Graph& g) {
    int total = 0;
    for (int i = 0; i < g.n; i++) total += degreeOf(g, i);
    return total / 2;
}

// All edges with u < v, in matrix order.
vector<Edge> listEdges(const Graph& g) {
    vector<Edge> edges;
    for (int i = 0; i < g.n; i++)
        for (int j = i + 1; j < g.n; j++)
            if (g.adj[i][j]) edges.push_back({i, j, g.weight[i][j]});
    return edges;
}

string edgeName(const Graph& g, int u, int v) { return g.names[u] + "-" + g.names[v]; }

// ---------------------------------------------------------------------
// 2. INPUT / OUTPUT HELPERS
// ---------------------------------------------------------------------
// Prints the prompt and reads a line. Piped input is echoed.
bool readLine(const string& prompt, string& line) {
    cout << prompt << flush;
    if (!getline(cin, line)) return false;
    if (!isatty(0)) cout << line << "\n";
    return true;
}

bool readInt(const string& prompt, int& value) {
    string line;
    if (!readLine(prompt, line)) return false;
    try {
        size_t used = 0;
        value = stoi(line, &used);
        if (used != line.size()) value = -1;
    } catch (...) {
        value = -1;
    }
    return true;
}

void printMatrix(const Graph& g, const string& title, bool showWeights) {
    cout << "\n" << title << "\n";
    size_t width = 3;
    for (const string& s : g.names) width = max(width, s.size() + 1);
    cout << string(width, ' ');
    for (const string& s : g.names) cout << s << string(width - s.size(), ' ');
    cout << "\n";
    for (int i = 0; i < g.n; i++) {
        cout << g.names[i] << string(width - g.names[i].size(), ' ');
        for (int j = 0; j < g.n; j++) {
            string cell = to_string(showWeights ? g.weight[i][j] : g.adj[i][j]);
            cout << cell << string(width - cell.size(), ' ');
        }
        cout << "\n";
    }
}

string setToText(const Graph& g, const vector<int>& colour, int wantedColour) {
    string text = "{";
    bool first = true;
    for (int i = 0; i < g.n; i++)
        if (colour[i] == wantedColour) {
            if (!first) text += ", ";
            text += g.names[i];
            first = false;
        }
    return text + "}";
}

string listToText(const Graph& g, const vector<int>& vertices) {
    string text = "{";
    for (size_t i = 0; i < vertices.size(); i++) {
        if (i > 0) text += ", ";
        text += g.names[vertices[i]];
    }
    return text + "}";
}

// ---------------------------------------------------------------------
// 3. BUILDING THE GRAPH (typed by the user, or the two test graphs)
// ---------------------------------------------------------------------
// Test graphs are given as "node -> associated nodes" exactly like the assignment tables.
Graph buildFromAssociations(const vector<pair<string, vector<string>>>& table) {
    Graph g;
    for (const auto& row : table) addVertex(g, row.first);
    for (const auto& row : table) {
        int u = findVertex(g, row.first);
        for (const string& other : row.second) addEdge(g, u, findVertex(g, other));
    }
    return g;
}

Graph testGraph1() {
    return buildFromAssociations({{"A", {"B", "C"}}, {"B", {"A", "D"}}, {"C", {"A", "D"}},
                                  {"D", {"B", "C", "E"}}, {"E", {"D", "F", "G"}},
                                  {"F", {"E", "G"}}, {"G", {"E", "F"}}});
}

Graph testGraph2() {
    return buildFromAssociations({{"S1", {"C1", "C2"}}, {"S2", {"C2"}}, {"S3", {"C3", "C4"}},
                                  {"C1", {"S1"}}, {"C2", {"S1", "S2"}}, {"C3", {"S3"}},
                                  {"C4", {"S3"}}});
}

// Reads vertices and edges from the user with validation. Returns false on end of input.
bool readGraphFromUser(Graph& g) {
    int vertexCount = -1;
    while (vertexCount < 1 || vertexCount > 20) {
        if (!readInt("Number of vertices (1-20): ", vertexCount)) return false;
        if (vertexCount < 1 || vertexCount > 20) cout << "Invalid: enter a whole number from 1 to 20.\n";
    }

    // Vertex labels
    vector<string> labels;
    while (true) {
        string line;
        if (!readLine("Vertex labels separated by spaces (press Enter for A, B, C, ...): ", line)) return false;
        labels.clear();
        istringstream in(line);
        string word;
        while (in >> word) labels.push_back(word);
        if (labels.empty())
            for (int i = 0; i < vertexCount; i++) labels.push_back(string(1, 'A' + i));
        set<string> unique(labels.begin(), labels.end());
        if ((int)labels.size() != vertexCount) cout << "Invalid: you must give exactly " << vertexCount << " labels.\n";
        else if ((int)unique.size() != vertexCount) cout << "Invalid: labels must all be different.\n";
        else break;
    }
    g = Graph();
    for (const string& label : labels) addVertex(g, label);

    int maxEdges = vertexCount * (vertexCount - 1) / 2;
    int edgeCount = -1;
    while (edgeCount < 0 || edgeCount > maxEdges) {
        if (!readInt("Number of edges (0-" + to_string(maxEdges) + "): ", edgeCount)) return false;
        if (edgeCount < 0 || edgeCount > maxEdges) cout << "Invalid: enter a whole number from 0 to " << maxEdges
                                                        << " (a simple graph with " << vertexCount << " vertices has at most that many edges).\n";
    }

    int entered = 0;
    while (entered < edgeCount) {
        string line;
        if (!readLine("Edge " + to_string(entered + 1) + " (two labels, e.g. A B): ", line)) return false;
        istringstream in(line);
        string a, b, extra;
        in >> a >> b;
        if (b.empty() || (in >> extra)) { cout << "Invalid: type exactly two vertex labels.\n"; continue; }
        int u = findVertex(g, a), v = findVertex(g, b);
        if (u < 0 || v < 0) { cout << "Invalid: unknown vertex label.\n"; continue; }
        if (u == v) { cout << "Invalid: self-loops are not allowed.\n"; continue; }
        if (g.adj[u][v]) { cout << "Invalid: that edge was already entered.\n"; continue; }
        addEdge(g, u, v);
        entered++;
    }
    return true;
}

// ---------------------------------------------------------------------
// 4. PART (a): BIPARTITE CHECK BY BFS 2-COLOURING
// ---------------------------------------------------------------------
// colour[i] is 0 or 1 (-1 = not coloured yet). Every component is started
// separately, so disconnected graphs are handled. On the first conflicting
// edge (both ends same colour) it stores the ends in conflictU / conflictV.
bool bfsTwoColour(const Graph& g, vector<int>& colour, vector<int>& parent, int& conflictU, int& conflictV) {
    colour.assign(g.n, -1);
    parent.assign(g.n, -1);
    conflictU = conflictV = -1;
    for (int start = 0; start < g.n; start++) {
        if (colour[start] != -1) continue;       // already reached from an earlier component
        colour[start] = 0;
        queue<int> waiting;
        waiting.push(start);
        while (!waiting.empty()) {
            int u = waiting.front();
            waiting.pop();
            for (int v = 0; v < g.n; v++) {
                if (!g.adj[u][v]) continue;
                if (colour[v] == -1) {
                    colour[v] = 1 - colour[u];   // neighbour gets the opposite colour
                    parent[v] = u;
                    waiting.push(v);
                } else if (colour[v] == colour[u]) {
                    conflictU = u;
                    conflictV = v;
                    return false;                // two touching vertices share a colour
                }
            }
        }
    }
    return true;
}

// Builds the odd cycle from the BFS tree: the two conflicting vertices are joined by
// their paths to the lowest common ancestor. The result starts and ends at that ancestor.
vector<int> buildOddCycle(const vector<int>& parent, int u, int v) {
    set<int> ancestorsOfU;
    for (int x = u; x != -1; x = parent[x]) ancestorsOfU.insert(x);
    int meeting = v;
    while (!ancestorsOfU.count(meeting)) meeting = parent[meeting];   // lowest common ancestor

    vector<int> downToU;                          // meeting ... u
    for (int x = u; x != meeting; x = parent[x]) downToU.push_back(x);
    downToU.push_back(meeting);
    reverse(downToU.begin(), downToU.end());

    vector<int> cycle = downToU;
    for (int x = v; x != meeting; x = parent[x]) cycle.push_back(x);
    cycle.push_back(meeting);                     // close the cycle
    return cycle;
}

// Prints the two sets of a bipartite colouring.
void printBipartiteSets(const Graph& g, const vector<int>& colour) {
    cout << "  Set 1: " << setToText(g, colour, 0) << "\n";
    cout << "  Set 2: " << setToText(g, colour, 1) << "\n";
}

// Runs the check and prints YES/NO output. Returns true when bipartite.
bool reportBipartite(const Graph& g, vector<int>& colour, vector<int>& parent, bool showCycle) {
    int cu, cv;
    bool ok = bfsTwoColour(g, colour, parent, cu, cv);
    if (ok) {
        cout << "Is the graph bipartite? YES\n";
        printBipartiteSets(g, colour);
        return true;
    }
    cout << "Is the graph bipartite? NO\n";
    if (showCycle) {
        vector<int> cycle = buildOddCycle(parent, cu, cv);
        string text;
        for (size_t i = 0; i < cycle.size(); i++) text += (i ? "-" : "") + g.names[cycle[i]];
        cout << "  Conflict: " << g.names[cu] << " and " << g.names[cv]
             << " are adjacent but both got colour " << colour[cu] << ".\n";
        cout << "  Odd cycle found: " << text << "  (length " << cycle.size() - 1 << ")\n";
    }
    return false;
}

// Picks a new vertex name: the first unused capital letter, else N1, N2, ...
string newVertexName(const Graph& g) {
    for (char c = 'A'; c <= 'Z'; c++)
        if (findVertex(g, string(1, c)) < 0) return string(1, c);
    for (int i = 1;; i++)
        if (findVertex(g, "N" + to_string(i)) < 0) return "N" + to_string(i);
}

void partA(Graph& g) {
    cout << "\n---------- PART (a): bipartite check ----------\n";
    printMatrix(g, "Adjacency matrix:", false);
    cout << "\nBFS 2-colouring (every component is checked):\n";
    vector<int> colour, parent;
    if (reportBipartite(g, colour, parent, true)) {
        cout << "No modification needed.\n";
        return;
    }

    // Repeat: find an odd cycle, subdivide one of its edges, re-check.
    int round = 0;
    while (round < 20) {
        int cu, cv;
        if (bfsTwoColour(g, colour, parent, cu, cv)) break;
        round++;
        vector<int> cycle = buildOddCycle(parent, cu, cv);
        // Subdivide the closing edge of the cycle (last vertex before the start, and the start).
        int a = cycle[cycle.size() - 2];
        int b = cycle[0];
        string newName = newVertexName(g);
        int lowEnd = min(a, b), highEnd = max(a, b);   // print edges with the lower index first
        cout << "\nModification " << round << ": subdivide edge " << edgeName(g, lowEnd, highEnd)
             << " with new vertex " << newName << ":\n";
        removeEdge(g, a, b);
        int h = addVertex(g, newName);
        addEdge(g, a, h);
        addEdge(g, h, b);
        cout << "  remove " << edgeName(g, lowEnd, highEnd) << ", add " << g.names[lowEnd] << "-" << newName
             << ", add " << newName << "-" << g.names[highEnd] << "\n";
        printMatrix(g, "Updated adjacency matrix:", false);
        cout << "\nRe-running BFS 2-colouring:\n";
        if (reportBipartite(g, colour, parent, true)) break;
    }
    cout << "\nThe stored graph now has " << g.n << " vertices and " << countEdges(g) << " edges.\n";
}

// ---------------------------------------------------------------------
// 5. PART (b): HAMILTONIAN PATH AND CIRCUIT BY BACKTRACKING
// ---------------------------------------------------------------------
// Tries to extend 'path' until it contains every vertex. If wantCircuit is true,
// the last vertex must also be adjacent to the first. Neighbours are tried in index order.
bool extendPath(const Graph& g, vector<int>& path, vector<bool>& used, bool wantCircuit) {
    if ((int)path.size() == g.n) return wantCircuit ? g.adj[path.back()][path.front()] == 1 : true;
    int last = path.back();
    for (int next = 0; next < g.n; next++) {
        if (!g.adj[last][next] || used[next]) continue;
        path.push_back(next);          // choose
        used[next] = true;
        if (extendPath(g, path, used, wantCircuit)) return true;
        path.pop_back();               // un-choose (backtrack)
        used[next] = false;
    }
    return false;
}

bool findHamiltonianPath(const Graph& g, vector<int>& path) {
    for (int start = 0; start < g.n; start++) {
        path.assign(1, start);
        vector<bool> used(g.n, false);
        used[start] = true;
        if (extendPath(g, path, used, false)) return true;
    }
    path.clear();
    return false;
}

// A circuit passes through every vertex, so it can start at vertex 0.
bool findHamiltonianCircuit(const Graph& g, vector<int>& circuit) {
    if (g.n < 3) { circuit.clear(); return false; }
    circuit.assign(1, 0);
    vector<bool> used(g.n, false);
    used[0] = true;
    if (extendPath(g, circuit, used, true)) {
        circuit.push_back(0);          // show the return to the start
        return true;
    }
    circuit.clear();
    return false;
}

// Number of connected components; also fills compOf with the component number of each vertex.
int findComponents(const Graph& g, vector<int>& compOf) {
    compOf.assign(g.n, -1);
    int count = 0;
    for (int s = 0; s < g.n; s++) {
        if (compOf[s] != -1) continue;
        queue<int> waiting;
        waiting.push(s);
        compOf[s] = count;
        while (!waiting.empty()) {
            int u = waiting.front();
            waiting.pop();
            for (int v = 0; v < g.n; v++)
                if (g.adj[u][v] && compOf[v] == -1) { compOf[v] = count; waiting.push(v); }
        }
        count++;
    }
    return count;
}

// A bridge is an edge whose removal splits a component.
vector<Edge> findBridges(const Graph& g) {
    vector<Edge> bridges;
    vector<int> comp;
    int before = findComponents(g, comp);
    Graph copy = g;
    for (const Edge& e : listEdges(g)) {
        removeEdge(copy, e.u, e.v);
        if (findComponents(copy, comp) > before) bridges.push_back(e);
        addEdge(copy, e.u, e.v);
    }
    return bridges;
}

// Explains why no Hamiltonian circuit exists (bridges and forced degree-2 edges).
void explainNoCircuit(const Graph& g) {
    cout << "\nWhy there is no Hamiltonian circuit:\n";
    vector<Edge> bridges = findBridges(g);
    for (const Edge& e : bridges)
        cout << "  - " << edgeName(g, e.u, e.v) << " is a bridge (removing it disconnects the graph). A circuit that visits\n"
             << "    both sides would have to cross it twice, but a circuit may not reuse an edge.\n";
    if (bridges.empty()) cout << "  - The graph has no bridges.\n";

    // Edges at degree-2 vertices are forced: both edges must be in any circuit.
    // Build forced edges, then look for a closed loop made only of forced edges.
    vector<vector<int>> forced(g.n);
    for (int v = 0; v < g.n; v++)
        if (degreeOf(g, v) == 2)
            for (int w = 0; w < g.n; w++)
                if (g.adj[v][w]) {
                    if (find(forced[v].begin(), forced[v].end(), w) == forced[v].end()) forced[v].push_back(w);
                    if (find(forced[w].begin(), forced[w].end(), v) == forced[w].end()) forced[w].push_back(v);
                }
    vector<bool> seen(g.n, false);
    bool foundEarlyLoop = false;
    for (int s = 0; s < g.n && !foundEarlyLoop; s++) {
        if (seen[s] || forced[s].empty()) continue;
        vector<int> stack = {s}, members;
        seen[s] = true;
        bool allTwo = true;
        while (!stack.empty()) {
            int x = stack.back();
            stack.pop_back();
            members.push_back(x);
            if (forced[x].size() != 2) allTwo = false;
            for (int y : forced[x])
                if (!seen[y]) { seen[y] = true; stack.push_back(y); }
        }
        if (allTwo && (int)members.size() < g.n) {
            sort(members.begin(), members.end());
            cout << "  - Every vertex of degree 2 forces both of its edges. The forced edges close a loop on only "
                 << listToText(g, members) << ",\n    which leaves the other vertices out.\n";
            foundEarlyLoop = true;
        }
    }
}

// Is the given list of vertex names a Hamiltonian path of the graph?
bool isHamiltonianPath(const Graph& g, const vector<string>& nameList) {
    if ((int)nameList.size() != g.n) return false;
    vector<bool> used(g.n, false);
    int previous = -1;
    for (const string& name : nameList) {
        int v = findVertex(g, name);
        if (v < 0 || used[v]) return false;
        if (previous >= 0 && !g.adj[previous][v]) return false;
        used[v] = true;
        previous = v;
    }
    return true;
}

// Finds the pairs (u, v) that lie in different bipartite sets, are not adjacent yet,
// and give a Hamiltonian circuit when added.
vector<pair<int, int>> findCircuitFixes(const Graph& g, const vector<int>& colour) {
    vector<pair<int, int>> fixes;
    for (int u = 0; u < g.n; u++)
        for (int v = u + 1; v < g.n; v++) {
            if (g.adj[u][v] || colour[u] == colour[v]) continue;
            Graph trial = g;
            addEdge(trial, u, v);
            vector<int> circuit;
            if (findHamiltonianCircuit(trial, circuit)) fixes.push_back({u, v});
        }
    return fixes;
}

void partB(Graph& g) {
    cout << "\n---------- PART (b): Hamiltonian path and circuit ----------\n";
    vector<int> colour, parent;
    int cu, cv;
    if (!bfsTwoColour(g, colour, parent, cu, cv)) {
        cout << "The stored graph is not bipartite. Run option 1 first (Part b starts from its result).\n";
        return;
    }
    cout << "Starting graph: " << g.n << " vertices, " << countEdges(g) << " edges (bipartite).\n";
    printMatrix(g, "Adjacency matrix:", false);

    vector<int> path, circuit;
    cout << "\nSearching for a Hamiltonian PATH (backtracking):\n";
    if (findHamiltonianPath(g, path)) cout << "  Hamiltonian path found: " << listToText(g, path) << "\n";
    else cout << "  No Hamiltonian path exists.\n";
    vector<string> example = {"B", "A", "C", "D", "E", "H", "G", "F"};
    if (g.n == 8 && findVertex(g, "H") >= 0 && findVertex(g, "B") >= 0)
        cout << "  Check of the assignment's example {B, A, C, D, E, H, G, F}: "
             << (isHamiltonianPath(g, example) ? "also a valid Hamiltonian path" : "NOT a valid path") << "\n";

    cout << "\nSearching for a Hamiltonian CIRCUIT (backtracking):\n";
    if (findHamiltonianCircuit(g, circuit)) {
        cout << "  Hamiltonian circuit found: " << listToText(g, circuit) << "\n";
        cout << "  The graph already has a circuit; no modification needed.\n";
        return;
    }
    cout << "  No Hamiltonian circuit exists.\n";
    explainNoCircuit(g);

    // Choose an edge to add. It must join the two bipartite sets so the graph stays bipartite.
    vector<pair<int, int>> fixes = findCircuitFixes(g, colour);
    if (fixes.empty()) {
        cout << "\nNo single edge between the two sets creates a Hamiltonian circuit.\n";
        return;
    }
    cout << "\nEdges between the two sets that would create a Hamiltonian circuit: ";
    for (size_t i = 0; i < fixes.size(); i++)
        cout << (i ? ", " : "") << edgeName(g, fixes[i].first, fixes[i].second);
    cout << "\n";

    int u = -1, v = -1;
    while (true) {
        string line;
        if (!readLine("Enter the two vertices of the edge to add (e.g. F C), or press Enter for the first one: ", line)) return;
        if (line.empty()) { u = fixes[0].first; v = fixes[0].second; break; }
        istringstream in(line);
        string a, b;
        in >> a >> b;
        u = findVertex(g, a);
        v = findVertex(g, b);
        bool listed = false;
        for (auto& f : fixes)
            if ((f.first == u && f.second == v) || (f.first == v && f.second == u)) listed = true;
        if (listed) break;
        cout << "Invalid: choose one of the listed edges.\n";
    }

    cout << "\nModification: add edge " << g.names[u] << "-" << g.names[v] << " (" << g.names[u] << " and "
         << g.names[v] << " are in different sets, so the graph stays bipartite).\n";
    addEdge(g, u, v);
    printMatrix(g, "Updated adjacency matrix:", false);

    cout << "\nSearching again for a Hamiltonian CIRCUIT:\n";
    if (findHamiltonianCircuit(g, circuit)) cout << "  Hamiltonian circuit found: " << listToText(g, circuit) << "\n";
    else cout << "  Still no Hamiltonian circuit.\n";

    cout << "\nRe-checking bipartiteness after the modification:\n";
    reportBipartite(g, colour, parent, true);
}

// ---------------------------------------------------------------------
// 6. PART (c): KRUSKAL'S ALGORITHM WITH UNION-FIND
// ---------------------------------------------------------------------
// Union-Find: parentOf[x] points towards the representative (root) of x's set.
struct UnionFind {
    vector<int> parentOf;
    explicit UnionFind(int size) : parentOf(size) {
        for (int i = 0; i < size; i++) parentOf[i] = i;
    }
    int find(int x) {                                  // Find: follow parents to the root
        if (parentOf[x] != x) parentOf[x] = find(parentOf[x]);   // path compression
        return parentOf[x];
    }
    bool unite(int a, int b) {                         // Union: join two sets; false if same set
        int rootA = find(a), rootB = find(b);
        if (rootA == rootB) return false;
        parentOf[rootB] = rootA;
        return true;
    }
};

// The weights given in the assignment, used when the user agrees to take them.
int knownWeight(const Graph& g, int u, int v) {
    static const map<string, int> table = {
        {"A-B", 4}, {"A-C", 6}, {"B-D", 5}, {"C-D", 3}, {"D-E", 8}, {"E-F", 2}, {"F-G", 7},
        {"E-H", 3}, {"G-H", 4}, {"C-F", 9},
        {"C1-S1", 2}, {"C2-S1", 4}, {"C2-S2", 1}, {"C3-S3", 3}, {"C4-S3", 5}, {"C3-S2", 6}};
    string a = g.names[u], b = g.names[v];
    if (a > b) swap(a, b);
    auto it = table.find(a + "-" + b);
    return it == table.end() ? 0 : it->second;
}

void printWeightedGraph(const Graph& g) {
    cout << "\nWeighted graph (edge list):\n";
    for (const Edge& e : listEdges(g)) cout << "  " << edgeName(g, e.u, e.v) << " = " << e.w << "\n";
    printMatrix(g, "Weight matrix (0 = no edge):", true);
}

// Step 1: every edge needs a positive weight.
bool assignWeights(Graph& g) {
    bool needsWeights = false;
    for (const Edge& e : listEdges(g))
        if (e.w == 0) needsWeights = true;
    if (!needsWeights) { cout << "All edges already have positive weights.\n"; return true; }

    string answer;
    if (!readLine("Use the weights given in the assignment where they exist? (y/n): ", answer)) return false;
    bool useKnown = !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
    for (const Edge& e : listEdges(g)) {
        if (e.w != 0) continue;
        int w = useKnown ? knownWeight(g, e.u, e.v) : 0;
        while (w <= 0) {
            if (!readInt("Weight for edge " + edgeName(g, e.u, e.v) + " (positive integer): ", w)) return false;
            if (w <= 0) cout << "Invalid: the weight must be a positive whole number.\n";
        }
        g.weight[e.u][e.v] = g.weight[e.v][e.u] = w;
    }
    return true;
}

// Step 2: a spanning tree needs a connected graph. Joins the components if necessary.
bool makeConnected(Graph& g) {
    vector<int> compOf;
    int components = findComponents(g, compOf);
    cout << "\nConnectivity check: the graph has " << components << " connected component(s).\n";
    if (components == 1) { cout << "The graph is connected, so a spanning tree exists.\n"; return true; }

    for (int c = 0; c < components; c++) {
        vector<int> members;
        for (int i = 0; i < g.n; i++)
            if (compOf[i] == c) members.push_back(i);
        cout << "  Component " << c + 1 << ": " << listToText(g, members) << "\n";
    }
    cout << "No spanning tree exists: a tree must reach every vertex, but there is no path between the components.\n";

    // Join components one by one. Use the lowest-degree vertices so no vertex becomes crowded,
    // and choose endpoints with different colours so the graph stays bipartite.
    while (components > 1) {
        vector<int> colour, parent;
        int cu, cv;
        bool bipartiteNow = bfsTwoColour(g, colour, parent, cu, cv);
        int bestU = -1, bestV = -1;
        for (int u = 0; u < g.n; u++) {
            if (compOf[u] != 0) continue;                      // first component
            for (int v = 0; v < g.n; v++) {
                if (compOf[v] == 0) continue;
                if (bipartiteNow && colour[u] == colour[v]) continue;
                bool better = bestU < 0 ||
                              degreeOf(g, u) + degreeOf(g, v) < degreeOf(g, bestU) + degreeOf(g, bestV);
                if (better) { bestU = u; bestV = v; }
            }
        }
        if (bestU < 0) {                                       // fallback: any pair
            for (int v = 0; v < g.n && bestU < 0; v++)
                if (compOf[v] != 0) { bestU = 0; bestV = v; }
        }
        cout << "\nModification: add edge " << edgeName(g, bestU, bestV) << " to join the components";
        if (bipartiteNow) cout << " (the two ends are in different sets, so the graph stays bipartite)";
        cout << ".\n";
        int w = knownWeight(g, bestU, bestV);
        while (w <= 0) {
            if (!readInt("Weight for the new edge " + edgeName(g, bestU, bestV) + " (positive integer): ", w)) return false;
            if (w <= 0) cout << "Invalid: the weight must be a positive whole number.\n";
        }
        addEdge(g, bestU, bestV);
        g.weight[bestU][bestV] = g.weight[bestV][bestU] = w;
        cout << "  Added " << edgeName(g, bestU, bestV) << " with weight " << w << ".\n";
        components = findComponents(g, compOf);
    }
    cout << "\nConnectivity re-check: the graph now has " << components << " connected component.\n";
    return true;
}

void partC(Graph& g) {
    cout << "\n---------- PART (c): Kruskal's algorithm ----------\n";
    printMatrix(g, "Adjacency matrix of the stored graph:", false);
    if (!assignWeights(g)) return;
    printWeightedGraph(g);
    int edgesBefore = countEdges(g);
    if (!makeConnected(g)) return;

    vector<int> colour, parent;
    int cu, cv;
    cout << "\nBipartite check of the connected graph: "
         << (bfsTwoColour(g, colour, parent, cu, cv) ? "still bipartite" : "not bipartite") << "\n";
    if (countEdges(g) != edgesBefore) printWeightedGraph(g);   // show the graph again after the repair

    // Sort edges by weight; equal weights keep matrix order (stable sort).
    vector<Edge> edges = listEdges(g);
    stable_sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) { return a.w < b.w; });
    cout << "\nEdges sorted by increasing weight:\n ";
    for (const Edge& e : edges) cout << " " << edgeName(g, e.u, e.v) << "(" << e.w << ")";
    cout << "\n";

    cout << "\nKruskal's algorithm (a cycle is detected when both ends have the same Find root):\n";
    UnionFind sets(g.n);
    vector<Edge> tree;
    long long totalWeight = 0;
    vector<Edge> rejected;
    for (const Edge& e : edges) {
        if ((int)tree.size() == g.n - 1) {
            cout << "  The tree already has " << g.n - 1 << " edges; the remaining edges are not needed.\n";
            break;
        }
        int rootU = sets.find(e.u), rootV = sets.find(e.v);
        cout << "  Consider " << edgeName(g, e.u, e.v) << " (" << e.w << "):  Find(" << g.names[e.u] << ")="
             << g.names[rootU] << ", Find(" << g.names[e.v] << ")=" << g.names[rootV] << "  ->  ";
        if (sets.unite(e.u, e.v)) {
            cout << "ACCEPTED (different sets, Union done)\n";
            tree.push_back(e);
            totalWeight += e.w;
        } else {
            cout << "REJECTED (same set, would form a cycle)\n";
            rejected.push_back(e);
        }
    }

    if ((int)tree.size() != g.n - 1) {
        cout << "\nOnly " << tree.size() << " edges were accepted, so no spanning tree exists.\n";
        return;
    }
    cout << "\nMinimum spanning tree edges:\n";
    for (const Edge& e : tree) cout << "  " << edgeName(g, e.u, e.v) << " (" << e.w << ")\n";
    cout << "Rejected edges (cycles):";
    if (rejected.empty()) cout << " none";
    for (const Edge& e : rejected) cout << " " << edgeName(g, e.u, e.v) << "(" << e.w << ")";
    cout << "\nNumber of MST edges: " << tree.size() << "  (= vertices - 1 = " << g.n - 1 << ")\n";
    cout << "Total MST weight: " << totalWeight << "\n";
}

// ---------------------------------------------------------------------
// 7. MAIN: choose the starting graph, then the menu
// ---------------------------------------------------------------------
int main() {
    Graph g;
    cout << "=========== Question 2: Graph Theory ===========\n"
            "How do you want to enter the graph?\n"
            "  1. Type my own graph (vertices, edges)\n"
            "  2. Test Graph 1 (from the assignment)\n"
            "  3. Test Graph 2 (from the assignment)\n";
    int source = 0;
    while (source < 1 || source > 3) {
        if (!readInt("Choice: ", source)) return 0;
        if (source < 1 || source > 3) cout << "Please enter 1, 2 or 3.\n";
    }
    if (source == 1) { if (!readGraphFromUser(g)) return 0; }
    else g = (source == 2) ? testGraph1() : testGraph2();
    printMatrix(g, "Adjacency matrix of the input graph:", false);
    cout << "(" << g.n << " vertices, " << countEdges(g) << " edges)\n";

    while (true) {
        cout << "\n================= MENU =================\n"
                "1. Check whether the graph is bipartite and make it bipartite if necessary\n"
                "2. Make the bipartite graph Hamiltonian\n"
                "3. Make the graph Kruskal-ready and find its MST\n"
                "4. Exit\n";
        int choice = 0;
        if (!readInt("Choice: ", choice)) break;
        if (choice == 1) partA(g);
        else if (choice == 2) partB(g);
        else if (choice == 3) partC(g);
        else if (choice == 4) break;
        else cout << "Please enter a number from 1 to 4.\n";
    }
    return 0;
}
