#pragma once

#include <string>
#include <vector>

class Term {
public:
  
    std::string functor;
    std::vector<Term*> args;
  
};
