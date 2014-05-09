#include "stdafx.h"
#include "readConfig.h"
#include "math.h"

#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <osg/Notify>

using namespace std;

ReadConfig::ReadConfig():
_filename("")
{
	_nurbs = new Nurbs;
	_screens = new Screens;
	_vehicle = new Vehicle;
	_roads = new RoadSet;
}

ReadConfig::ReadConfig(const std::string &filename):
_filename(filename)
{
	assignConfig();
}

ReadConfig::~ReadConfig()
{
}

void ReadConfig::assignConfig()
{
	ifstream filein(_filename.c_str());

	if (!filein)
	{
		cout << "File Open Failed" << endl;
		cout << _filename << "does not exist!" << endl;
		return;
	}

	string config;
	const string ENDLESSNURBS = "Endless Nurbs";
	byPassSpace(filein, config);
	if (config == ENDLESSNURBS)
	{
		filein.close();
		_nurbs = new Nurbs;
		readNurbs();
		alignCtrlPoints(NULL);
		updateNurbs(new NurbsCurve);
		return;
	}

	const string TRAIL = "Trial Config";
	if (config == TRAIL)
	{
		filein.close();
		_screens = new Screens;
		_vehicle = new Vehicle;
		_roads = new RoadSet;
		//read trial config
		readTrial();
		//read nurbs config according to trial config
		stringList::const_iterator i = _roads->_roadTxt.cbegin();
		while (i != _roads->_roadTxt.cend())
		{
			_filename = *i++;
			_nurbs = new Nurbs;
			readNurbs();
			
			Nurbs *prve = (_roads->_nurbs.empty() ? NULL : _roads->_nurbs.back());			
			alignCtrlPoints(prve);
			
			updateNurbs(new NurbsCurve);

			_roads->_nurbs.push_back(_nurbs.release());
		}
		return;
	}
}

bool ReadConfig::byPassSpace(ifstream &in, std::string &content)
{
	const string COLON = ":";
	const string COMMA = ",";
	const string SPACE = " ";
	const string SEMICOLON = ";";

	const string TAB = "\t";
	const string ENTER = "\n";
	const string COMMENT = "#";

	while (!in.eof())
	{
		getline(in, content);
		std::size_t found = content.find_first_not_of(*SPACE.c_str());
		if (found != std::string::npos) content.erase(content.begin(), content.begin() + found);
		found = content.find_first_not_of(*TAB.c_str());
		if (found != std::string::npos) content.erase(content.begin(), content.begin() + found);
		found = content.find_first_not_of(*ENTER.c_str());
		if (found != std::string::npos) content.erase(content.begin(), content.begin() + found);
		found = content.find_first_of(*COMMENT.c_str());
		if (found != std::string::npos) content.erase(content.begin() + found, content.end());
		if (content.empty()) continue;
		else break;
	}

	if (!content.empty())
	{
		std::replace(content.begin(),content.end(),*TAB.c_str(), *SPACE.c_str());
		std::replace(content.begin(), content.end(), *SEMICOLON.c_str(), *SPACE.c_str());
		std::replace(content.begin(), content.end(), *COMMA.c_str(), *SPACE.c_str());
		std::replace(content.begin(), content.end(), *COLON.c_str(), *SPACE.c_str());
		std::string::iterator i = std::unique(content.begin(), content.end(), uniqueTest<const char>(*SPACE.c_str()));
		content.erase(i, content.end());
		if (content.back() == *SPACE.c_str()) content.erase(content.end() - 1);
		return true;
	}

	return false;
}

void ReadConfig::readTrial()
{
	ifstream in(_filename.c_str());
	if (!in)
	{
		osg::notify(osg::FATAL) << "File Open Failed (Config)" << endl;
		return;
	}

	string flag;
	byPassSpace(in, flag);
	if (flag != "Trial Config")
	{
		in.close();
		osg::notify(osg::FATAL) << "File Doesn't Match" << std::endl;
		return;
	}

	const string SCREEN = "[ScreenSettings]";
	const string CAR = "[CarSettings]";
	const string ROAD = "[RoadSettings]";

	while (!in.eof())
	{
		byPassSpace(in, flag);
		string config;
		std::size_t found;
		//Set Screen
		const string SCR = "screens";
		const string ASPECT = "aspect";
		const string REALWORD = "RealWorld";
		const string BGPIC = "background";
		while (flag == SCREEN)
		{
			byPassSpace(in, config);
			if (config.find(SCR) != config.npos)
			{
				config.erase(config.begin(), config.begin() + SCR.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_screens->_scrs->push_back(stoi(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (config.find(ASPECT) != config.npos)
			{
				config.erase(config.begin(), config.begin() + ASPECT.size());
				_screens->_aspect = stod(config);
				continue;
			}
			else if (config.find(REALWORD) != config.npos)
			{
				config.erase(config.begin(), config.begin() + REALWORD.size());				
				std::string::size_type sz;
				if(!stoi(config, &sz)) continue;
				config.erase(config.begin(), config.begin() + sz);
				while (!config.empty())
				{
					_screens->_realworld->push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (config.find(BGPIC) != config.npos)
			{
				config.erase(config.begin(), config.begin() + BGPIC.size());
				config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				_screens->_background = config;
				continue;
			}

			flag = config;
		}

		//Set Vehicle
		const string ACCEL = "acceleration";
		const string CARSPEED = "CarSpeed";
		const string CARWHEEL = "CarWheel";
		const string CARPIC = "CarPic";
		while (flag == CAR)
		{
			byPassSpace(in, config);
			if (config.find(ACCEL) != config.npos)
			{
				config.erase(config.begin(), config.begin() + ACCEL.size());
				_vehicle->_acceleration = (stoi(config) == 1) ? true : false;
				continue;
			}
			else if (config.find(CARSPEED) != config.npos)
			{
				config.erase(config.begin(), config.begin() + CARSPEED.size());
				_vehicle->_speed = stod(config);
				_vehicle->_speed /= frameRate;
				continue;
			}
			else if (config.find(CARWHEEL) != config.npos)
			{
				config.erase(config.begin(), config.begin() + CARWHEEL.size());
				_vehicle->_rotate = stod(config);
				_vehicle->_rotate *= TO_RADDIAN;
				continue;
			}
			else if (config.find(CARPIC) != config.npos)
			{
				config.erase(config.begin(), config.begin() + CARPIC.size());
				config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				_vehicle->_texture = config;
				continue;
			}

			flag = config;
		}

		//Setting Road
		const string ROADWD = "RoadWidth";
		const string ROADPIC = "RoadPic";
		const string ROADTXT = "RoadTxt";
		const string ROADDT = "Density";
		while (flag == ROAD)
		{
			byPassSpace(in, config);
			if (config.find(ROADWD) != config.npos)
			{
				config.erase(config.begin(), config.begin() + ROADWD.size());
				_roads->_width = stod(config);
				continue;
			}
			else if (config.find(ROADPIC) != config.npos)
			{
				config.erase(config.begin(), config.begin() + ROADPIC.size());
				config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				_roads->_texture = config;
				continue;
			}
			else if (config.find(ROADTXT) != config.npos)
			{
				config.erase(config.begin(), config.begin() + ROADTXT.size());
				config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				while (!config.empty())
				{
					std::string txtRoad;
					std::size_t found_to = config.find_first_of(" ");
					if (found_to != config.npos)
					{
						std::copy(config.begin(), config.begin() + found_to, std::back_inserter(txtRoad));
					}
					else
					{
						txtRoad = config;
						config.clear();
						continue;
					}
					_roads->_roadTxt.push_back(txtRoad);
					config.erase(config.begin(), config.begin() + found_to);
					config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				}
				continue;
			}
			else if (config.find(ROADDT) != config.npos)
			{
				config.erase(config.begin(), config.begin() + ROADDT.size());
				_roads->_density = stoi(config);
				continue;
			}

			flag = config;
		}
	}

	in.close();
	return;
}

Nurbs * ReadConfig::readNurbs()
{
	ifstream filein(_filename.c_str());
	if (!filein)
	{
		cout << "File Open Failed" << endl;
		return NULL;
	}
	string config;
	getline(filein, config);
	if (config != "Endless Nurbs")
	{
		cout << "File Doesn't Match" << endl;
		filein.close();
		return NULL;
	}

	unsigned _cpcounts(0);
	unsigned _knotcounts(0);
	filein >> config;
	while (config == "ctrlpoints" && !filein.eof())
	{
		double _x, _y, _z;
		filein >> _x;
		filein >> _y;
		filein >> _z;
		_nurbs->_ctrlPoints->push_back(osg::Vec3d(_x, _y, _z));
		++_cpcounts;

		filein >> config;
	}
	while (config == "knots" && !filein.eof())
	{
		double _k;
		filein >> _k;
		unsigned counts(1);
		getline(filein, config);
		if (config != "") counts = atoi(config.c_str());
		_knotcounts += counts;
		for (unsigned int i = 0; i < counts; i++)
		{
			_nurbs->_knotVector->push_back(_k);
		}

		filein >> config;
	}
	while (config == "orders" && !filein.eof())
	{
		int order;
		filein >> order;
		if (order == -1)
		{
			_nurbs->_order = _knotcounts - _cpcounts;
		}
		else if(order != _knotcounts - _cpcounts)
		{
			osg::notify(osg::FATAL) << "Order != Knots - CtrlPoints!!!" << std::endl;
			filein.close();
			return NULL;
		}
		else
		{
			_nurbs->_order = order;
		}
		filein >> config;
	}

	filein.close();
	return _nurbs.get();
}

void ReadConfig::alignCtrlPoints(Nurbs *refNurbs)
{
	if (_nurbs)
	{
		osg::Vec3 p = O_POINT;
		osg::Vec3 direction = UP_DIR;
		if (refNurbs)
		{
			p = *(refNurbs->_ctrlPoints->rbegin());
			direction = *(refNurbs->_ctrlPoints->rbegin()) - *(refNurbs->_ctrlPoints->rbegin()+1);
		}
		osg::Vec3 v = *(_nurbs->_ctrlPoints->begin() + 1) - *(_nurbs->_ctrlPoints->begin());
		osg::Vec3 m = *(_nurbs->_ctrlPoints->begin());

		osg::Matrix align = osg::Matrix::translate(-m) * osg::Matrix::rotate(v, direction) * osg::Matrix::translate(m);
		align = align * osg::Matrix::translate(p - m);

		//add a minor offset between ctrlpoints
		align = align * osg::Matrix::translate(direction*eps_100);

		arrayByMatrix(_nurbs->_ctrlPoints, align);
	}
}

void ReadConfig::updateNurbs(osg::ref_ptr<NurbsCurve> refNB)
{
	if (_roads)
	{
		const unsigned density = _roads->_density;
		const double width = _roads->_width;

		refNB->setKnotVector(_nurbs->_knotVector);
		refNB->setDegree(_nurbs->_order - 1);
		refNB->setNumPath(density);

		refNB->setCtrlPoints(_nurbs->_ctrlPoints);
		refNB->update();
		_nurbs->_path = refNB->getPath();

		_nurbs->_ctrl_left = project_Line(_nurbs->_ctrlPoints, width);
		refNB->setCtrlPoints(_nurbs->_ctrl_left);
		refNB->update();
		_nurbs->_path_left = refNB->getPath();

		_nurbs->_ctrl_right = project_Line(_nurbs->_ctrlPoints, -width);
		refNB->setCtrlPoints(_nurbs->_ctrl_right);
		refNB->update();
		_nurbs->_path_right = refNB->getPath();

		//Adjust "Left" and "Right"
		osg::Vec3 left = *_nurbs->_ctrl_left->begin() - *_nurbs->_ctrl_right->begin();
		osg::Vec3 right = *(_nurbs->_ctrl_right->begin() + 1) - *_nurbs->_ctrl_right->begin();
		osg::Vec3 dir = right^left;
		if (dir.z() < 0)
		{
			osg::ref_ptr<osg::Vec3Array> tmp = _nurbs->_ctrl_left;
			_nurbs->_ctrl_left = _nurbs->_ctrl_right;
			_nurbs->_ctrl_right = tmp;

			tmp = _nurbs->_path_left;
			_nurbs->_path_left = _nurbs->_path_right;
			_nurbs->_path_right = tmp;
		}
	}
}