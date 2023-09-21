#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>

#include "ana/analogical.h"
#include "ana/entry_count.h"
#include "ana/sum_of_weights.h"
#include "ana/trivial_input.h"

using ana::multithread;
template <typename T> using dataflow = ana::dataflow<T>;
using cut = ana::selection::cut;
using weight = ana::selection::weight;

TEST_CASE("correctness & consistency of selections") {

  auto nentries = 100;

  // generate random data
  trivial_data_t random_data;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::poisson_distribution random_weight(1);
  std::uniform_int_distribution random_category(1, 3);
  std::unordered_map<int, std::string> category_name{
      {1, "a"}, {2, "b"}, {3, "c"}};

  for (unsigned int i = 0; i < nentries; ++i) {
    auto w = random_weight(gen);
    auto c = category_name[random_category(gen)];
    random_data.emplace_back(
        trivial_row_t{{"index", i}, {"category", c}, {"weight", w}});
  }

  // compute correct answer
  long long correct_count_a = 0;
  long long correct_count_b = 0;
  long long correct_count_c = 0;
  for (unsigned int i = 0; i < nentries; ++i) {
    auto c = std::get<std::string>(random_data[i]["category"]);
    if (c == "a") {
      ++correct_count_a;
    } else if (c == "b") {
      ++correct_count_b;
    } else {
      ++correct_count_c;
    }
  }

  // compute answer with analogical
  ana::multithread::disable();
  auto df = ana::dataflow<trivial_input>(random_data);
  auto entry_category = df.read<std::string>("category");
  auto cut_a =
      df.filter<cut>("cut_a")(entry_category == df.constant<std::string>("a"));
  auto cut_b =
      df.filter<cut>("cut_b")(entry_category == df.constant<std::string>("b"));
  auto cut_c =
      df.filter<cut>("cut_c")(entry_category == df.constant<std::string>("c"));

  auto cut_ab = cut_a || cut_b;
  auto cut_bc = cut_b || cut_c;
  auto cut_abc = cut_a || cut_b || cut_c;

  auto cut_none = cut_a && cut_b && cut_c;
  auto cut_a2 = cut_a && cut_ab;
  auto cut_b2 = cut_b && cut_bc;

  auto count_a = df.book<entry_count>().at(cut_a);
  auto count_b = df.book<entry_count>().at(cut_b);
  auto count_c = df.book<entry_count>().at(cut_c);

  auto count_ab = df.book<entry_count>().at(cut_ab);
  auto count_bc = df.book<entry_count>().at(cut_bc);
  auto count_abc = df.book<entry_count>().at(cut_abc);

  auto count_a2 = df.book<entry_count>().at(cut_a2);
  auto count_b2 = df.book<entry_count>().at(cut_b2);
  auto count_none = df.book<entry_count>().at(cut_none);

  SUBCASE("basic results") {
    CHECK(count_a.result() == correct_count_a);
    CHECK(count_b.result() == correct_count_b);
    CHECK(count_c.result() == correct_count_c);
  }

  SUBCASE("binary operations") {
    CHECK(count_ab.result() == correct_count_a + correct_count_b);
    CHECK(count_bc.result() == correct_count_b + correct_count_c);
    CHECK(count_abc.result() ==
          correct_count_a + correct_count_b + correct_count_c);

    CHECK(count_a2.result() == correct_count_a);
    CHECK(count_b2.result() == correct_count_b);
    CHECK(count_none.result() == 0);
  }
}