#pragma once

#include <string>

namespace ana 
{

class action
{

public:
	action(const std::string& name);
	virtual ~action() = default;

	virtual void initialize() = 0;
	virtual void execute()    = 0;
	virtual void finalize()   = 0;

	const std::string& get_name() const;

protected:
	std::string m_name;

};

}
