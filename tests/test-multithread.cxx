#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "ana/analogical.h"
#include <random>
#include <unordered_map>

#include "ana/trivial_input.h"
#include "ana/weighted_sum.h"

using ana::multithread;
template <typename T> using dataflow = ana::dataflow<T>;
using cut = ana::selection::cut;
using weight = ana::selection::weight;

int get_correct_answer(const trivial_data_t &random_data) {
  int correct_answer = 0;
  for (int i = 0; i < random_data.size(); ++i) {
    auto x = std::get<int>(random_data.at(i).at("value"));
    auto w = std::get<int>(random_data.at(i).at("weight"));
    correct_answer += x * w;
  }
  return correct_answer;
}

int get_analogical_answer(const trivial_data_t &random_data, int ncores) {
  ana::multithread::enable(ncores);
  auto df = ana::dataflow<trivial_input>(random_data);
  auto entry_weight = df.read<int>("weight");
  auto entry_value = df.read<int>("value");
  auto weightd_entries = df.filter<weight>("weight")(entry_weight);
  auto answer = df.book<weighted_sum>().fill(entry_value).at(weightd_entries);
  return answer.result();
}

TEST_CASE("multithreading consistency") {

  // generate random data
  trivial_data_t random_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution random_weight(0, 2);
  std::uniform_int_distribution random_entry(0, 10);
  auto nentries = 1000;
  for (int i = 0; i < nentries; ++i) {
    auto w = random_weight(gen);
    auto x = random_entry(gen);
    random_data.emplace_back(
        std::unordered_map<std::string, std::variant<int, double, std::string>>{
            {"index", i}, {"weight", w}, {"value", x}});
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