#include "foundrules.hh"
#include "node.hh"
#include "term.hh"
#include "tree.hh"
#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

Term *applyDoubleNegation(Term *t) {

  if (!t)
    return nullptr;

  for (auto &child : t->args) {
    child = applyDoubleNegation(child);
  }

  if (t->functor == "not" && !t->args.empty() && t->args[0]->functor == "not") {
    Term *inner = t->args[0]->args[0];
    t->args[0]->args.clear();
    delete t->args[0];
    t->args.clear();
    t->functor = inner->functor;
    t->args = inner->args;
    delete inner;
  }

  return t;
}

Term *applyImplication(Term *t) {
  if (!t)
    return nullptr;

  for (auto &child : t->args) {
    child = applyImplication(child);
  }

  if (t->functor == "impl" && !t->args.empty() && t->args[0] != nullptr &&
      t->args[1] != nullptr) {
    Term *p = t->args[0];
    Term *q = t->args[1];

    Term *new_p = new Term();
    new_p->functor = "not";
    new_p->args.push_back(p);

    t->functor = "or";
    t->args[0] = new_p;
    t->args[1] = q;
  }

  return t;
}

Term *applyDemoorganOr(Term *t) {

  if (!t)
    return nullptr;

  for (auto &child : t->args) {
    child = applyDemoorganOr(child);
  }

  if (t->functor == "not" && !t->args.empty() && t->args[0]->functor == "or" &&
      t->args[0]->args[0] != nullptr && t->args[0]->args[1] != nullptr) {
    Term *p = t->args[0]->args[0];
    Term *q = t->args[0]->args[1];

    Term *new_p = new Term();
    new_p->functor = "not";
    new_p->args.push_back(p);

    Term *new_q = new Term();
    new_q->functor = "not";
    new_q->args.push_back(q);

    t->functor = "and";
    t->args[0] = new_p;
    t->args.push_back(new_q);
  }

  return t;
}

Term *applyDemoorganAnd(Term *t) {

  if (!t)
    return nullptr;

  for (auto &child : t->args) {
    child = applyDemoorganAnd(child);
  }

  if (t->functor == "not" && !t->args.empty() && t->args[0]->functor == "and" &&
      t->args[0]->args[0] != nullptr && t->args[0]->args[1] != nullptr) {
    Term *p = t->args[0]->args[0];
    Term *q = t->args[0]->args[1];

    Term *new_p = new Term();
    new_p->functor = "not";
    new_p->args.push_back(p);

    Term *new_q = new Term();
    new_q->functor = "not";
    new_q->args.push_back(q);

    t->functor = "or";
    t->args[0] = new_p;
    t->args.push_back(new_q);
  }

  return t;
}

Term *applyDistributivity(Term *t) {
  if (!t)
    return nullptr;

  for (auto &child : t->args) {
    child = applyDistributivity(child);
  }

  // only or
  if (t->functor == "or" && t->args.size() == 2) {
    Term *A = t->args[0];
    Term *B = t->args[1];

    // fuck this
    if (B->functor == "and" && B->args.size() == 2) {
      Term *Q = B->args[0];
      Term *R = B->args[1];

      Term *or1 = new Term();
      or1->functor = "or";
      or1->args = {A, Q};

      Term *or2 = new Term();
      or2->functor = "or";
      or2->args = {A, R};

      t->functor = "and";
      t->args = {applyDistributivity(or1), applyDistributivity(or2)};
    }

    else if (A->functor == "and" && A->args.size() == 2) {
      Term *Q = A->args[0];
      Term *R = A->args[1];

      Term *or1 = new Term();
      or1->functor = "or";
      or1->args = {Q, B};

      Term *or2 = new Term();
      or2->functor = "or";
      or2->args = {R, B};

      t->functor = "and";
      t->args = {applyDistributivity(or1), applyDistributivity(or2)};
    }
  }

  return t;
}

std::string popOuterOperator(std::string &s) {

  while (!s.empty() && isspace(s.front()))
    s.erase(s.begin());
  while (!s.empty() && isspace(s.back()))
    s.pop_back();

  size_t paren = s.find('(');
  if (paren == std::string::npos)
    return "";

  std::string op = s.substr(0, paren);

  int depth = 0;
  size_t i = paren;
  for (; i < s.size(); ++i) {
    if (s[i] == '(')
      depth++;
    else if (s[i] == ')') {
      depth--;
      if (depth == 0)
        break;
    }
  }

  if (i >= s.size())
    return "";

  std::string inner = s.substr(paren + 1, i - paren - 1);
  s = inner;
  return op;
}

std::vector<std::string> splitArguments(const std::string &s) {

  std::vector<std::string> args;
  int depth = 0;
  size_t start = 0;

  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '(')
      depth++;
    else if (s[i] == ')')
      depth--;
    else if (isspace(s[i]) && depth == 0) {
      if (i > start)
        args.push_back(s.substr(start, i - start));
      start = i + 1;
    }
  }

  if (start < s.size())
    args.push_back(s.substr(start));
  return args;
}

Term *buildTerm(std::string s) {

  while (!s.empty() && isspace(s.front()))
    s.erase(s.begin());
  while (!s.empty() && isspace(s.back()))
    s.pop_back();
  if (s.empty())
    return nullptr;

  std::string op = popOuterOperator(s);
  if (!op.empty()) {
    Term *t = new Term();
    t->functor = op;

    std::vector<std::string> subargs = splitArguments(s);
    for (auto &a : subargs) {
      t->args.push_back(buildTerm(a));
    }
    return t;
  } else {
    Term *t = new Term();
    t->functor = s;
    return t;
  }
}

foundrules detectApplicableRules(Term *t) {
  foundrules rules;

  if (!t) {
    return rules;
  }

  std::function<void(const Term *)> traverse = [&](const Term *node) {
    if (!node)
      return;

    // notnot
    if (node->functor == "not" && !node->args.empty() && node->args[0] &&
        node->args[0]->functor == "not" && !node->args[0]->args.empty()) {
      rules.found_not_not_resolve = true;
    }

    // impl
    if (node->functor == "impl" && node->args.size() == 2) {
      if (node->args[0] && node->args[1]) {
        rules.found_impl_resolve = true;
      }
    }

    // DM and
    if (node->functor == "not" && !node->args.empty() && node->args[0] &&
        node->args[0]->functor == "and" && node->args[0]->args.size() == 2) {
      if (node->args[0]->args[0] && node->args[0]->args[1]) {
        rules.found_demoorgan_and = true;
      }
    }

    // DM or
    if (node->functor == "not" && !node->args.empty() && node->args[0] &&
        node->args[0]->functor == "or" && node->args[0]->args.size() == 2) {
      if (node->args[0]->args[0] && node->args[0]->args[1]) {
        rules.found_demoorgan_or = true;
      }
    }

    // distributivity
    if (node->functor == "or" && node->args.size() == 2) {
      if ((node->args[0] && node->args[0]->functor == "and") ||
          (node->args[1] && node->args[1]->functor == "and")) {
        rules.found_distributivity = true;
      }
    }

    // Recurse into children
    for (const auto &child : node->args) {
      traverse(child);
    }
  };

  traverse(t);
  return rules;
}

void printTerm(Term *t, int indent = 0) {
  if (!t)
    return;
  std::cout << std::string(indent, ' ') << t->functor << "\n";
  for (auto a : t->args)
    printTerm(a, indent + 2);
}

int main() {

  std::string toload;
  std::cout << "please enter formula.\nPossible commands: and(), or(), "
               "variables(1,2,3...n), not(), impl(), biimpl(), brackets( ( ) ) "
               "\nDelimit with space!"
            << std::endl;
  std::getline(std::cin, toload);
  std::cout << "parsing input: " << toload << std::endl;

  Term *toparse = buildTerm(toload);

  std::cout << "Applying rules to term tree:\n";
  printTerm(toparse);

  foundrules foundrules = detectApplicableRules(toparse);

  while ((foundrules.found_demoorgan_and || foundrules.found_demoorgan_or ||
          foundrules.found_impl_resolve || foundrules.found_not_not_resolve ||
          foundrules.found_distributivity)) {

    if (foundrules.found_demoorgan_and) {
      applyDemoorganAnd(toparse);
      std::cout << "applied demoorgan and" << std::endl;
      printTerm(toparse);
    }

    if (foundrules.found_demoorgan_or) {
      applyDemoorganOr(toparse);
      std::cout << "applied demoorgan or" << std::endl;
      printTerm(toparse);
    }

    if (foundrules.found_impl_resolve) {
      applyImplication(toparse);
      std::cout << "applied impl" << std::endl;
      printTerm(toparse);
    }

    if (foundrules.found_not_not_resolve) {
      applyDoubleNegation(toparse);
      std::cout << "applied not not" << std::endl;
      printTerm(toparse);
    }

    if (foundrules.found_distributivity) {
      applyDistributivity(toparse);
      std::cout << "applied distributivity" << std::endl;
      printTerm(toparse);
    }

    foundrules = {false, false, false, false, false};
    foundrules = detectApplicableRules(toparse);

    // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  delete toparse;

  return 0;
}
