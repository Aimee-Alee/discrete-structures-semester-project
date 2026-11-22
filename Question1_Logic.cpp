// =====================================================================
// Discrete Structures - Semester Project (Fall 2026)
// Question 1: Propositional Logic
//   Part (a): Simplification using the 13 logic laws
//   Part (b): Testing rules of inference with truth tables
//
// Compile:  g++ -std=c++17 -Wall -Wextra -o Question1_Logic Question1_Logic.cpp
// Run:      ./Question1_Logic
//
// Symbols accepted (ASCII alternatives in brackets):
//   ¬ [~ or !]   ∧ [&]   ∨ [|]   → [->]   ↔ [<->]   ( )   T   F
//
// OPERATOR PRECEDENCE (highest binds first):
//   1. ¬   (right-to-left, e.g. ¬¬p)
//   2. ∧   (left-associative)
//   3. ∨   (left-associative)
//   4. →   (RIGHT-associative:  p → q → r  means  p → (q → r))
//   5. ↔   (left-associative)
// =====================================================================

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>   // isatty(): used to echo piped input so output looks like a session

using namespace std;

// ---------------------------------------------------------------------
// 1. EXPRESSION TREE
// ---------------------------------------------------------------------
enum NodeType { VAR, CONST_T, CONST_F, NOT, AND, OR, IMP, IFF };

struct Node;
typedef shared_ptr<Node> NodePtr;

struct Node {
    NodeType type;
    char name;       // only used when type == VAR
    NodePtr left;    // child (NOT uses only 'left')
    NodePtr right;
};

NodePtr makeVar(char c) { return make_shared<Node>(Node{VAR, c, nullptr, nullptr}); }
NodePtr makeConst(bool value) {
    return make_shared<Node>(Node{value ? CONST_T : CONST_F, ' ', nullptr, nullptr});
}
NodePtr makeNot(NodePtr a) { return make_shared<Node>(Node{NOT, ' ', a, nullptr}); }
NodePtr makeBinary(NodeType t, NodePtr a, NodePtr b) {
    return make_shared<Node>(Node{t, ' ', a, b});
}

bool isBinaryType(NodeType t) { return t == AND || t == OR || t == IMP || t == IFF; }

// ---------------------------------------------------------------------
// 2. PRINTING A TREE AS TEXT
// ---------------------------------------------------------------------
// Parentheses are added around every binary child that sits inside a
// binary parent, except when the same operator continues in the
// direction of its associativity. This keeps the tree shape visible,
// which matters for showing the Associative Law.
string show(const NodePtr& n);

string symbolOf(NodeType t) {
    switch (t) {
        case AND: return "∧";
        case OR:  return "∨";
        case IMP: return "→";
        default:  return "↔";
    }
}

string showChild(const NodePtr& child, NodeType parentType, bool isLeftChild) {
    string text = show(child);
    if (!isBinaryType(child->type)) return text;
    bool rightAssociative = (parentType == IMP);
    bool sameOperator = (child->type == parentType);
    bool noParens = sameOperator && (isLeftChild != rightAssociative);
    return noParens ? text : "(" + text + ")";
}

string show(const NodePtr& n) {
    switch (n->type) {
        case VAR:     return string(1, n->name);
        case CONST_T: return "T";
        case CONST_F: return "F";
        case NOT:
            if (n->left->type == VAR || n->left->type == NOT ||
                n->left->type == CONST_T || n->left->type == CONST_F)
                return "¬" + show(n->left);
            return "¬(" + show(n->left) + ")";
        default:
            return showChild(n->left, n->type, true) + " " + symbolOf(n->type) + " " +
                   showChild(n->right, n->type, false);
    }
}

// ---------------------------------------------------------------------
// 3. TOKENIZER
// ---------------------------------------------------------------------
enum TokenKind { TK_VAR, TK_TRUE, TK_FALSE, TK_NOT, TK_AND, TK_OR, TK_IMP, TK_IFF,
                 TK_LPAREN, TK_RPAREN };

struct Token {
    TokenKind kind;
    char name;      // variable letter (for TK_VAR)
    string text;    // text shown in error messages
};

bool isBinaryToken(TokenKind k) { return k == TK_AND || k == TK_OR || k == TK_IMP || k == TK_IFF; }
bool isOperatorToken(TokenKind k) { return isBinaryToken(k) || k == TK_NOT; }

// Length in bytes of the UTF-8 character that starts with byte 'lead'.
int utf8Length(unsigned char lead) {
    if (lead >= 0xF0) return 4;
    if (lead >= 0xE0) return 3;
    if (lead >= 0xC0) return 2;
    return 1;
}

// Number of characters shown on screen (for aligning table columns).
int displayLength(const string& s) {
    int count = 0;
    for (unsigned char c : s)
        if ((c & 0xC0) != 0x80) count++;
    return count;
}

string padRight(const string& s, int width) {
    int extra = width - displayLength(s);
    return s + string(extra > 0 ? extra : 0, ' ');
}

// Splits the text into tokens. Throws runtime_error("Rejected: ...") on a bad symbol.
vector<Token> tokenize(const string& input) {
    vector<Token> tokens;
    int depth = 0;  // parenthesis depth, used for the balance check
    size_t i = 0;

    while (i < input.size()) {
        unsigned char c = input[i];

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { i++; continue; }

        // Operators: UTF-8 symbols first, then ASCII alternatives.
        struct Sym { const char* text; TokenKind kind; };
        static const Sym symbols[] = {
            {"¬", TK_NOT}, {"∧", TK_AND}, {"∨", TK_OR}, {"→", TK_IMP}, {"↔", TK_IFF},
            {"<->", TK_IFF}, {"->", TK_IMP}, {"~", TK_NOT}, {"!", TK_NOT},
            {"&", TK_AND}, {"|", TK_OR}};
        bool matched = false;
        for (const Sym& s : symbols) {
            string sym = s.text;
            if (input.compare(i, sym.size(), sym) == 0) {
                static const map<TokenKind, string> shown = {
                    {TK_NOT, "¬"}, {TK_AND, "∧"}, {TK_OR, "∨"}, {TK_IMP, "→"}, {TK_IFF, "↔"}};
                tokens.push_back({s.kind, ' ', shown.at(s.kind)});
                i += sym.size();
                matched = true;
                break;
            }
        }
        if (matched) continue;

        if (c == '(') { depth++; tokens.push_back({TK_LPAREN, ' ', "("}); i++; continue; }
        if (c == ')') {
            depth--;
            if (depth < 0) throw runtime_error("Rejected: unbalanced parentheses (extra ')').");
            tokens.push_back({TK_RPAREN, ' ', ")"});
            i++;
            continue;
        }

        // Letters/digits: read the whole word so "pq" and "x1" can be rejected.
        if (isalnum(c) || c == '_') {
            size_t start = i;
            while (i < input.size() && (isalnum((unsigned char)input[i]) || input[i] == '_')) i++;
            string word = input.substr(start, i - start);
            if (word.size() > 1)
                throw runtime_error("Rejected: variable \"" + word + "\" is longer than 1 character.");
            if (word == "T") tokens.push_back({TK_TRUE, ' ', "T"});
            else if (word == "F") tokens.push_back({TK_FALSE, ' ', "F"});
            else if (isalpha((unsigned char)word[0])) tokens.push_back({TK_VAR, word[0], word});
            else throw runtime_error("Rejected: unknown symbol \"" + word + "\".");
            continue;
        }

        int len = utf8Length(c);
        throw runtime_error("Rejected: unknown symbol \"" + input.substr(i, len) + "\".");
    }

    if (depth > 0) throw runtime_error("Rejected: unbalanced parentheses (missing ')').");
    return tokens;
}

// ---------------------------------------------------------------------
// 4. RECURSIVE-DESCENT PARSER  (one function per precedence level)
// ---------------------------------------------------------------------
struct Parser {
    vector<Token> tokens;
    size_t pos = 0;
};

NodePtr parseIff(Parser& p);

NodePtr parsePrimary(Parser& p) {
    if (p.pos >= p.tokens.size())
        throw runtime_error("Rejected: missing operand after '" + p.tokens.back().text + "'.");
    Token t = p.tokens[p.pos];
    const Token* prev = (p.pos > 0) ? &p.tokens[p.pos - 1] : nullptr;

    if (t.kind == TK_VAR) { p.pos++; return makeVar(t.name); }
    if (t.kind == TK_TRUE) { p.pos++; return makeConst(true); }
    if (t.kind == TK_FALSE) { p.pos++; return makeConst(false); }

    if (t.kind == TK_LPAREN) {
        p.pos++;
        NodePtr inside = parseIff(p);
        if (p.pos >= p.tokens.size() || p.tokens[p.pos].kind != TK_RPAREN)
            throw runtime_error("Rejected: missing operator between '" +
                                p.tokens[p.pos - 1].text + "' and '" + p.tokens[p.pos].text + "'.");
        p.pos++;
        return inside;
    }

    // Anything else cannot start an operand.
    if (isBinaryToken(t.kind)) {
        if (prev && isOperatorToken(prev->kind))
            throw runtime_error("Rejected: two operators in a row ('" + prev->text + "' followed by '" + t.text + "').");
        throw runtime_error("Rejected: missing operand before '" + t.text + "'.");
    }
    // t is ')'
    if (prev && prev->kind == TK_LPAREN) throw runtime_error("Rejected: empty parentheses '()'.");
    throw runtime_error("Rejected: missing operand before ')' (after '" + prev->text + "').");
}

NodePtr parseNot(Parser& p) {
    if (p.pos < p.tokens.size() && p.tokens[p.pos].kind == TK_NOT) {
        p.pos++;
        return makeNot(parseNot(p));
    }
    return parsePrimary(p);
}

NodePtr parseAnd(Parser& p) {
    NodePtr left = parseNot(p);
    while (p.pos < p.tokens.size() && p.tokens[p.pos].kind == TK_AND) {
        p.pos++;
        left = makeBinary(AND, left, parseNot(p));
    }
    return left;
}

NodePtr parseOr(Parser& p) {
    NodePtr left = parseAnd(p);
    while (p.pos < p.tokens.size() && p.tokens[p.pos].kind == TK_OR) {
        p.pos++;
        left = makeBinary(OR, left, parseAnd(p));
    }
    return left;
}

NodePtr parseImp(Parser& p) {
    NodePtr left = parseOr(p);
    if (p.pos < p.tokens.size() && p.tokens[p.pos].kind == TK_IMP) {
        p.pos++;
        return makeBinary(IMP, left, parseImp(p));   // right-associative
    }
    return left;
}

NodePtr parseIff(Parser& p) {
    NodePtr left = parseImp(p);
    while (p.pos < p.tokens.size() && p.tokens[p.pos].kind == TK_IFF) {
        p.pos++;
        left = makeBinary(IFF, left, parseImp(p));
    }
    return left;
}

// Limits for the input. Part (a) uses 10..20 tokens and 4 variables.
struct Limits {
    int minTokens;
    int maxTokens;
    int maxVariables;
};

void collectVariables(const NodePtr& n, set<char>& vars) {
    if (!n) return;
    if (n->type == VAR) vars.insert(n->name);
    collectVariables(n->left, vars);
    collectVariables(n->right, vars);
}

// Full validation + parsing. Throws runtime_error with the rejection reason.
NodePtr parseEquation(const string& text, const Limits& limits) {
    vector<Token> tokens = tokenize(text);
    if (tokens.empty()) throw runtime_error("Rejected: the equation is empty.");

    Parser parser;
    parser.tokens = tokens;
    NodePtr tree = parseIff(parser);
    if (parser.pos < tokens.size())
        throw runtime_error("Rejected: missing operator between '" + tokens[parser.pos - 1].text +
                            "' and '" + tokens[parser.pos].text + "'.");

    set<char> vars;
    collectVariables(tree, vars);
    if ((int)vars.size() > limits.maxVariables)
        throw runtime_error("Rejected: more than " + to_string(limits.maxVariables) + " distinct variables.");

    int tokenCount = 0;   // parentheses are not counted
    for (const Token& t : tokens)
        if (t.kind != TK_LPAREN && t.kind != TK_RPAREN) tokenCount++;
    if (tokenCount < limits.minTokens)
        throw runtime_error("Rejected: equation has " + to_string(tokenCount) + " tokens (minimum is " +
                            to_string(limits.minTokens) + ").");
    if (tokenCount > limits.maxTokens)
        throw runtime_error("Rejected: equation has " + to_string(tokenCount) + " tokens (maximum is " +
                            to_string(limits.maxTokens) + ").");
    return tree;
}

// ---------------------------------------------------------------------
// 5. EVALUATION AND TRUTH TABLES (independent of the simplifier)
// ---------------------------------------------------------------------
bool evaluate(const NodePtr& n, const map<char, bool>& values) {
    switch (n->type) {
        case VAR:     return values.at(n->name);
        case CONST_T: return true;
        case CONST_F: return false;
        case NOT:     return !evaluate(n->left, values);
        case AND:     return evaluate(n->left, values) && evaluate(n->right, values);
        case OR:      return evaluate(n->left, values) || evaluate(n->right, values);
        case IMP:     return !evaluate(n->left, values) || evaluate(n->right, values);
        default:      return evaluate(n->left, values) == evaluate(n->right, values);
    }
}

// Value of variable number 'index' in row 'row' (first variable changes slowest, T first).
map<char, bool> assignmentForRow(const vector<char>& vars, int row) {
    map<char, bool> values;
    int count = vars.size();
    for (int i = 0; i < count; i++)
        values[vars[i]] = !((row >> (count - 1 - i)) & 1);
    return values;
}

string tf(bool b) { return b ? "T" : "F"; }

// Compares two trees on every row. Prints the table when 'print' is true.
bool truthTableEquivalent(const NodePtr& a, const NodePtr& b, bool print) {
    set<char> varSet;
    collectVariables(a, varSet);
    collectVariables(b, varSet);
    vector<char> vars(varSet.begin(), varSet.end());
    int rows = 1 << vars.size();
    bool equivalent = true;

    if (print) {
        cout << "\n  ";
        for (char v : vars) cout << v << "  ";
        cout << "| Original | Final\n";
    }
    for (int row = 0; row < rows; row++) {
        map<char, bool> values = assignmentForRow(vars, row);
        bool resultA = evaluate(a, values);
        bool resultB = evaluate(b, values);
        if (resultA != resultB) equivalent = false;
        if (print) {
            cout << "  ";
            for (char v : vars) cout << tf(values[v]) << "  ";
            cout << "|    " << tf(resultA) << "     |   " << tf(resultB)
                 << (resultA != resultB ? "   <-- DIFFERENT" : "") << "\n";
        }
    }
    return equivalent;
}

// ---------------------------------------------------------------------
// 6. THE 13 LAWS.  Each law function looks at ONE node and returns the
//    rewritten node, or nullptr when the law does not apply there.
// ---------------------------------------------------------------------
typedef NodePtr (*LawFunction)(const NodePtr&);

bool sameTree(const NodePtr& a, const NodePtr& b) {
    if (a->type != b->type) return false;
    if (a->type == VAR) return a->name == b->name;
    if (a->type == CONST_T || a->type == CONST_F) return true;
    if (a->type == NOT) return sameTree(a->left, b->left);
    return sameTree(a->left, b->left) && sameTree(a->right, b->right);
}

bool isComplement(const NodePtr& a, const NodePtr& b) {
    return (a->type == NOT && sameTree(a->left, b)) || (b->type == NOT && sameTree(b->left, a));
}

// "Pair" = equal (x, x) or complementary (x, ¬x).
bool isPair(const NodePtr& a, const NodePtr& b) { return sameTree(a, b) || isComplement(a, b); }

bool isAndOr(const NodePtr& n) { return n->type == AND || n->type == OR; }

// Law 12: p ↔ q  ≡  (p → q) ∧ (q → p)
NodePtr lawBiconditional(const NodePtr& n) {
    if (n->type != IFF) return nullptr;
    return makeBinary(AND, makeBinary(IMP, n->left, n->right), makeBinary(IMP, n->right, n->left));
}

// Law 11: p → q  ≡  ¬p ∨ q
NodePtr lawConditional(const NodePtr& n) {
    if (n->type != IMP) return nullptr;
    return makeBinary(OR, makeNot(n->left), n->right);
}

// Law 13: p → q  ≡  ¬q → ¬p   (NOT part of the automatic order, see main notes)
NodePtr lawContrapositive(const NodePtr& n) {
    if (n->type != IMP) return nullptr;
    return makeBinary(IMP, makeNot(n->right), makeNot(n->left));
}

// Law 4: ¬(¬p) ≡ p
NodePtr lawDoubleNegation(const NodePtr& n) {
    if (n->type == NOT && n->left->type == NOT) return n->left->left;
    return nullptr;
}

// Law 9: ¬(p ∧ q) ≡ ¬p ∨ ¬q ,  ¬(p ∨ q) ≡ ¬p ∧ ¬q
NodePtr lawDeMorgan(const NodePtr& n) {
    if (n->type != NOT || !isAndOr(n->left)) return nullptr;
    NodeType flipped = (n->left->type == AND) ? OR : AND;
    return makeBinary(flipped, makeNot(n->left->left), makeNot(n->left->right));
}

// Law 5: p ∨ ¬p ≡ T ,  p ∧ ¬p ≡ F   (also ¬T ≡ F and ¬F ≡ T, the negation of a constant)
NodePtr lawNegation(const NodePtr& n) {
    if (n->type == OR && isComplement(n->left, n->right)) return makeConst(true);
    if (n->type == AND && isComplement(n->left, n->right)) return makeConst(false);
    if (n->type == NOT && n->left->type == CONST_T) return makeConst(false);
    if (n->type == NOT && n->left->type == CONST_F) return makeConst(true);
    return nullptr;
}

// Law 2: p ∨ T ≡ T ,  p ∧ F ≡ F
NodePtr lawDomination(const NodePtr& n) {
    if (n->type == OR && (n->left->type == CONST_T || n->right->type == CONST_T)) return makeConst(true);
    if (n->type == AND && (n->left->type == CONST_F || n->right->type == CONST_F)) return makeConst(false);
    return nullptr;
}

// Law 1: p ∧ T ≡ p ,  p ∨ F ≡ p
NodePtr lawIdentity(const NodePtr& n) {
    if (n->type == AND) {
        if (n->right->type == CONST_T) return n->left;
        if (n->left->type == CONST_T) return n->right;
    }
    if (n->type == OR) {
        if (n->right->type == CONST_F) return n->left;
        if (n->left->type == CONST_F) return n->right;
    }
    return nullptr;
}

// Law 3: p ∨ p ≡ p ,  p ∧ p ≡ p
NodePtr lawIdempotent(const NodePtr& n) {
    if (isAndOr(n) && sameTree(n->left, n->right)) return n->left;
    return nullptr;
}

// Law 10: p ∨ (p ∧ q) ≡ p ,  p ∧ (p ∨ q) ≡ p   (and the mirrored orders)
NodePtr lawAbsorption(const NodePtr& n) {
    if (!isAndOr(n)) return nullptr;
    NodeType inner = (n->type == OR) ? AND : OR;
    // p op (p inner q)  or  p op (q inner p)
    if (n->right->type == inner &&
        (sameTree(n->left, n->right->left) || sameTree(n->left, n->right->right)))
        return n->left;
    // (p inner q) op p  or  (q inner p) op p
    if (n->left->type == inner &&
        (sameTree(n->right, n->left->left) || sameTree(n->right, n->left->right)))
        return n->right;
    return nullptr;
}

// Law 8: p ∨ (q ∧ r) ≡ (p ∨ q) ∧ (p ∨ r) ,  p ∧ (q ∨ r) ≡ (p ∧ q) ∨ (p ∧ r)
// To avoid endless expansion it is only used when it creates a pair
// (p with q or r, equal or complement) that a later law can remove.
NodePtr lawDistributive(const NodePtr& n) {
    if (!isAndOr(n)) return nullptr;
    NodeType inner = (n->type == OR) ? AND : OR;
    if (n->right->type == inner &&
        (isPair(n->left, n->right->left) || isPair(n->left, n->right->right)))
        return makeBinary(inner, makeBinary(n->type, n->left, n->right->left),
                          makeBinary(n->type, n->left, n->right->right));
    if (n->left->type == inner &&
        (isPair(n->right, n->left->left) || isPair(n->right, n->left->right)))
        return makeBinary(inner, makeBinary(n->type, n->left->left, n->right),
                          makeBinary(n->type, n->left->right, n->right));
    return nullptr;
}

// Law 7: (p op q) op r ≡ p op (q op r)
// Used only to bring a pair next to each other: (x op y) op z with z paired with y,
// or x op (y op z) with x paired with y.
NodePtr lawAssociative(const NodePtr& n) {
    if (!isAndOr(n)) return nullptr;
    if (n->left->type == n->type && isPair(n->right, n->left->right))
        return makeBinary(n->type, n->left->left, makeBinary(n->type, n->left->right, n->right));
    if (n->right->type == n->type && isPair(n->left, n->right->left))
        return makeBinary(n->type, makeBinary(n->type, n->left, n->right->left), n->right->right);
    return nullptr;
}

// Law 6: p op q ≡ q op p
// Used only to put a paired operand in the position where Associative can reach it.
NodePtr lawCommutative(const NodePtr& n) {
    if (!isAndOr(n)) return nullptr;
    if (n->left->type == n->type && isPair(n->right, n->left->left))     // (x op y) op z, z pairs x
        return makeBinary(n->type, makeBinary(n->type, n->left->right, n->left->left), n->right);
    if (n->right->type == n->type && isPair(n->left, n->right->right))   // x op (y op z), x pairs z
        return makeBinary(n->type, n->left, makeBinary(n->type, n->right->right, n->right->left));
    return nullptr;
}

// Applies 'law' at the first node (pre-order, left to right) where it works.
// Sets 'changed' to true when a rewrite happened.
NodePtr rewriteFirst(const NodePtr& n, LawFunction law, bool& changed) {
    NodePtr replacement = law(n);
    if (replacement) { changed = true; return replacement; }
    if (n->left) {
        NodePtr newLeft = rewriteFirst(n->left, law, changed);
        if (changed) return make_shared<Node>(Node{n->type, n->name, newLeft, n->right});
    }
    if (n->right) {
        NodePtr newRight = rewriteFirst(n->right, law, changed);
        if (changed) return make_shared<Node>(Node{n->type, n->name, n->left, newRight});
    }
    return n;
}

// FIXED PRIORITY ORDER of the automatic simplifier.
// 1-2: Biconditional, Conditional (required to go first)
// then: Double Negation, De Morgan, Negation, Domination, Identity,
//       Idempotent, Absorption, Distributive, Associative, Commutative.
// Contrapositive (law 13) is deliberately absent: after step 2 there is no → left, and
// p → q ⇄ ¬q → ¬p could be applied forever.
struct Rule {
    const char* name;
    LawFunction apply;
};

const vector<Rule>& automaticRules() {
    static const vector<Rule> rules = {
        {"Biconditional Law", lawBiconditional},
        {"Conditional Law", lawConditional},
        {"Double Negation Law", lawDoubleNegation},
        {"De Morgan's Law", lawDeMorgan},
        {"Negation (Complement) Law", lawNegation},
        {"Domination Law", lawDomination},
        {"Identity Law", lawIdentity},
        {"Idempotent Law", lawIdempotent},
        {"Absorption Law", lawAbsorption},
        {"Distributive Law", lawDistributive},
        {"Associative Law", lawAssociative},
        {"Commutative Law", lawCommutative}};
    return rules;
}

// ---------------------------------------------------------------------
// 7. THE SIMPLIFIER
// ---------------------------------------------------------------------
const int MAX_STEPS = 100;   // safety limit so the program always terminates

NodePtr simplify(NodePtr current) {
    set<string> seen;              // second safety net: stop if a form repeats
    seen.insert(show(current));
    int step = 0;

    while (step < MAX_STEPS) {
        bool appliedSomething = false;
        for (const Rule& rule : automaticRules()) {      // highest priority first
            bool changed = false;
            NodePtr next = rewriteFirst(current, rule.apply, changed);
            if (changed) {
                step++;
                current = next;
                cout << "Step " << step << ": " << show(current) << "    [" << rule.name << "]\n";
                appliedSomething = true;
                break;                                   // restart from the top priority
            }
        }
        if (!appliedSomething) break;                    // no law applies: done
        if (!seen.insert(show(current)).second) {
            cout << "(Stopped: this form was already produced before.)\n";
            break;
        }
    }
    if (step >= MAX_STEPS) cout << "(Stopped: safe step limit of " << MAX_STEPS << " reached.)\n";
    if (step == 0) cout << "(No law applies to this equation.)\n";
    return current;
}

// ---------------------------------------------------------------------
// 8. INPUT HELPERS
// ---------------------------------------------------------------------
// Reads one line after printing 'prompt'. When input is piped, the line is
// echoed so the transcript looks like a normal interactive session.
bool readLine(const string& prompt, string& line) {
    cout << prompt << flush;
    if (!getline(cin, line)) return false;
    if (!isatty(0)) cout << line << "\n";
    return true;
}

// ---------------------------------------------------------------------
// 9. PART (a) MENU ACTIONS
// ---------------------------------------------------------------------
void runSimplification() {
    string text;
    if (!readLine("Enter equation: ", text)) return;
    Limits limits = {10, 20, 4};
    NodePtr original;
    try {
        original = parseEquation(text, limits);
    } catch (const runtime_error& e) {
        cout << e.what() << "\n";
        return;
    }
    cout << "Original: " << text << "\n";
    NodePtr result = simplify(original);
    cout << "Final: " << show(result) << "\n";
    bool same = truthTableEquivalent(original, result, true);
    cout << "Truth-table check: " << (same ? "EQUIVALENT" : "NOT EQUIVALENT") << "\n";
}

// Law 13 demo: applies the Contrapositive Law to the first → and then tidies
// double negations. It is separate from the automatic simplifier on purpose.
void runContrapositiveDemo() {
    string text;
    if (!readLine("Enter an equation containing → (no length limits here): ", text)) return;
    Limits limits = {1, 100, 4};
    NodePtr original;
    try {
        original = parseEquation(text, limits);
    } catch (const runtime_error& e) {
        cout << e.what() << "\n";
        return;
    }
    cout << "Original: " << show(original) << "\n";
    bool changed = false;
    NodePtr current = rewriteFirst(original, lawContrapositive, changed);
    if (!changed) { cout << "No → found, so the Contrapositive Law does not apply.\n"; return; }
    int step = 1;
    cout << "Step " << step << ": " << show(current) << "    [Contrapositive Law]\n";
    while (true) {
        changed = false;
        NodePtr next = rewriteFirst(current, lawDoubleNegation, changed);
        if (!changed) break;
        current = next;
        cout << "Step " << ++step << ": " << show(current) << "    [Double Negation Law]\n";
    }
    cout << "Final: " << show(current) << "\n";
    cout << "Truth-table check: "
         << (truthTableEquivalent(original, current, false) ? "EQUIVALENT" : "NOT EQUIVALENT") << "\n";
}

// Tests each of the 13 laws on its own, using a small example for each.
void runLawSelfTests() {
    struct LawTest { const char* name; LawFunction law; const char* example; };
    const LawTest tests[] = {
        {"1  Identity", lawIdentity, "p ∧ T"},
        {"2  Domination", lawDomination, "p ∨ T"},
        {"3  Idempotent", lawIdempotent, "p ∧ p"},
        {"4  Double Negation", lawDoubleNegation, "¬(¬p)"},
        {"5  Negation (Complement)", lawNegation, "p ∨ ¬p"},
        {"6  Commutative", lawCommutative, "(p ∨ q) ∨ p"},
        {"7  Associative", lawAssociative, "(q ∨ p) ∨ p"},
        {"8  Distributive", lawDistributive, "p ∨ (¬p ∧ q)"},
        {"9  De Morgan's", lawDeMorgan, "¬(p ∧ q)"},
        {"10 Absorption", lawAbsorption, "p ∧ (p ∨ q)"},
        {"11 Conditional", lawConditional, "p → q"},
        {"12 Biconditional", lawBiconditional, "p ↔ q"},
        {"13 Contrapositive", lawContrapositive, "p → q"}};
    Limits relaxed = {1, 100, 4};
    for (const LawTest& t : tests) {
        NodePtr before = parseEquation(t.example, relaxed);
        bool changed = false;
        NodePtr after = rewriteFirst(before, t.law, changed);
        cout << "Law " << t.name << ":  " << show(before) << "   =>   " << show(after) << "   ["
             << (changed ? "applied" : "NOT APPLIED") << ", "
             << (truthTableEquivalent(before, after, false) ? "EQUIVALENT" : "NOT EQUIVALENT") << "]\n";
    }
}

// ---------------------------------------------------------------------
// 10. PART (b): ARGUMENT VALIDITY BY TRUTH TABLE
// ---------------------------------------------------------------------
// Sum of the column widths (used for the divider line).
int accumulate_width(const vector<int>& widths) {
    int total = 0;
    for (int w : widths) total += w;
    return total;
}

// Tests premises => conclusion. Prints Valid + critical rows, or False + counterexample.
void checkArgument(const vector<string>& premiseTexts, const string& conclusionText, bool& validOut) {
    Limits limits = {1, 100, 5};
    vector<NodePtr> premises;
    NodePtr conclusion;
    try {
        for (const string& text : premiseTexts) premises.push_back(parseEquation(text, limits));
        conclusion = parseEquation(conclusionText, limits);
    } catch (const runtime_error& e) {
        cout << e.what() << "\n";
        validOut = false;
        return;
    }

    set<char> varSet;
    for (const NodePtr& p : premises) collectVariables(p, varSet);
    collectVariables(conclusion, varSet);
    if (varSet.size() > 5) {
        cout << "Rejected: more than 5 distinct variables in the argument.\n";
        validOut = false;
        return;
    }
    vector<char> vars(varSet.begin(), varSet.end());

    // Column headings.
    vector<string> headings;
    for (char v : vars) headings.push_back(string(1, v));
    for (const NodePtr& p : premises) headings.push_back(show(p));
    headings.push_back(show(conclusion));
    vector<int> widths;
    for (const string& h : headings) widths.push_back(max(displayLength(h), 1) + 2);

    cout << "\nPremises:\n";
    for (size_t i = 0; i < premises.size(); i++) cout << "  P" << (i + 1) << ": " << show(premises[i]) << "\n";
    cout << "Conclusion: " << show(conclusion) << "\n\n";

    cout << " ";
    for (size_t c = 0; c < headings.size(); c++) cout << padRight(headings[c], widths[c]);
    cout << "\n " << string(accumulate_width(widths), '-') << "\n";

    int rows = 1 << vars.size();
    int criticalCount = 0;
    int firstCounterexample = -1;
    int counterexampleCount = 0;
    vector<string> rowTexts;

    for (int row = 0; row < rows; row++) {
        map<char, bool> values = assignmentForRow(vars, row);
        vector<bool> cells;
        for (char v : vars) cells.push_back(values[v]);
        bool allPremisesTrue = true;
        for (const NodePtr& p : premises) {
            bool value = evaluate(p, values);
            cells.push_back(value);
            if (!value) allPremisesTrue = false;
        }
        bool conclusionValue = evaluate(conclusion, values);
        cells.push_back(conclusionValue);

        string marker;
        if (allPremisesTrue) {
            criticalCount++;
            marker = "  <-- CRITICAL ROW";
            if (!conclusionValue) {
                counterexampleCount++;
                marker = "  <-- CRITICAL ROW, COUNTEREXAMPLE (conclusion is false)";
                if (firstCounterexample < 0) firstCounterexample = row + 1;
            }
        }
        string line = " ";
        for (size_t c = 0; c < cells.size(); c++) line += padRight(tf(cells[c]), widths[c]);
        cout << line << marker << "\n";
    }

    cout << "\n";
    validOut = (counterexampleCount == 0);
    if (validOut) {
        cout << "Valid\n";
        cout << "Critical rows (all premises true): " << criticalCount
             << ". The conclusion is true in every one of them, so no counterexample exists.\n";
    } else {
        cout << "False\nInvalid argument.\n";
        cout << "Counterexample: row " << firstCounterexample
             << " (marked above) makes every premise true and the conclusion false.\n";
    }
}

void runManualArgument() {
    string line;
    if (!readLine("How many premises (1-5)? ", line)) return;
    int count = 0;
    try { count = stoi(line); } catch (...) { count = 0; }
    if (count < 1 || count > 5) { cout << "Rejected: enter a number from 1 to 5.\n"; return; }
    vector<string> premises(count);
    for (int i = 0; i < count; i++)
        if (!readLine("Premise " + to_string(i + 1) + ": ", premises[i])) return;
    string conclusion;
    if (!readLine("Conclusion: ", conclusion)) return;
    bool valid;
    checkArgument(premises, conclusion, valid);
}

struct ArgumentCase {
    string title;
    vector<string> premises;
    string conclusion;
    bool expectedValid;
};

void runCases(const vector<ArgumentCase>& cases) {
    int matches = 0;
    for (const ArgumentCase& c : cases) {
        cout << "\n==================== " << c.title << " ====================";
        bool valid = false;
        checkArgument(c.premises, c.conclusion, valid);
        cout << "Expected: " << (c.expectedValid ? "Valid" : "Invalid") << "  |  Program says: "
             << (valid ? "Valid" : "Invalid") << "  ->  " << (valid == c.expectedValid ? "MATCH" : "MISMATCH") << "\n";
        if (valid == c.expectedValid) matches++;
    }
    cout << "\nSummary: " << matches << " of " << cases.size() << " results match the expected answer.\n";
}

void runMandatoryTests() {
    runCases({
        {"Test 1", {"p → q", "p"}, "q", true},
        {"Test 2", {"p → q", "q → r"}, "p → r", true},
        {"Test 3", {"p ∨ q", "¬p"}, "q", true},
        {"Test 4", {"p → q", "q"}, "p", false},
        {"Test 5", {"p → q", "¬p"}, "¬q", false},
        {"Test 6", {"(p → q) ∧ (r → s)", "p ∨ r"}, "q ∨ s", true}});
}

void runAllNineRules() {
    runCases({
        {"Rule: Modus Ponens", {"p → q", "p"}, "q", true},
        {"Rule: Modus Tollens", {"p → q", "¬q"}, "¬p", true},
        {"Rule: Hypothetical Syllogism", {"p → q", "q → r"}, "p → r", true},
        {"Rule: Disjunctive Syllogism", {"p ∨ q", "¬p"}, "q", true},
        {"Rule: Addition", {"p"}, "p ∨ q", true},
        {"Rule: Simplification", {"p ∧ q"}, "p", true},
        {"Rule: Conjunction", {"p", "q"}, "p ∧ q", true},
        {"Rule: Resolution", {"p ∨ q", "¬p ∨ r"}, "q ∨ r", true},
        {"Rule: Constructive Dilemma", {"(p → q) ∧ (r → s)", "p ∨ r"}, "q ∨ s", true}});
}

void runOriginalArgument() {
    cout << "\nMy own argument: if it rains (p) then I take an umbrella (q) or a taxi (r);\n"
            "it rains; I do not take an umbrella; therefore I take a taxi.\n";
    runCases({{"Original argument", {"p → (q ∨ r)", "p", "¬q"}, "r", true}});
    cout << "Why valid: the only critical row is p=T, q=F, r=T. p is true, so p → (q ∨ r) forces\n"
            "q ∨ r to be true; q is false, so r must be true. The conclusion r is true there.\n";
}

// ---------------------------------------------------------------------
// 11. MAIN MENU
// ---------------------------------------------------------------------
int main() {
    while (true) {
        cout << "\n=========== Question 1: Propositional Logic ===========\n"
                "1. Simplify an equation (Part a)\n"
                "2. Contrapositive Law demo (law 13, Part a)\n"
                "3. Test each of the 13 laws separately (Part a)\n"
                "4. Test an argument you type in (Part b)\n"
                "5. Run the 6 mandatory test cases (Part b)\n"
                "6. Test all 9 rules of inference (Part b)\n"
                "7. Run my original argument (Part b)\n"
                "8. Exit\n";
        string choice;
        if (!readLine("Choice: ", choice)) break;
        if (choice == "1") runSimplification();
        else if (choice == "2") runContrapositiveDemo();
        else if (choice == "3") runLawSelfTests();
        else if (choice == "4") runManualArgument();
        else if (choice == "5") runMandatoryTests();
        else if (choice == "6") runAllNineRules();
        else if (choice == "7") runOriginalArgument();
        else if (choice == "8") break;
        else cout << "Please enter a number from 1 to 8.\n";
    }
    return 0;
}
