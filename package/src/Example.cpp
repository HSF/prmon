#include "prmon/Example.h"

namespace prmon {

  Example::Example() :
    m_number(0)
  {}

  Example::Example(int number) :
    m_number(number)
  {}

  Example::~Example()
  {}

  int Example::get() const {
    return m_number;
  }

  void Example::set(int number) {
    m_number = number;
  }

} // namespace
