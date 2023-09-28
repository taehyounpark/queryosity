#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <boost/histogram.hpp>
#include <nlohmann/json.hpp>
#include <random>
#include <unordered_map>

#include "ana/analogical.h"

#include "ana/hist.h"
#include "ana/json.h"

namespace multithread = ana::multithread;
namespace sample = ana::sample;
template <typename T> using dataflow = ana::dataflow<T>;
using cut = ana::selection::cut;

// generate random data
nlohmann::json generate_random_data(unsigned int nentries = 100) {
  nlohmann::json random_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<double> random_signal(100, 10);
  std::normal_distribution<double> random_variation(1.05, 0.1);
  std::poisson_distribution<unsigned int> random_weight(1);
  for (unsigned int i = 0; i < nentries; ++i) {
    auto x = random_signal(gen);
    auto v = random_variation(gen);
    auto w = random_weight(gen);
    random_data.emplace_back<nlohmann::json>(
        {{"i", i}, {"w", w}, {"x_nom", x}, {"x_var", x * v}});
  }
  return random_data;
}

double get_correct_answer(const nlohmann::json &random_data) {
  double wsumx_nom = 0;
  double wsumx_var = 0;
  unsigned long long sumw = 0.0;
  for (unsigned int i = 0; i < random_data.size(); ++i) {
    auto x_nom = random_data[i]["x_nom"].template get<double>();
    auto x_var = random_data[i]["x_var"].template get<double>();
    auto w = random_data[i]["w"].template get<unsigned int>();
    wsumx_nom += x_nom * w;
    wsumx_var += x_var * w;
    sumw += w;
  }
  return sumw;
}

double get_analogical_answer(const nlohmann::json &random_data) {
  // auto data = ana::json(random_data);
  auto df = ana::dataflow(ana::json(random_data));
  auto x = df.read<double>("x_nom").vary("vary_x", "x_var");
  auto weighted = df.weight("weight")(df.read<unsigned int>("w"));
  auto answer =
      df.book<ana::hist::hist<double>>(ana::hist::axis::regular(20, 0.0, 200))
          .fill(x)
          .at(weighted);
  return boost::histogram::algorithm::sum(*answer.nominal().result());
}

TEST_CASE("compute weighted average") {

  // get answers
  auto random_data = generate_random_data(1000);
  auto correct_answer = get_correct_answer(random_data);
  auto analogical_answer = get_analogical_answer(random_data);

  // compare answers
  CHECK(analogical_answer == correct_answer);
}