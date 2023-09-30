#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>

#include "ana/analogical.h"

#include "ana/json.h"
#include "ana/sumw.h"

template <typename T> using dataflow = ana::dataflow<T>;
using cut = ana::selection::cut;
using weight = ana::selection::weight;

TEST_CASE("correctness & consistency of selections") {

  // generate random data
  nlohmann::json random_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::exponential_distribution<double> random_value(1 / 1.0);
  std::poisson_distribution<unsigned int> random_weight(1);
  std::uniform_int_distribution random_category(1, 3);
  unsigned int nentries = 100;
  for (unsigned int i = 0; i < nentries; ++i) {
    auto x = random_value(gen);
    auto w = random_weight(gen);
    auto c = random_category(gen);
    random_data.emplace_back<nlohmann::json>(
        {{"i", i},
         {"x", x},
         {"w", w},
         {"c", (c == 1) ? ("a") : (c == 2 ? "b" : "c")}});
  }

  // compute correct answer
  int correct_sumw_a = 0;
  int correct_sumw_b = 0;
  int correct_sumw_c = 0;
  for (unsigned int i = 0; i < nentries; ++i) {
    auto c = random_data[i]["c"].template get<std::string>();
    auto w = random_data[i]["w"].template get<int>();
    if (c == "a") {
      correct_sumw_a += w;
    } else if (c == "b") {
      correct_sumw_b += w;
    } else {
      correct_sumw_c += w;
    }
  }

  // compute answer with analogical
  auto df = ana::dataflow(ana::json(random_data));
  auto category = df.read<std::string>("c");

  auto raw_entries = df.filter("raw")(df.constant(true));
  auto weighted_entries = df.weight("weight")(df.read<int>("w"));

  auto cut_a =
      weighted_entries.filter("a")(category == df.constant<std::string>("a"));
  auto cut_b =
      weighted_entries.filter("b")(category == df.constant<std::string>("b"));
  auto cut_c =
      weighted_entries.filter("c")(category == df.constant<std::string>("c"));

  auto cut_ab = weighted_entries.filter("ab")(cut_a || cut_b);
  auto cut_bc = weighted_entries.filter("bc")(cut_b || cut_c);
  auto cut_abc = weighted_entries.filter("abc")(cut_a || cut_b || cut_c);

  auto cut_none = weighted_entries.filter("none")(cut_a && cut_b && cut_c);
  auto cut_a2 = weighted_entries.filter("a2")(cut_ab && cut_a);
  auto cut_b2 = weighted_entries.filter("b2")(cut_ab && cut_b);

  auto sumw_a = df.book<sumw>().at(cut_a);
  auto sumw_one = df.book<sumw>().at(cut_a, cut_b, cut_c);
  auto sumw_two = df.book<sumw>().at(cut_ab, cut_bc);
  auto sumw_three = df.book<sumw>().at(cut_none, cut_abc);

  auto sumw_one2 = df.book<sumw>().at(cut_a2, cut_b2);

  SUBCASE("booking consistency") {
    CHECK(sumw_one["a"].result() == sumw_a.result());
  }

  SUBCASE("basic results") {
    CHECK(sumw_one["a"].result() == correct_sumw_a);
    CHECK(sumw_one["b"].result() == correct_sumw_b);
    CHECK(sumw_one["c"].result() == correct_sumw_c);
  }

  // SUBCASE("commposite filters") {
  //   CHECK(sumw_two["ab"].result() == correct_sumw_a + correct_sumw_b);
  //   CHECK(sumw_two["bc"].result() == correct_sumw_b + correct_sumw_c);
  //   CHECK(sumw_three["abc"].result() ==
  //         correct_sumw_a + correct_sumw_b + correct_sumw_c);

  //   CHECK(sumw_one2["a2"].result() == correct_sumw_a);
  //   CHECK(sumw_one2["b2"].result() == correct_sumw_b);
  //   CHECK(sumw_three["none"].result() == 0);
  // }
}