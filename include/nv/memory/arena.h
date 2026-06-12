#pragma once

#include "nv/core/core_types.h"
#include "nv/core/intdefs.h"
struct VMem;

struct ABlock {
  struct VMem* mem; 
};
alias(ABlock);

struct Arena {
  struct VMem* mem;
  byte* cursor;
};



