// SPDX-License-Identifier: MIT
// Dummy source for nova::parameter

#include <nova/parameter/parameter.hpp>

namespace nova::parameter {

// trivial implementation placeholder
int get_default_value()
{
  return Parameter{}.value;
}

} // namespace nova::parameter
