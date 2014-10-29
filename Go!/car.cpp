#include "stdafx.h"
#include "car.h"
#include <algorithm>

Car::Car()
{
	_carState = new CarState;
}

Car::Car(const Car &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */):
EulerPoly(copy, copyop), _vehicle(copy._vehicle), _carState(copy._carState)
{

}

Car::~Car()
{
	std::cout << "Deconsturct Car" << std::endl;
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
	//right-top
	_carState->_frontWheel->push_back(*(i + 1));
	//left-top
	_carState->_frontWheel->push_back(*(i + 2));
	//right-bottom
	_carState->_backWheel->push_back(*(i));
	//left-bottom
	_carState->_backWheel->push_back(*(i + 3));

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
}