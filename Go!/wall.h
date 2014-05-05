#pragma once
#include "obstacles.h"
class Road;

class Wall :
	public Obstacles
{
public:
	Wall();
	virtual void build(Solid *refRd);

private:
	virtual ~Wall();
};

