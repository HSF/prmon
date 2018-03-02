// Catch setup with supplied main()
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// Header for interface we want to test
#include "prmon/Example.h"

// Define Catch test case
// See
//   https://github.com/philsquared/Catch/blob/master/docs/tutorial.md
// and
//   https://github.com/philsquared/Catch/blob/master/docs/Readme.md
// for additional docs on Catch
TEST_CASE( "Example behaves correctly", "[interface]" ) {
  auto e = prmon::Example();
  REQUIRE( e.get() == 0 );

  SECTION( "Copy assignment works" ) {
    e = prmon::Example(10);
    REQUIRE( e.get() == 10 );
  }

  SECTION( "Setting/Getting interface works" ) {
    e.set(12);
    REQUIRE( e.get() == 12 );
  }
}

