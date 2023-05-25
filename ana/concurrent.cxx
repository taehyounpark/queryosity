#include "ana/concurrent.h"

int ana::multithread::s_suggestion = 0;

void ana::multithread::enable(int suggestion)
{
  s_suggestion = suggestion;
}

void ana::multithread::disable()
{
  s_suggestion = 0;
}

bool ana::multithread::status()
{
  return s_suggestion == 0 ? false : true;
}

unsigned int ana::multithread::concurrency()
{
  return std::max<unsigned int>( 1, s_suggestion < 0 ? std::thread::hardware_concurrency() :  s_suggestion );
}