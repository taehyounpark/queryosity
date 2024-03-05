#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>

#include "queryosity/queryosity.h"

#include <nlohmann/json.hpp>

#include "queryosity/col.h"
#include "queryosity/json.h"

using dataflow = queryosity::dataflow;
namespace multithread = queryosity::multithread;
namespace dataset = queryosity::dataset;
namespace column = queryosity::column;
namespace query = queryosity::query;
namespace systematic = queryosity::systematic;

std::vector<int> get_correct_result(const nlohmann::json &random_data) {
  std::vector<int> correct_result;
  for (unsigned int i = 0; i < random_data.size(); ++i) {
    auto x = random_data.at(i).at("value").template get<int>();
    correct_result.push_back(x);
  }
  return correct_result;
}

std::vector<int> get_queryosity_result(const nlohmann::json &random_data,
                                       int ncores) {
  dataflow df(multithread::enable(ncores));
  auto ds = df.load(dataset::input<queryosity::json>(random_data));
  auto entry_value = ds.read(dataset::column<int>("value"));
  auto all = df.define(column::constant<bool>(true));
  auto incl = df.filter(all);
  auto col = df.get(query::output<queryosity::col<int>>());
  col = col.fill(entry_value);
  return incl.book(col).result();
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
  auto queryosity_result1 = get_queryosity_result(random_data, 1);
  auto queryosity_result2 = get_queryosity_result(random_data, 2);
  auto queryosity_result3 = get_queryosity_result(random_data, 3);
  auto queryosity_result4 = get_queryosity_result(random_data, 4);

  // compare results
  SUBCASE("single-threaded result") {
    CHECK(queryosity_result1 == correct_result);
  }

  SUBCASE("multithreaded results") {
    CHECK(queryosity_result1 == queryosity_result2);
    CHECK(queryosity_result1 == queryosity_result3);
    CHECK(queryosity_result1 == queryosity_result4);
  }
}