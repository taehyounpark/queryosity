#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <random>

#include "ana/analysis.h"

#include "plugins/trivial_input.h"
#include "plugins/simple_sum.h"

trivial_data_t generate_random_data(int nentries) {

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> category(1,3);
  std::poisson_distribution<> weight(1);
  std::exponential_distribution<> entry(1/10.0);

  trivial_data_t random_data;
  for (int index=0 ; index<nentries ; ++index) {

    auto cat = category(gen);
    auto x = entry(gen);
    auto w = weight(gen);

    random_data.emplace_back(std::unordered_map<std::string,std::variant<int,double,std::string>>{
      {"index", index},
      {"category", cat==1 ? "a" : (cat==2 ? "b" : "c")}, 
      {"entry", x},
      {"weight", w}
      });
  }

  return random_data;
}

int get_string_selection(int ncores) {
  ana::multithread::enable(ncores);
  auto random_data = generate_random_data(10000);
  auto df = ana::dataflow<trivial_input>(random_data);
  auto entry = df.read<int>("entry");
  auto category = df.read<std::string>("category");
  auto all_entries = df.filter<ana::selection::cut>("all_entries")(category == df.constant<std::string>("a"));
  auto simple_sum_of_values = df.book<simple_sum>().fill(entry).at(all_entries);
  return simple_sum_of_values.result();
}

TEST_CASE("simple sum") {
  CHECK(get_string_selection(1) == 103);
  CHECK(get_string_selection(4) == 103);
}