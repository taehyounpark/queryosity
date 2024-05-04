#include <queryosity/json.hpp>
#include <nlohmann/json.hpp>

using json = qty::json;

#include <queryosity.hpp>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace selection = qty::selection;
namespace query = qty::query;
namespace systematic = qty::systematic;

#include <random>
#include <unordered_map>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("correctness & consistency of selections") {

  // generate random data
  nlohmann::json test_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::exponential_distribution<double> random_value(1 / 1.0);
  std::poisson_distribution<unsigned int> random_weight(1);
  std::uniform_int_distribution random_category(1, 3);
  unsigned int nentries = 100;
  for (unsigned int i = 0; i < nentries; ++i) {
    auto x = random_value(gen);
    auto w = random_weight(gen);
    auto cat = random_category(gen);
    test_data.emplace_back<nlohmann::json>(
        {{"i", i},
         {"x", x},
         {"w", w},
         {"c", (cat == 1) ? ("a") : (cat == 2 ? "b" : "c")}});
  }

  // compute correct answer
  int correct_sumw_a = 0;
  int correct_sumw_b = 0;
  int correct_sumw_c = 0;
  int correct_sumw_abc = 0;
  for (unsigned int i = 0; i < nentries; ++i) {
    auto cat = test_data[i]["c"].template get<std::string>();
    auto w = test_data[i]["w"].template get<int>();
    if (cat == "a") {
      correct_sumw_a += w;
    } else if (cat == "b") {
      correct_sumw_b += w;
    } else {
      correct_sumw_c += w;
    }
    correct_sumw_abc += w;
  }

  // compute answer with queryosity
  dataflow df(multithread::disable());

  auto [cat, w] = df.read(dataset::input<json>(test_data),
                          dataset::column<std::string>("c"),
                          dataset::column<unsigned int>("w"));

  auto weighted = df.weight(w);

  auto a = df.define(column::constant<std::string>("a"));
  auto b = df.define(column::constant<std::string>("b"));
  auto c = df.define(column::constant<std::string>("c"));

  auto cut_a = weighted.filter(cat == a);
  auto cut_b = weighted.filter(cat == b);
  auto cut_c = weighted.filter(cat == c);

  auto cut_ab = weighted.filter(cut_a || cut_b);
  auto cut_bc = weighted.filter(cut_b || cut_c);
  auto cut_abc = weighted.filter(cut_a || cut_b || cut_c);

  auto cut_none = weighted.filter(cut_a && cut_b && cut_c);
  auto cut_a2 = weighted.filter(cut_ab && cut_a);
  auto cut_b2 = weighted.filter(cut_ab && cut_b);

  auto [sumw_a, sumw_b, sumw_c] = df.get(selection::yield(cut_a, cut_b, cut_c));
  auto [sumw_ab, sumw_bc] = df.get(selection::yield(cut_ab, cut_bc));
  auto [sumw_none, sumw_abc] = df.get(selection::yield(cut_none, cut_abc));

  SUBCASE("branching") {
    CHECK(sumw_a.result().value == correct_sumw_a);
    CHECK(sumw_b.result().value == correct_sumw_b);
    CHECK(sumw_c.result().value == correct_sumw_c);
  }

  SUBCASE("merging") {
    CHECK(sumw_abc.result().value == correct_sumw_abc);
    CHECK(sumw_none.result().value == 0);
  }
}