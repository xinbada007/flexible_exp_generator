#pragma once
#include "eulerPoly.h"

class Obstacles :
	public EulerPoly
{
public:
	Obstacles();
	virtual ~Obstacles();

	virtual void draw();
	virtual void build(Solid *refRd) = 0;
};

