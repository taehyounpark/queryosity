#include "queryosity/ROOT/Tree.hpp"
#include "queryosity/ROOT/Hist.hpp"

#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclasses;
#pragma link C++ nestedtypedefs;

#pragma link C++ namespace queryosity;

#pragma link C++ class queryosity::dataflow+;

#pragma link C++ class queryosity::ROOT::Tree+;
#pragma link C++ struct queryosity::dataset::input<queryosity::ROOT::Tree>+;

#pragma link C++ class queryosity::column::constant<float>+;
#pragma link C++ class queryosity::column::expression<float(float)>+;

#pragma link C++ class queryosity::query::output<queryosity::ROOT::Hist<1,float>>+;

#endif
