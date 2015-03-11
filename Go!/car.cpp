#include "stdafx.h"
#include "car.h"
#include <algorithm>

Car::Car()
{
	_carState = new CarState;
}

Car::Car(const Car &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */) :
EulerPoly(copy, copyop), _vehicle(copy._vehicle), _carState(copy._carState)
{

}

Car::~Car()
{
	std::cout << "Deconsturct Car" << std::endl;

	_vehicle = NULL;
	_carState = NULL;
}

void Car::genCar(osg::ref_ptr<ReadConfig> refRC)
{
	_vehicle = refRC->getVehicle();

	osg::ref_ptr<osg::Vec3dArray> refV = _vehicle->_V;
	osg::Vec3dArray::const_iterator i = refV->begin();
	while (++i != refV->end())
	{
		link(*(i - 1), *i);
	}
	link(*(i - 1), *(refV->begin()));

	//Set-up backWheel and frontWheel
	i = refV->begin();

	const osg::Vec3d &RIGHT_TOP = *(i + 1);
	const osg::Vec3d &LEFT_TOP = *(i + 2);
	const osg::Vec3d &RIGHT_BOTTOM = *i;
	const osg::Vec3d &LEFT_BOTTOM = *(i + 3);
	const double ratio = 0.5f - (_vehicle->_wheelBase / (2 * _vehicle->_length));

	//right-top
//	_carState->_frontWheel->push_back(*(i + 1));
	_carState->_frontWheel->push_back(RIGHT_TOP*(1 - ratio) + RIGHT_BOTTOM*ratio);
	//left-top
//	_carState->_frontWheel->push_back(*(i + 2));
	_carState->_frontWheel->push_back(LEFT_TOP*(1 - ratio) + LEFT_BOTTOM*ratio);
	//right-bottom
//	_carState->_backWheel->push_back(*(i));
	_carState->_backWheel->push_back(RIGHT_BOTTOM*(1 - ratio) + RIGHT_TOP*ratio);
	//left-bottom
//	_carState->_backWheel->push_back(*(i + 3));
	_carState->_backWheel->push_back(LEFT_BOTTOM*(1 - ratio) + LEFT_TOP*ratio);

	_carState->_frontWheel->front().z() = 0.0f;
	_carState->_frontWheel->back().z() = 0.0f;
	_carState->_backWheel->front().z() = 0.0f;
	_carState->_backWheel->back().z() = 0.0f;

	//Set-up Original Point of Car
	_carState->_O = _vehicle->_O;
	_carState->updateLastO(_carState->_O);

	while (i != refV->end())
	{
		//from right-bottom to left-bottom under counter-clockwise
		_carState->_carArray->push_back(*i++);
		_carState->_carArray->back().z() = 0.0f;
	}
	_carState->_carArray->push_back(_carState->_O);
	_carState->_lastCarArray->resize(_carState->_carArray->size());
	copy(_carState->_carArray->begin(), _carState->_carArray->end(), _carState->_lastCarArray->begin());

	if (refRC->getSaveState())
	{
		_carState->_saveState = refRC->getSaveState();
	}
	if (refRC->getDynamicState())
	{
		_carState->_dynamicState = refRC->getDynamicState();
	}

	//set acceleration mode
	_carState->_dynamic = _vehicle->_acceleration;

	this->absoluteTerritory.center = _carState->_O;
	this->absoluteTerritory._detectR = std::max(_vehicle->_height, _vehicle->_width);
	this->absoluteTerritory._detectR = std::max(this->absoluteTerritory._detectR, _vehicle->_length);
	this->absoluteTerritory._refuseR = 0.0f;

	setDataVariance(osg::Object::DYNAMIC);
}