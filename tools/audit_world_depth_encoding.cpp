// CPU expression diagnostic only. No GPU fixed-point conversion is modeled.
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

constexpr std::uint32_t maximum = 0xffffff;
struct Stats {
    std::uint64_t count{}, word_differences{}, reciprocal_word_differences{}, source_floor_errors{}, source_nearest_errors{};
    std::uint64_t division_floor_errors{}, division_nearest_errors{}, reciprocal_floor_errors{}, reciprocal_nearest_errors{};
    std::uint32_t maximum_word_distance{}, first_word_difference = 0xffffffff;
    void add(std::uint32_t n) {
        if (n > maximum) throw std::runtime_error("input is not a D24 integer");
        const float source = std::bit_cast<float>(std::bit_cast<std::uint32_t>(float(n) / 16777216.f) + 1u);
        // Correctly rounded division via exact binary64 integer operands.
        const float division = float(double(n) / double(maximum));
        // A separate diagnostic for constant-reciprocal lowering, not a claim
        // that the current driver's compiled world shader selects this form.
        volatile const float reciprocal = float(1.0 / double(maximum));
        const float product = float(n) * reciprocal;
        const auto a = std::bit_cast<std::uint32_t>(source), b = std::bit_cast<std::uint32_t>(division);
        ++count;
        reciprocal_word_differences += a != std::bit_cast<std::uint32_t>(product);
        if (a != b) {
            ++word_differences;
            if (first_word_difference == 0xffffffff) first_word_difference = n;
            maximum_word_distance = std::max(maximum_word_distance, a > b ? a-b : b-a);
        }
        const auto floor_value = [](float x) { return std::uint32_t(std::floor(std::clamp(double(x), 0., 1.) * maximum)); };
        const auto nearest_value = [](float x) { return std::uint32_t(std::floor(std::clamp(double(x), 0., 1.) * maximum + .5)); };
        source_floor_errors += floor_value(source) != n;
        source_nearest_errors += nearest_value(source) != n;
        division_floor_errors += floor_value(division) != n;
        division_nearest_errors += nearest_value(division) != n;
        reciprocal_floor_errors += floor_value(product) != n;
        reciprocal_nearest_errors += nearest_value(product) != n;
    }
    void print() const {
        std::cout << "{\"count\":" << count << ",\"source_vs_rounded_division_float_words\":" << word_differences
            << ",\"source_vs_rounded_reciprocal_float_words\":" << reciprocal_word_differences
            << ",\"first_difference_integer\":" << first_word_difference << ",\"maximum_float_word_distance\":" << maximum_word_distance
            << ",\"ideal_real_product_floor_errors\":{\"source\":" << source_floor_errors << ",\"rounded_division\":" << division_floor_errors
            << ",\"rounded_reciprocal_multiply\":" << reciprocal_floor_errors << "},\"ideal_real_product_nearest_errors\":{\"source\":" << source_nearest_errors
            << ",\"rounded_division\":" << division_nearest_errors << ",\"rounded_reciprocal_multiply\":" << reciprocal_nearest_errors << "}}";
    }
};
int main(int argc, char **argv) try {
    static_assert(std::endian::native == std::endian::little);
    if (argc != 2) throw std::runtime_error("expected original GL packed D24S8 file");
    Stats all, observed;
    for (std::uint32_t n = 0; n <= maximum; ++n) all.add(n);
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) throw std::runtime_error("cannot read original D24 file");
    std::uint32_t word{};
    while (input.read(reinterpret_cast<char *>(&word), 4)) observed.add(word >> 8);
    if (!input.eof() || input.gcount()) throw std::runtime_error("partial original depth word");
    std::cout << "{\"complete_integer_domain\":"; all.print();
    std::cout << ",\"actual_captured_depth_integers_as_expression_inputs\":"; observed.print();
    std::cout << "}\n";
} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
