#pragma once

#include <cassert>

#define SYNERA_EXPECTS(expr) assert((expr) && "Precondition failed: " #expr)
#define SYNERA_ENSURES(expr) assert((expr) && "Postcondition failed: " #expr)
#define SYNERA_INVARIANT(expr) assert((expr) && "Invariant failed: " #expr)
