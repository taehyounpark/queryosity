#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>

#include "ana/analogical.h"

#include "ana/table.h"
#include "ana/vecx.h"

template <typename T> using dataflow = ana::dataflow<T>;
using multithread = ana::multithread;

using cut = ana::selection::cut;
using weight = ana::selection::weight;

std::vector<int> get_correct_answer(const table_data_t &random_data) {
  std::vector<int> correct_answer;
  for (unsigned int i = 0; i < random_data.size(); ++i) {
    auto x = std::get<int>(random_data.at(i).at("value"));
    correct_answer.push_back(x);
  }
  return correct_answer;
}

std::vector<int> get_analogical_answer(const table_data_t &random_data,
                                       int ncores) {
  auto df = ana::dataflow<table>(random_data, multithread::enable(ncores));
  auto entry_value = df.read<int>("value");
  auto all_entries = df.filter("all")(df.constant(true));
  auto answer = df.book<vecx<int>>().fill(entry_value).at(all_entries);
  return answer.result();
}

TEST_CASE("multithreading consistency") {

  // generate random data
  table_data_t random_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  unsigned int nentries = 100;
  std::uniform_int_distribution<int> random_value(0, nentries);
  for (unsigned int i = 0; i < nentries; ++i) {
    auto x = random_value(gen);
    random_data.emplace_back(table_row_t{{"index", i}, {"value", x}});
  }

  // get answers
  auto correct_answer = get_correct_answer(random_data);
  auto analogical_answer1 = get_analogical_answer(random_data, 1);
  auto analogical_answer2 = get_analogical_answer(random_data, 2);
  auto analogical_answer3 = get_analogical_answer(random_data, 3);
  auto analogical_answer4 = get_analogical_answer(random_data, 4);
  auto analogical_answer5 = get_analogical_answer(random_data, 5);

  // compare answers
  SUBCASE("single-threaded result correctness") {
    CHECK(analogical_answer1 == correct_answer);
  }

  SUBCASE("multithreaded results consistency") {
    CHECK(analogical_answer1 == analogical_answer2);
    CHECK(analogical_answer1 == analogical_answer3);
    CHECK(analogical_answer1 == analogical_answer4);
    CHECK(analogical_answer1 == analogical_answer5);
  }
}