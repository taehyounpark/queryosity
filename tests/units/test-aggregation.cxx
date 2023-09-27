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
using weight = ana::selection::weight;

double get_correct_answer(const nlohmann::json &random_data) {
  double wsum = 0;
  unsigned long long sumw = 0.0;
  for (unsigned int i = 0; i < random_data.size(); ++i) {
    auto x = random_data[i]["x"].template get<double>();
    auto w = random_data[i]["weight"].template get<unsigned int>();
    wsum += x * w;
    sumw += w;
  }
  return sumw;
}

double get_analogical_answer(const nlohmann::json &random_data) {
  // auto data = ana::json(random_data);
  auto df = ana::dataflow(ana::json(random_data), multithread::enable(1),
                          sample::weight(1.0));
  auto entry_value = df.read<double>("x");
  auto entries_weighted =
      df.filter<weight>("weight")(df.read<unsigned int>("weight"));
  auto answer =
      df.book<ana::hist::hist<double>>(ana::hist::axis::regular(10, 0.0, 1000))
          .fill(entry_value)
          .at(entries_weighted);
  return boost::histogram::algorithm::sum(*answer.result());
  return 0.0;
}

TEST_CASE("compute weighted average") {

  // generate random data
  nlohmann::json random_data;
  std::random_device rd;
  std::mt19937 gen(rd());
  unsigned int nentries = 100;
  std::exponential_distribution<double> random_value(1 / 1.0);
  std::poisson_distribution<unsigned int> random_weight(1);
  for (unsigned int i = 0; i < nentries; ++i) {
    auto x = random_value(gen);
    auto w = random_weight(gen);
    random_data.emplace_back<nlohmann::json>(
        {{"index", i}, {"x", x}, {"weight", w}});
  }

  // get answers
  auto correct_answer = get_correct_answer(random_data);
  auto analogical_answer = get_analogical_answer(random_data);

  // compare answers
  CHECK(analogical_answer == correct_answer);
}