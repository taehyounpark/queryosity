#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>

#include "ana/analogical.h"

#include <nlohmann/json.hpp>

#include "Columnar.h"
#include "Json.h"

namespace multithread = ana::multithread;
using dataflow = ana::dataflow;

std::vector<int> get_correct_result(const nlohmann::json &random_data) {
  std::vector<int> correct_result;
  for (unsigned int i = 0; i < random_data.size(); ++i) {
    auto x = random_data.at(i).at("value").template get<int>();
    correct_result.push_back(x);
  }
  return correct_result;
}

std::vector<int> get_analogical_result(const nlohmann::json &random_data,
                                       int ncores) {
  dataflow df(multithread::enable(ncores));
  auto ds = df.open<Json>(random_data);
  auto entry_value = ds.read<int>("value");
  auto all_entries = df.filter("all")(df.constant(true));
  auto columnar = df.agg<Columnar<int>>();
  columnar = columnar.fill(entry_value);
  return columnar.book(all_entries).result();
}

TEST_CASE("multithreading consistency") {

  // generate random data
  nlohmann::json random_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  unsigned int nentries = 100;
  std::uniform_int_distribution<int> random_value(0, nentries);
  for (unsigned int i = 0; i < nentries; ++i) {
    auto x = random_value(gen);
    random_data.emplace_back(nlohmann::json{{"index", i}, {"value", x}});
  }

  // get results
  auto correct_result = get_correct_result(random_data);
  auto analogical_result1 = get_analogical_result(random_data, 1);
  auto analogical_result2 = get_analogical_result(random_data, 2);
  auto analogical_result3 = get_analogical_result(random_data, 3);
  auto analogical_result4 = get_analogical_result(random_data, 4);
  auto analogical_result5 = get_analogical_result(random_data, 5);

  // compare results
  SUBCASE("single-threaded result correctness") {
    CHECK(analogical_result1 == correct_result);
  }

  SUBCASE("multithreaded results consistency") {
    CHECK(analogical_result1 == analogical_result2);
    CHECK(analogical_result1 == analogical_result3);
    CHECK(analogical_result1 == analogical_result4);
    CHECK(analogical_result1 == analogical_result5);
  }
}