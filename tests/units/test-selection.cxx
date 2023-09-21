#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>

#include "ana/analogical.h"

#include "plugins/sumw.h"
#include "plugins/table.h"

using ana::multithread;
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
  auto weight_entry = df.filter<weight>("weight")(df.read<int>("weight"));
  auto cut_a = weight_entry.filter<cut>("cut_a")(entry_category ==
                                                 df.constant<std::string>("a"));
  auto cut_b = weight_entry.filter<cut>("cut_b")(entry_category ==
                                                 df.constant<std::string>("b"));
  auto cut_c = weight_entry.filter<cut>("cut_c")(entry_category ==
                                                 df.constant<std::string>("c"));

  auto cut_ab = (cut_a || cut_b) * weight_entry;
  auto cut_bc = (cut_b || cut_c) * weight_entry;
  auto cut_abc = (cut_a || cut_b || cut_c) * weight_entry;

  auto cut_none = (cut_a && cut_b && cut_c) * weight_entry;
  auto cut_a2 = (cut_a && cut_ab) * weight_entry;
  auto cut_b2 = (cut_b && cut_bc) * weight_entry;

  auto sumw_a = df.book<sumw>().at(cut_a);
  auto sumw_b = df.book<sumw>().at(cut_b);
  auto sumw_c = df.book<sumw>().at(cut_c);

  auto sumw_ab = df.book<sumw>().at(cut_ab);
  auto sumw_bc = df.book<sumw>().at(cut_bc);
  auto sumw_abc = df.book<sumw>().at(cut_abc);

  auto sumw_a2 = df.book<sumw>().at(cut_a2);
  auto sumw_b2 = df.book<sumw>().at(cut_b2);
  auto sumw_none = df.book<sumw>().at(cut_none);

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