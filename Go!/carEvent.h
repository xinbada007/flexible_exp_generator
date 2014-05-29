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
	void applyCarMovement();
	void calculateCarMovement();
	void autoNavigation();
	bool Joystick();

	osg::ref_ptr<CarState> _carState;
	const Vehicle *_vehicle;
	osg::ref_ptr<osg::MatrixTransform> _mTransform;

	bool _leftTurn;
	bool _acceleration;
	bool _updated;

	double _lastAngle;
	bool _lastTurn;
};

