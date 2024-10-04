#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <queryosity/nlohmann/json.hpp>
#include <nlohmann/json.hpp>

#include <queryosity.hpp>

#include <random>
#include <unordered_map>

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace selection = qty::selection;
namespace query = qty::query;
namespace systematic = qty::systematic;

nlohmann::json generate_test_data() {
  // generate random data
  nlohmann::json test_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  unsigned int nentries = 100;
  std::uniform_int_distribution<int> random_value(0, nentries);
  for (unsigned int i = 0; i < nentries; ++i) {
    auto x = random_value(gen);
    test_data.emplace_back(nlohmann::json{{"index", i}, {"x", x}});
  }
  return test_data;
}

std::vector<int> get_correct_result(nlohmann::json const &test_data) {
  std::vector<int> correct_result;
  for (unsigned int i = 0; i < test_data.size(); ++i) {
    auto x = test_data.at(i).at("x").template get<int>();
    correct_result.push_back(x);
  }
  return correct_result;
}

std::vector<int> get_queryosity_result(nlohmann::json const &test_data,
                                       int ncores) {
  dataflow df(multithread::enable(ncores));
  auto ds = df.load(dataset::input<qty::nlohmann::json>(test_data));
  auto entry_value = ds.read(dataset::column<int>("x"));
  auto all = df.filter(column::constant<bool>(true));
  auto col = df.get(column::series(entry_value)).at(all);
  return col.result();
}

TEST_CASE("multithreading consistency") {

  auto test_data = generate_test_data();

  // get results
  auto correct_result = get_correct_result(test_data);
  auto queryosity_result1 = get_queryosity_result(test_data, 1);
  auto queryosity_result2 = get_queryosity_result(test_data, 2);
  auto queryosity_result3 = get_queryosity_result(test_data, 3);
  auto queryosity_result4 = get_queryosity_result(test_data, 4);

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