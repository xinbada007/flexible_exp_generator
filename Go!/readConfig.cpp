#include "stdafx.h"
#include "readConfig.h"
#include <fstream>
#include <string>
#include <iostream>
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
	getline(filein, config);
	if (config == "Endless Nurbs")
	{
		filein.close();
		_nurbs = new Nurbs;
		readNurbs();
		alignCtrlPoints(NULL);
		updateNurbs(new NurbsCurve);
		return;
	}

	if (config == "Trial Config")
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

void ReadConfig::readTrial()
{
	ifstream in(_filename.c_str());
	if (!in)
	{
		osg::notify(osg::FATAL) << "File Open Failed (Config)" << endl;
		return;
	}

	const string ScreenFlag = "[ScreenSettings]";
	const string CarFlag = "[CarSettings]";
	const string RoadFlag = "[RoadSettings]";

	string flag;
	getline(in, flag);
	if (flag != "Trial Config")
	{
		in.close();
		osg::notify(osg::FATAL) << "File Doesn't Match" << std::endl;
		return;
	}

	string config;
	in >> flag;
	while (flag == ScreenFlag && !in.eof())
	{
		in >> config;
		if (config == "screens:")
		{
			getline(in, config);
			std::string::iterator iter = config.begin();
			for (; iter != config.end(); iter++)
			{
				if (*iter >= '0' && *iter <= '9')
				{
					int s = *iter;
					s -= '0';
					_screens->_scrs->push_back(s);
				}
			}
			continue;
		}
		else if (config == "cameras:")
		{
			getline(in, config);
			std::string::const_iterator iter = config.cbegin();
			while (iter != config.cend() && (*iter < '9' && *iter > '0'));
			{
				iter++;
			}
			unsigned c = (iter != config.cend()) ? (*iter) - '0' : 0;
			_screens->_cameras = c;
			while (iter != config.cend() && (*iter) != ':')
			{
				iter++;
			}
			while (iter != config.cend() && ((*iter) <'9' && (*iter) >'0'))
			{
				iter++;
			}
			while (iter != config.cend())
			{
				std::string txtCamera;
				while (iter != config.cend() && (*iter) != ',')
				{
					if (((*iter)>= '0' && (*iter) <= '9') || (*iter) == '.')
					{
						txtCamera.push_back(*iter);
					}
					iter++;
				}
				(iter != config.cend()) ? iter++ : iter;
				_screens->_cam->push_back(stod(txtCamera, NULL));
			}
			if (_screens->_cameras != _screens->_cam->size())
			{
				osg::notify(osg::WARN) << "Number of Camera not Match!" << std::endl;
				return;
			}
			continue;
		}
		else if (config == "aspect:")
		{
			in >> config;
			_screens->_aspect = stod(config, NULL);
			continue;
		}
		else if (config == "background:")
		{
			in >> config;
			_screens->_background = config;
			continue;
		}
		flag = config;
	}

	//Setting Vehicle
	const double roadwidth = 0.5f;
	const double ratio = 6.0f;
	while (flag == CarFlag && !in.eof())
	{
		in >> config;
		if (config == "acceleration:")
		{
			in >> _vehicle->_acceleration;
			continue;
		}
		else if (config == "CarSpeed:")
		{
			in >> _vehicle->_speed;
			_vehicle->_speed /= frameRate;
			_vehicle->_speed *= roadwidth;
			continue;
		}
		else if (config == "CarWheel:")
		{
			in >> _vehicle->_rotate;
			_vehicle->_rotate *= 2 * PI / 360.0f;
			continue;
		}
		else if (config == "CarPic:")
		{
			in >> config;
			_vehicle->_texture = config;
			continue;
		}

		flag = config;
	}
	_vehicle->_lwratio = 2.6f;
	_vehicle->_height = getZDeepth();
	_vehicle->_width = roadwidth / ratio;
	_vehicle->_length = _vehicle->_width*_vehicle->_lwratio;
	osg::Vec3 a(_vehicle->_width / 2, -_vehicle->_length / 2, _vehicle->_height);
	osg::Vec3 b(_vehicle->_width / 2, _vehicle->_length / 2, _vehicle->_height);
	osg::Vec3 c(-_vehicle->_width / 2, _vehicle->_length / 2, _vehicle->_height);
	osg::Vec3 d(-_vehicle->_width / 2, -_vehicle->_length / 2, _vehicle->_height);
	_vehicle->_V->push_back(a); _vehicle->_V->push_back(b); _vehicle->_V->push_back(c); _vehicle->_V->push_back(d);

	while (flag == RoadFlag && !in.eof())
	{
		in >> config;
		if (config == "RoadWidth:")
		{
			in >> _roads->_width;
			continue;
		}
		else if (config == "RoadPic:")
		{
			in >> config;
			_roads->_texture = config;
			continue;
		}
		else if (config == "RoadTxt:")
		{
			getline(in, config);
			std::string::const_iterator iter = config.cbegin();
			while (iter != config.cend())
			{
				std::string txtRoad;
				while (iter != config.cend() && *iter != ';')
				{
					(*iter != ' ') ? txtRoad.push_back(*iter) : iter;
					iter++;
				}
				_roads->_roadTxt.push_back(txtRoad);
				(iter != config.cend()) ? iter++ : iter;
			}
			continue;
// 			while (iter != config.end())
// 			{
// 				if ((*iter >= 'A' && *iter <= 'Z') || (*iter >= 'a' && *iter <= 'z'))
// 					break;
// 				iter++;
// 			}
// 			if (iter == config.end())
// 			{
// 				_roads->_roadTxt.push_back("");
// 				break;
// 			}
// 
// 			std::string txt;
// 			while (iter != config.end())
// 			{
// 				if (*iter != ';')
// 				{
// 					txt.push_back(*iter);
// 				}
// 				else if (*iter == ';')
// 				{
// 					_roads->_roadTxt.push_back(txt);
// 					txt.clear();
// 				}
// 				iter++;
// 			}
// 			_roads->_roadTxt.push_back(txt);
// 			continue;
		}
		else if (config == "Density:")
		{
			in >> _roads->_density;
			continue;
		}

		flag = config;
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