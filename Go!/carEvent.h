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
	void applyCarMovement();
	void calculateCarMovement();

	bool Joystick();

	void makeResetMatrix();

	void getTurningFactor();
	void shiftVehicle();
	void autoNavigation();
	void checkRotationLimit();
	void dealCollision();
	void carController();

	osg::ref_ptr<Car> _car;
	osg::ref_ptr<CarState> _carState;
	Vehicle *_vehicle;
	osg::ref_ptr<osg::MatrixTransform> _mTransform;
	osg::Matrix _moment;
	osg::Matrix _reset;

	bool _leftTurn;
	double _lastAngle;
	bool _autoNavi;
	bool _shifted;
	int _speedSign;

	bool _lockedSpeed;
	bool _lockedSW;

	bool _updated;

	osg::ref_ptr<osg::UIntArray> _buttons;
protected:
	~CarEvent();
};

class dirtyVisitor :
	public osg::NodeVisitor
{
public:
	dirtyVisitor() :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN){};
	virtual ~dirtyVisitor(){};
	void apply(osg::Geode &geo);
};
