#include "node.hh"
#include "term.hh"
#include "tree.hh"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

Term* applyDoubleNegation(Term* t) {
  
    if (!t) return nullptr;

    for (auto& child : t->args) {
        child = applyDoubleNegation(child);
    }

    if (t->functor == "not" && !t->args.empty() && t->args[0]->functor == "not") {
        Term* inner = t->args[0]->args[0];
        t->args[0]->args.clear();
        delete t->args[0];
        t->args.clear();
        t->functor = inner->functor;
        t->args = inner->args;
        delete inner;
    }

    return t;
}


std::string popOuterOperator(std::string& s) {
  
    while (!s.empty() && isspace(s.front())) s.erase(s.begin());
    while (!s.empty() && isspace(s.back())) s.pop_back();

    size_t paren = s.find('(');
    if (paren == std::string::npos) return "";

    std::string op = s.substr(0, paren);

    int depth = 0;
    size_t i = paren;
    for (; i < s.size(); ++i) {
        if (s[i] == '(') depth++;
        else if (s[i] == ')') {
            depth--;
            if (depth == 0) break;
        }
    }

    if (i >= s.size()) return "";

    std::string inner = s.substr(paren + 1, i - paren - 1);
    s = inner;
    return op;
}

std::vector<std::string> splitArguments(const std::string& s) {
  
    std::vector<std::string> args;
    int depth = 0;
    size_t start = 0;
    
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '(') depth++;
        else if (s[i] == ')') depth--;
        else if (isspace(s[i]) && depth == 0) {
            if (i > start) args.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    
    if (start < s.size()) args.push_back(s.substr(start));
    return args;
}

Term* buildTerm(std::string s) {

    while (!s.empty() && isspace(s.front())) s.erase(s.begin());
    while (!s.empty() && isspace(s.back())) s.pop_back();
    if (s.empty()) return nullptr;

    std::string op = popOuterOperator(s);
    if (!op.empty()) {
        Term* t = new Term();
        t->functor = op;
	
        std::vector<std::string> subargs = splitArguments(s);
        for (auto& a : subargs) {
            t->args.push_back(buildTerm(a));
        }
        return t;
    } else {
        Term* t = new Term();
        t->functor = s;
        return t;
    }
    
}

void printTerm(Term* t, int indent = 0) {
    if (!t) return;
    std::cout << std::string(indent, ' ') << t->functor << "\n";
    for (auto a : t->args) printTerm(a, indent + 2);
}

int main() {

  std::string toload;
  std::cout << "please enter formula.\nPossible commands: and(), or(), "
               "variables(1,2,3...n), not(), ->, <->, brackets( ( ) ) "
               "\nDelimit with space!"
            << std::endl;
  std::getline(std::cin, toload);
  std::cout << "parsing input: " << toload << std::endl;

  Term* toparse = buildTerm(toload);
  
  std::cout << "Parsed term tree:\n";
  printTerm(toparse);

  applyDoubleNegation(toparse); // fuck you nodiscard xd

  std::cout << "applied rules:\n";
  printTerm(toparse);
  
  delete toparse;

  return 0;
  
}
