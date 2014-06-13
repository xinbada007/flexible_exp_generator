#pragma once
#include <osg/NodeCallback>
#include <osg/MatrixTransform>

#include "car.h"

class CarEvent :
	public osg::NodeCallback
{
public:
	CarEvent();
	void operator()(osg::Node *node, osg::NodeVisitor *nv);
private:
	~CarEvent();
	void dynamicApply();
	void applyCarMovement();
	void calculateCarMovement();
	void autoNavigation();
	bool Joystick();
	void makeResetMatrix();

	osg::ref_ptr<CarState> _carState;
	Vehicle *_vehicle;
	osg::ref_ptr<osg::MatrixTransform> _mTransform;
	osg::Matrix _moment;
	osg::Matrix _reset;

	bool _leftTurn;
	double _lastAngle;
	bool _shifted;

	bool _updated;
};

