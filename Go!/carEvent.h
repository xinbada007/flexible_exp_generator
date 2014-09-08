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

	bool Joystick();

	void makeResetMatrix();

	void shiftVehicle();
	void autoNavigation();
	void checkRotationLimit();

	osg::ref_ptr<CarState> _carState;
	Vehicle *_vehicle;
	osg::ref_ptr<osg::MatrixTransform> _mTransform;
	osg::Matrix _moment;
	osg::Matrix _reset;

	bool _leftTurn;
	double _lastAngle;
	bool _autoNavi;
	bool _shifted;
	bool _speedLock;

	bool _updated;

	osg::ref_ptr<osg::UIntArray> _buttons;
};

