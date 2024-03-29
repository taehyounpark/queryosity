#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>

#include "queryosity/queryosity.h"

#include "queryosity/json.h"
#include "queryosity/sumw.h"

namespace qty = queryosity;

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
    auto cat = random_category(gen);
    random_data.emplace_back<nlohmann::json>(
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
    auto cat = random_data[i]["c"].template get<std::string>();
    auto w = random_data[i]["w"].template get<int>();
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
  qty::dataflow df(qty::multithread::disable());

  // auto test = dataset::input<qty::json>(random_data);
  // auto ds = df.load(std::move(test));

  // auto ds =
  // df.load(qty::dataset::input<qty::json>(random_data));

  auto [cat, w] =
      df.read(qty::dataset::input<qty::json>(random_data),
              qty::dataset::column<std::string>("c"),
              qty::dataset::column<unsigned int>("w"));

  auto weighted = df.weight(w);

  // auto a = df.define(qty::column::constant<std::string>("a"));
  // auto b = df.define(qty::column::constant<std::string>("b"));
  // auto c = df.define(qty::column::constant<std::string>("c"));

  auto a = df.define(qty::column::constant<std::string>("a"));
  auto b = df.define(qty::column::constant<std::string>("b"));
  auto c = df.define(qty::column::constant<std::string>("c"));

  auto cut_a = weighted.filter(cat == a);
  auto cut_b = weighted.filter(cat == b);
  auto cut_c = weighted.filter(cat == c);

  auto cut_ab = weighted.filter(cut_a || cut_b);
  auto cut_bc = weighted.filter(cut_b || cut_c);
  auto cut_abc = weighted.filter(cut_a || cut_b || cut_c);

  auto cut_none = weighted.filter(cut_a && cut_b && cut_c);
  auto cut_a2 = weighted.filter(cut_ab && cut_a);
  auto cut_b2 = weighted.filter(cut_ab && cut_b);

  // auto sumw_a = df.get<qty::sumw>().at(cut_a);
  // auto sumw_a = cut_a.book(df.get<qty::sumw>());

  auto [sumw_a, sumw_b, sumw_c] =
      df.make(qty::query::plan<qty::sumw>()).book(cut_a, cut_b, cut_c);
  auto [sumw_ab, sumw_bc] =
      df.make(qty::query::plan<qty::sumw>()).book(cut_ab, cut_bc);
  auto [sumw_none, sumw_abc] =
      df.make(qty::query::plan<qty::sumw>()).book(cut_none, cut_abc);

  auto sumw_one2 = df.make(qty::query::plan<qty::sumw>()).book(cut_a2, cut_b2);

  std::cout << "hi" << std::endl;
  std::cout << cut_a2.size() << std::endl;

  SUBCASE("basic results") {
    CHECK(sumw_a.result() == correct_sumw_a);
    CHECK(sumw_b.result() == correct_sumw_b);
    CHECK(sumw_c.result() == correct_sumw_c);
  }

  SUBCASE("advanced selections") {
    CHECK(sumw_abc.result() == correct_sumw_abc);
    CHECK(sumw_none.result() == 0);
  }
}