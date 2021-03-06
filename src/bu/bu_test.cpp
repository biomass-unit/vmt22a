#include "bu/utilities.hpp"
#include "bu/safe_integer.hpp"
#include "bu/flatmap.hpp"

#include "tests/tests.hpp"


namespace {

    auto run_bu_tests() -> void {
        using namespace tests;

        "vector"_test = [] {
            auto vector = bu::vector_with_capacity<int>(10);

            assert_eq(vector.size(), 0_uz);
            assert_eq(vector.capacity(), 10_uz);

            bu::release_vector_memory(vector);

            assert_eq(vector.size(), 0_uz);
            assert_eq(vector.capacity(), 0_uz);
        };
    }


    auto run_safe_integer_tests() -> void {
        using namespace tests;
        
        bu::Safe_integer<int> integer;

        "default_constructor"_test = [&] {
            assert_eq(integer, 0);
            assert_eq(static_cast<bool>(integer), false);
        };

        "arithmetic"_test = [&] {
            integer += 5;

            assert_eq(integer, 5);
            assert_eq(static_cast<bool>(integer), true);

            (void)(integer + 5);

            assert_eq(integer, 5);
        };

        "division_by_zero"_throwing_test = [&] {
            (void)(integer / 0);
        };

        "out_of_bounds_increment"_throwing_test = [&] {
            integer = std::numeric_limits<int>::max();
            ++integer;
        };

        "out_of_bounds_decrement"_throwing_test = [&] {
            integer = std::numeric_limits<int>::min();
            --integer;
        };
    }


    auto run_flatmap_tests() -> void {
        using namespace tests;

        bu::Flatmap<int, int> flatmap;

        "add"_test = [&] {
            flatmap.add(10, 20);

            assert_eq(flatmap.size(), 1_uz);
            assert_eq(static_cast<bool>(flatmap.find(10)), true);
            assert_eq(*flatmap.find(10), 20);

            flatmap.add(10, 30);

            assert_eq(flatmap.size(), 2_uz);
            assert_eq(*flatmap.find(10), 20);
        };
    }

}


REGISTER_TEST(run_bu_tests);
REGISTER_TEST(run_safe_integer_tests);
REGISTER_TEST(run_flatmap_tests);