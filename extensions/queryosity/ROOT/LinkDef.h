#ifdef BOOTSTRAP_HISTOGRAM
#include "queryosity/ROOT/hist_with_toys.hpp"
#endif

#include "queryosity/ROOT/tree.hpp"
#include "queryosity/ROOT/hist.hpp"

#include <queryosity.hpp>

#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedefs;

#pragma link C++ namespace queryosity;

#pragma link C++ class queryosity::dataset::processor+;
#pragma link C++ class queryosity::dataset::weight+;
#pragma link C++ class queryosity::multithread::core+;
#pragma link C++ class queryosity::dataset::player+;
#pragma link C++ class queryosity::ensemble::slotted<queryosity::dataset::player>+;
#pragma link C++ class queryosity::column::computation+;
#pragma link C++ class queryosity::selection::cutflow+;
#pragma link C++ class queryosity::query::experiment+;
#pragma link C++ class queryosity::dataflow+;

#pragma link C++ class queryosity::column::constant<float>+;
#pragma link C++ class queryosity::column::expression<float(float)>+;
#pragma link C++ class queryosity::ROOT::tree+;
#pragma link C++ class queryosity::ROOT::hist<1,float>+;
#ifdef BOOTSTRAP_HISTOGRAM
#pragma link C++ class queryosity::ROOT::toy_generator+;
#pragma link C++ class queryosity::ROOT::hist_with_toys<1,float>+;
#endif

#endif
