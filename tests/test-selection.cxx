#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>
#include <unordered_map>
#include "ana/analysis.h"

#include "plugins/trivial_input.h"
#include "plugins/sum_of_weights.h"
#include "plugins/entry_count.h"

using ana::multithread;
template <typename T> using dataflow = ana::dataflow<T>;
using cut = ana::selection::cut;
using weight = ana::selection::weight;

TEST_CASE("correctness & consistency of selections") {

  auto nentries = 10000;

  // generate random data
  trivial_data_t random_data;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::poisson_distribution random_weight(1);
  std::uniform_int_distribution random_category(1,3);
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
  long long correct_count_a = 0;
  long long correct_count_b = 0;
  long long correct_count_c = 0;
  for (int i=0 ; i<nentries ; ++i) {
    auto c = std::get<std::string>(random_data[i]["category"]);
    auto w = std::get<int>(random_data[i]["weight"]);
    if (c=="a") {
      ++correct_count_a;
    } else if (c=="b") {
      ++correct_count_b;
    } else {
      ++correct_count_c;
    }
  }

  // compute answer with analogical
  ana::multithread::disable();
  auto df = ana::dataflow<trivial_input>(random_data);
  auto entry_weight = df.read<int>("weight");
  auto entry_category = df.read<std::string>("category");

  auto weighted_all = df.filter<weight>("weight")(entry_weight);

  auto category_a = weighted_all.filter<cut>("category_a")(entry_category==df.constant<std::string>("a"));
  auto category_b = weighted_all.filter<cut>("category_b")(entry_category==df.constant<std::string>("b"));
  auto category_c = weighted_all.filter<cut>("category_c")(entry_category==df.constant<std::string>("c"));

  auto category_ab = category_a || category_b;
  auto category_bc = category_b || category_c;
  auto category_abc = category_a || category_b || category_c;

  auto count_a = df.book<entry_count>().at(category_a);
  auto count_b = df.book<entry_count>().at(category_b);
  auto count_c = df.book<entry_count>().at(category_c);

  auto count_ab = df.book<entry_count>().at(category_ab);
  auto count_bc = df.book<entry_count>().at(category_bc);
  auto count_abc = df.book<entry_count>().at(category_abc);

  // compare answers
  CHECK(count_a.result() == correct_count_a);
  CHECK(count_b.result() == correct_count_b);
  CHECK(count_c.result() == correct_count_c);

  CHECK(count_ab.result() == correct_count_a+correct_count_b);
  CHECK(count_bc.result() == correct_count_b+correct_count_c);
  CHECK(count_abc.result() == correct_count_a+correct_count_b+correct_count_c);

}