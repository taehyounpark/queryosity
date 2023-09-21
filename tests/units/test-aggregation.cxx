#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>

#include "ana/analogical.h"

#include "plugins/table.h"
#include "plugins/wavg.h"

using ana::multithread;
template <typename T> using dataflow = ana::dataflow<T>;
using cut = ana::selection::cut;
using weight = ana::selection::weight;

double get_correct_answer(const table_data_t &random_data) {
  double wsum = 0;
  unsigned long long sumw = 0.0;
  for (unsigned int i = 0; i < random_data.size(); ++i) {
    auto x = std::get<double>(random_data.at(i).at("value"));
    auto w = std::get<unsigned int>(random_data.at(i).at("weight"));
    wsum += x * w;
    sumw += w;
  }
  return wsum / sumw;
}

double get_analogical_answer(const table_data_t &random_data) {
  ana::multithread::disable();
  auto df = ana::dataflow<table>(random_data);
  auto entry_value = df.read<double>("value");
  auto entries_weighted =
      df.filter<weight>("weight")(df.read<unsigned int>("weight"));
  auto answer = df.book<wavg>().fill(entry_value).at(entries_weighted);
  return answer.result();
}

TEST_CASE("compute weighted average") {

  // generate random data
  table_data_t random_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  unsigned int nentries = 100;
  std::exponential_distribution<double> random_value(1 / 1.0);
  std::poisson_distribution<unsigned int> random_weight(1);
  for (unsigned int i = 0; i < nentries; ++i) {
    auto x = random_value(gen);
    auto w = random_weight(gen);
    random_data.emplace_back(
        table_row_t{{"index", i}, {"value", x}, {"weight", w}});
  }

  // get answers
  auto correct_answer = get_correct_answer(random_data);
  auto analogical_answer = get_analogical_answer(random_data);

  // compare answers
  CHECK(analogical_answer == correct_answer);
}