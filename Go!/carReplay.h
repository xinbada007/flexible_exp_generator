#pragma once
#include <osg/NodeCallback>
#include <osg/MatrixTransform>

#include "car.h"

class CarReplay :
	public osg::NodeCallback
{
public:
	CarReplay();
	virtual ~CarReplay();
	void operator()(osg::Node *node, osg::NodeVisitor *nv);
	void applyCarMovement(osg::Matrixd &m);

private:
	osg::ref_ptr<osg::MatrixTransform> _mTransform;
	CarState *_carState;
	unsigned _frame;
	bool _pause;
};

