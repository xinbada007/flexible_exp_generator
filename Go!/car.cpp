#include "stdafx.h"
#include "car.h"
#include <algorithm>

Car::Car()
{
	_carState = new CarState;
}

Car::Car(const Car &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */) :
EulerPoly(copy, copyop), _vehicle(copy._vehicle), _carState(copy._carState), _carThread(NULL),
_carTerminated(false)
{

}

Car::~Car()
{
	std::cout << "Deconsturct Car" << std::endl;

	_vehicle = NULL;
	_carState = NULL;

	_carTerminated = true;
	if (_carThread)
	{
		WaitForSingleObject(_carThread, INFINITE);
		_carThread = NULL;
	}
}

DWORD WINAPI Car::callCommunication(LPVOID lpParam)
{
	Car *thisCar = (Car *)lpParam;

	while (!thisCar->_carTerminated)
	{
		thisCar->networkINFunc();
	}

	return 0;
}

DWORD WINAPI Car::networkINFunc()
{	
	boost::asio::io_service io;
	boost::asio::ip::tcp::socket socket(io);
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(_vehicle->_ipAddress), _vehicle->_ipPort);
	boost::asio::deadline_timer timer(io);

	bool isconnected = true;

	socket.async_connect(ep, boost::bind(&Car::receiveMsg, this, &socket, &timer, boost::asio::placeholders::error));
	timer.expires_from_now(boost::posix_time::millisec(1000));
	timer.async_wait(boost::bind(&Car::connectionTimeOut, this, &socket, boost::asio::placeholders::error));

	io.run();

	if (!isconnected)
	{
		return 1;
	}

	return 0;
}

void Car::connectionTimeOut(boost::asio::ip::tcp::socket *socket, const boost::system::error_code &ec)
{
	if (_carTerminated)
	{
		socket->close();
		return;
	}

	if (!ec)
	{
		std::cout << "Connection Time Out" << std::endl;
		socket->close();
	}
}

void Car::receiveMsg(boost::asio::ip::tcp::socket *socket, boost::asio::deadline_timer *timer, const boost::system::error_code &ec)
{
	if (ec) return;
	timer->cancel();
 
// 	boost::asio::deadline_timer rTimer(socket->get_io_service());
	bool isread = false;
	char input[4096];
	boost::system::error_code ercode;
	unsigned bytestranferred = 0;
	const unsigned RETRY(100);
	unsigned numretry(0);
	while (!_carTerminated)
	{
		try
		{
			bytestranferred = socket->read_some(boost::asio::buffer(input, sizeof(input)), ercode);
		}
		catch (std::exception &e)
		{
			std::cout << e.what() << std::endl;
			break;
		}
		if ((!bytestranferred || ercode))
		{
			break;
		}
// 		socket->async_read_some(boost::asio::buffer(temp), boost::bind(&Car::receiveMsgHandler, this, &isread, temp, timer, boost::asio::placeholders::bytes_transferred, boost::asio::placeholders::error));
// 		timer->expires_from_now(boost::posix_time::millisec(100));
// 		timer->async_wait(boost::bind(&Car::receiveTimeOut, this, socket, boost::asio::placeholders::error));
	}
	socket->close();
}

void Car::receiveMsgHandler(bool *isread, char temp[4096], boost::asio::deadline_timer *timer, const unsigned &bytestransferred, const boost::system::error_code &ec)
{
	if (!ec && bytestransferred != 0)
	{
 		std::cout << "OK\n";
		*isread = true;
		timer->cancel();
	}
	else if (ec && !bytestransferred)
	{
// 		std::cout << "NO\n";
		*isread = false;
		return;
	}
}

void Car::receiveTimeOut(boost::asio::ip::tcp::socket *socket, const boost::system::error_code &ec)
{
	if (!ec)
	{
		std::cout << "Disconnected" << std::endl;
		socket->close();
	}
	else
	{
		std::cout << "OK\n";
	}
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

	if (!_vehicle->_ipAddress.empty() && !_carThread)
	{
 		_carThread = CreateThread(NULL, NULL, Car::callCommunication,(LPVOID)this, NULL, NULL);
	}
}