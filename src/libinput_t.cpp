#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include "libinput.hpp"

TEST_CASE("transformation_matrix") {
  const float xs = 0.1;
  const float ys = 0.2;
  const float x = 0.3;
  const float y = 0.4;
  REQUIRE(li::transformation_matrix::translate(x, y) *
              li::transformation_matrix::scale(xs, ys) ==
          li::transformation_matrix{xs, 0, x, 0, ys, y});
}
