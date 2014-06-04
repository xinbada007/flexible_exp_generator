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

	osg::ref_ptr<CarState> _carState;
	const Vehicle *_vehicle;
	osg::ref_ptr<osg::MatrixTransform> _mTransform;
	osg::Matrix _moment;

	bool _leftTurn;
	bool _acceleration;
	double _lastAngle;
	bool _shifted;

	bool _updated;
};

