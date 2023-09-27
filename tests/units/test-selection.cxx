#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>

#include "ana/analogical.h"

#include "ana/sumw.h"
#include "ana/table.h"

using multithread = ana::multithread;
template <typename T> using dataflow = ana::dataflow<T>;
using cut = ana::selection::cut;
using weight = ana::selection::weight;

TEST_CASE("correctness & consistency of selections") {

  unsigned int nentries = 1000;

  // generate random data
  table_data_t random_data;

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
        table_row_t{{"index", i}, {"category", c}, {"weight", w}});
  }

  // compute correct answer
  int correct_sumw_a = 0;
  int correct_sumw_b = 0;
  int correct_sumw_c = 0;
  for (unsigned int i = 0; i < nentries; ++i) {
    auto c = std::get<std::string>(random_data[i]["category"]);
    auto w = std::get<int>(random_data[i]["weight"]);
    if (c == "a") {
      correct_sumw_a += w;
    } else if (c == "b") {
      correct_sumw_b += w;
    } else {
      correct_sumw_c += w;
    }
  }

  // compute answer with analogical
  ana::multithread::disable();
  auto df = ana::dataflow<table>(random_data);
  auto entry_category = df.read<std::string>("category");

  auto raw_entries = df.filter("raw")(df.constant(true));
  auto weighted_entries = df.filter<weight>("weight")(df.read<int>("weight"));

  auto cut_a = weighted_entries.filter<cut>("a")(entry_category ==
                                                 df.constant<std::string>("a"));
  auto cut_b = weighted_entries.filter<cut>("b")(entry_category ==
                                                 df.constant<std::string>("b"));
  auto cut_c = weighted_entries.filter<cut>("c")(entry_category ==
                                                 df.constant<std::string>("c"));

  auto cut_ab = weighted_entries.filter<cut>("ab")(cut_a || cut_b);
  auto cut_bc = weighted_entries.filter<cut>("bc")(cut_b || cut_c);
  auto cut_abc = weighted_entries.filter<cut>("abc")(cut_a || cut_b || cut_c);

  auto cut_none = weighted_entries.filter<cut>("none")(cut_a && cut_b && cut_c);
  auto cut_a2 = weighted_entries.filter<cut>("a2")(cut_ab && cut_a);
  auto cut_b2 = weighted_entries.filter<cut>("b2")(cut_ab && cut_b);

  auto sumw_a = df.book<sumw>().at(cut_a);
  auto sumw_b = df.book<sumw>().at(cut_b);
  auto sumw_c = df.book<sumw>().at(cut_c);

  auto sumw_ab = df.book<sumw>().at(cut_ab);
  auto sumw_bc = df.book<sumw>().at(cut_bc);
  auto sumw_abc = df.book<sumw>().at(cut_abc);

  auto sumw_none = df.book<sumw>().at(cut_none);
  auto sumw_a2 = df.book<sumw>().at(cut_a2);
  auto sumw_b2 = df.book<sumw>().at(cut_b2);

  SUBCASE("basic results") {
    CHECK(sumw_a.result() == correct_sumw_a);
    CHECK(sumw_b.result() == correct_sumw_b);
    CHECK(sumw_c.result() == correct_sumw_c);
  }

  SUBCASE("binary operations") {
    CHECK(sumw_ab.result() == correct_sumw_a + correct_sumw_b);
    CHECK(sumw_bc.result() == correct_sumw_b + correct_sumw_c);
    CHECK(sumw_abc.result() ==
          correct_sumw_a + correct_sumw_b + correct_sumw_c);

    CHECK(sumw_a2.result() == correct_sumw_a);
    CHECK(sumw_b2.result() == correct_sumw_b);
    CHECK(sumw_none.result() == 0);
  }
}