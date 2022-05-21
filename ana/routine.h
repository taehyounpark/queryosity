#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <atomic>

namespace ana
{

class routine
{

public:
	routine() = default;
	virtual ~routine() = default;

	virtual void initialize() = 0;
	virtual void execute()    = 0;
	virtual void finalize()   = 0;

};

}