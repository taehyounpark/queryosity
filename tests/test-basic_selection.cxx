#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>
#include "ana/analysis.h"

#include "plugins/trivial_input.h"
#include "plugins/sum_of_weights.h"

using ana::multithread;
template <typename T> using dataflow = ana::dataflow<T>;
using cut = ana::selection::cut;
using weight = ana::selection::weight;

TEST_CASE("basic selection") {

  auto nentries = 10000;

  // generate random data
  trivial_data_t random_data;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::normal_distribution<double> random_weight(1.0, 0.1);
  std::uniform_int_distribution<int> random_category(1,3);
  std::unordered_map<int,std::string> category_name{{1,"a"},{2,"b"},{3,"c"}};

  for (int i=0 ; i<nentries ; ++i) {

    auto w = random_weight(gen);
    auto c = category_name[random_category(gen)];

    random_data.emplace_back(std::unordered_map<std::string,std::variant<int,double,std::string>>{
      {"index", i},
      {"category", c}, 
      {"weight", w}
      });

  }

  // compute correct answer
  double correct_answer = 0;
  for (int i=0 ; i<nentries ; ++i) {
    if (std::get<std::string>(random_data[i]["category"])=="a") {
      correct_answer += std::get<double>(random_data[i]["weight"]);
    }
  }

  // compute answer with analogical
  ana::multithread::disable();
  auto df = ana::dataflow<trivial_input>(random_data);
  auto entry_weight = df.read<double>("weight");
  auto category = df.read<std::string>("category");
  auto category_a = df.constant<std::string>("a");
  auto weighted_category_a = df.filter<weight>("weight")(entry_weight).filter<cut>("category_a")(category==category_a);
  auto answer = df.book<sum_of_weights>().at(weighted_category_a);

  // compare answers
  REQUIRE(answer.result() == correct_answer);
}