#ifdef BOOTSTRAP_HISTOGRAM
#include "queryosity/ROOT/ToySeed.hpp"
#include "queryosity/ROOT/ToyHist.hpp"
#endif

#include "queryosity/ROOT/Tree.hpp"
#include "queryosity/ROOT/Hist.hpp"

#include <queryosity.hpp>

#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedefs;

#pragma link C++ namespace queryosity;

#pragma link C++ class queryosity::dataflow+;

#pragma link C++ class queryosity::ROOT::Tree+;

#pragma link C++ class queryosity::column::constant<float>+;
#pragma link C++ class queryosity::column::expression<float(float)>+;

#pragma link C++ class queryosity::ROOT::Hist<1,float>+;

#ifdef BOOTSTRAP_HISTOGRAM
#pragma link C++ class queryosity::ROOT::ToySeed+;
#pragma link C++ class queryosity::ROOT::ToyHist<1,float>+;
#endif

#endif
