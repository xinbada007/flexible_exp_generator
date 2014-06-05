#include "stdafx.h"
#include "readConfig.h"

#include <string>
#include <fstream>
#include <algorithm>
#include <io.h>
#include <direct.h>

#include <osg/Notify>
#include <osg/Geometry>
#include <osgDB/ReadFile>

using namespace std;

// ReadConfig::ReadConfig():
// _filename("")
// {
// 	_nurbs = new Nurbs;
// 
// 	_screens = new Screens;
// 	_vehicle = new Vehicle;
// 	_roads = new RoadSet;
// 	_camset = new CameraSet;
// 	_subjects = new Subjects;
// }

ReadConfig::ReadConfig(const std::string &filename):
_filename(filename)
{
	assignConfig();
}

ReadConfig::~ReadConfig()
{
	std::cout << "Deconstruct ReadConfig" << std::endl;
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

	const string TRAIL = "TRIALCONFIG";
	string config;
	byPassSpace(filein, config);
	const string title = getTillFirstSpaceandToUpper(config);
	if (title == TRAIL)
	{
		_screens = new Screens;
		_vehicle = new Vehicle;
		_roads = new RoadSet;
		_camset = new CameraSet;
		_subjects = new Subjects;
		
		//read trial config
		readTrial(filein);
		initializeAfterReadTrial();
		return;
	}
}

void ReadConfig::initializeAfterReadTrial()
{
	//Initialize Screen
	osg::GraphicsContext::WindowingSystemInterface *wsi;
	wsi = osg::GraphicsContext::getWindowingSystemInterface();
	osg::GraphicsContext::ScreenSettings ss;

	if (!_screens->_aspect)
	{
		int min_height = INT_MAX;
		int sum_width = 0;
		for (unsigned i = 0; i < _screens->_scrs->getNumElements(); i++)
		{
			unsigned si = *(_screens->_scrs->begin() + i);
			wsi->getScreenSettings(si, ss);
			min_height = ss.height < min_height ? ss.height : min_height;
			sum_width += ss.width;
		}
		_screens->_aspect = (double)(sum_width) / (double)(min_height);
	}

	//Initialize Vehicle
	const double halfW = _vehicle->_width * 0.5f;
	const double halfL = _vehicle->_length * 0.5f;
	osg::Vec3d right_bottom(halfW, -halfL, _vehicle->_height);
	osg::Vec3d right_top(halfW, halfL, _vehicle->_height);
	osg::Vec3d left_top(-halfW, halfL, _vehicle->_height);
	osg::Vec3d left_bottom(-halfW, -halfL, _vehicle->_height);
	_vehicle->_V->clear();
	_vehicle->_V->push_back(right_bottom); _vehicle->_V->push_back(right_top);
	_vehicle->_V->push_back(left_top); _vehicle->_V->push_back(left_bottom);
	_alignPoint = (right_bottom + left_bottom) * 0.5f;
	_alignPoint += (_alignPoint - _vehicle->_O) * eps_100;
	_alignPoint.z() = 0.0f;

	//convert speed from KM/H to M/S
	_vehicle->_speed /= 3.6f;
	_vehicle->_speed /= frameRate;

	//Initialize Roads
	_roads->_width = _roads->_roadLane * _roads->_laneWidth;
	//generate CtrlPoints from Txt file
	stringList::const_iterator i = _roads->_roadTxt.cbegin();
	while (i != _roads->_roadTxt.cend())
	{
		_filename = *i++;
		_nurbs = new Nurbs;
		readNurbs();
		scaleCtrlPoints();
		Nurbs *prve = (_roads->_nurbs.empty() ? NULL : _roads->_nurbs.back());
		alignCtrlPoints(prve);
		updateNurbs(new NurbsCurve);
		_roads->_nurbs.push_back(_nurbs.release());
	}
	//load the pic as texture
	_roads->_imgRoad = osgDB::readImageFile(_roads->_texture);
	_roads->_imgWall = osgDB::readImageFile(_roads->_textureWall);
	if (!_roads->_imgRoad.valid())
	{
		osg::notify(osg::WARN) << "Unable to load Road texture file. Exiting." << std::endl;
		_roads->_imgRoad = NULL;
	}
	if (!_roads->_imgWall.valid())
	{
		osg::notify(osg::WARN) << "Unable to load Wall texture file. Exiting." << std::endl;
		_roads->_imgWall = NULL;
	}

	//Initialize Camera
	if (_camset->_offset->empty())
	{
		//set X offset
		_camset->_offset->push_back(0.0f);
		//set Y offset
		_camset->_offset->push_back(-_screens->_horDistance);
		//set Z offset
		_camset->_offset->push_back(_screens->_verDistance + _screens->_verOffset);
	}

	//Initialize Subjects
	if (_subjects)
	{
		char timestr[10], datestr[10];
		_strtime_s(timestr, sizeof(timestr));
		_strdate_s(datestr, sizeof(datestr));
		timestr[2] = '-', timestr[5] = '-';
		datestr[2] = '-', datestr[5] = '-';

		const string timedir = timestr;
		const string datedir = datestr;
		const string dirPath = "..\\" + _subjects->getName();
		const string path = dirPath + ".\\" + datestr + '-' + timedir;
		if (_access(dirPath.c_str(), 6) == -1) //if doesn't find the directory
		{
			if (_mkdir(dirPath.c_str()) == -1)//creat one
			{
				osg::notify(osg::FATAL) << "Cannot create folder" << endl;
			}
		}
		if (_access((path).c_str(), 6) == -1)
		{
			if (_mkdir((path).c_str()) == -1)//creat sub dir based on time
			{
				osg::notify(osg::FATAL) << "Cannot create folder" << endl;
			}
		}
		const string out = path + "\\" + _subjects->getName() + ".txt";
		fstream fileout(out.c_str(), ios::out);
		ifstream filein(_subjects->getFilePath().c_str());
		if (!fileout.is_open() || !filein)
		{
			osg::notify(osg::FATAL) << "Cannot copy config file" << std::endl;
			fileout.close();
			filein.close();
		}
		else
		{
			filein.seekg(0, std::ios::beg);
			fileout << filein.rdbuf();
			_subjects->setRecPath(path);
			filein.close();
			fileout.close();
		}
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

std::string ReadConfig::getTillFirstSpaceandToUpper(std::string &content)
{
	std::string flag = "";
	const string SPACE = " ";
	std::size_t to = content.find_first_of(SPACE);
	if (to != content.npos)
	{
		std::transform(content.begin(), content.begin() + to, std::back_inserter(flag),toupper);
	}
	else
	{
		std::transform(content.begin(), content.end(), std::back_inserter(flag), toupper);;
	}

	std::string::const_iterator i = flag.cbegin();
	bool isLetter(false);
	while (i != flag.cend())
	{
		isLetter = isLetter || isletter(*i);
		i++;
	}
	if (!isLetter)
	{
		flag.clear();
		flag = "";
	}

	return flag;
}

void ReadConfig::readTrial(ifstream &in)
{
	const string SCREEN = "[SCREENSETTINGS]";
	const string CAR = "[CARSETTINGS]";
	const string ROAD = "[ROADSETTINGS]";
	const string CAMERA = "[CAMERASETTINGS]";
	const string SUBJECTS = "[SUBJECTS]";

	string flag;
	if (!in.eof())
	{
		byPassSpace(in, flag);
		flag = getTillFirstSpaceandToUpper(flag);
	}
	while (!in.eof())
	{
		string config;

		//Set Subjects' Name
		const string NAME = "NAME";
		const string AGE = "AGE";
		const string GENDER = "GENDER";
		const string DRIVING = "DRIVING";
		while (flag == SUBJECTS && !in.eof())
		{
			byPassSpace(in, config);
			const string title = getTillFirstSpaceandToUpper(config);
			_subjects->setFilePath(_filename);
			if (title == NAME)
			{
				config.erase(config.begin(), config.begin() + NAME.size());
				_subjects->setName(config);
				continue;
			}
			else if (title == AGE)
			{
				config.erase(config.begin(), config.begin() + AGE.size());
				if (!config.empty())
				{
					_subjects->setAge(stoi(config));
				}
				continue;
			}
			else if (title == GENDER)
			{
				config.erase(config.begin(), config.begin() + GENDER.size());
				_subjects->setGender(config);
				continue;
			}
			else if (title == DRIVING)
			{
				config.erase(config.begin(), config.begin() + DRIVING.size());
				if (!config.empty())
				{
					_subjects->setDriving(stod(config));
				}
				continue;
			}
			flag = config;
			flag = getTillFirstSpaceandToUpper(flag);
			continue;
		}

		//Set Screen
		const string SCR = "SCREEN";
		const string ASPECT = "ASPECT";
		const string REALWORD = "REALWORLD";
		const string BGPIC = "BACKGROUND";
		const string HDISTANCE = "HDISTANCE";
		const string VDISTANCE = "VDISTANCE";
		const string VOFFSET = "VOFFSET";
		const string FOVY = "FOVY";
		const string DISTOR = "DISTORTION";
		while (flag == SCREEN && !in.eof())
		{
			byPassSpace(in, config);
			const string title = getTillFirstSpaceandToUpper(config);
			if (title == SCR)
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
			else if (title == ASPECT)
			{
				config.erase(config.begin(), config.begin() + ASPECT.size());
				_screens->_aspect = stod(config);
				continue;
			}
			else if (title == REALWORD)
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
			else if (title == BGPIC)
			{
				config.erase(config.begin(), config.begin() + BGPIC.size());
				config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				_screens->_background = config;
				continue;
			}
			else if (title == HDISTANCE)
			{
				config.erase(config.begin(),config.begin()+HDISTANCE.size());
				if (!config.empty())
				{
					_screens->_horDistance = stod(config);
				}
				continue;
			}
			else if (title == VDISTANCE)
			{
				config.erase(config.begin(), config.begin() + VDISTANCE.size());
				if (!config.empty())
				{
					_screens->_verDistance = stod(config);
				}
				continue;
			}
			else if (title == VOFFSET)
			{
				config.erase(config.begin(), config.begin() + VOFFSET.size());
				if (!config.empty())
				{
					_screens->_verOffset = stod(config);
				}
				continue;
			}
			else if (title == FOVY)
			{
				config.erase(config.begin(), config.begin() + FOVY.size());
				if (!config.empty())
				{
					_screens->_fovy = stod(config);
				}
				continue;
			}
			else if (title == DISTOR)
			{
				config.erase(config.begin(), config.begin() + DISTOR.size());
				if (!config.empty())
				{
					_screens->_distortion = stod(config);
				}
				continue;
			}

			flag = config;
			flag = getTillFirstSpaceandToUpper(flag);
			continue;
		}

		//Set Vehicle
		const string ACCEL = "ACCELERATION";
		const string CARSPEED = "CARSPEED";
		const string CARWHEEL = "CARWHEEL";
		const string CARPIC = "CARPIC";
		const string CARWIDTH = "CARWIDTH";
		const string CARHEIGHT = "CARHEIGHT";
		const string CARLENGTH = "CARLENGTH";
		const string WHEELACCL = "WHEELACCL";
		while (flag == CAR && !in.eof())
		{
			byPassSpace(in, config);
			const string title = getTillFirstSpaceandToUpper(config);
			if (title == ACCEL)
			{
				config.erase(config.begin(), config.begin() + ACCEL.size());
				_vehicle->_acceleration = (stoi(config) == 1) ? true : false;
				continue;
			}
			else if (title == CARSPEED)
			{
				config.erase(config.begin(), config.begin() + CARSPEED.size());
				_vehicle->_speed = stod(config);
				continue;
			}
			else if (title == CARWHEEL)
			{
				config.erase(config.begin(), config.begin() + CARWHEEL.size());
				_vehicle->_rotate = stod(config);
				_vehicle->_rotate *= TO_RADDIAN;
				continue;
			}
			else if (title == CARPIC)
			{
				config.erase(config.begin(), config.begin() + CARPIC.size());
				config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				_vehicle->_texture = config;
				continue;
			}
			else if (title == CARWIDTH)
			{
				config.erase(config.begin(), config.begin() + CARWIDTH.size());
				if (!config.empty())
				{
					_vehicle->_width = stod(config);
				}
				continue;
			}
			else if (title == CARHEIGHT)
			{
				config.erase(config.begin(), config.begin() + CARHEIGHT.size());
				if (!config.empty())
				{
					_vehicle->_height = stod(config);
				}
				continue;
			}
			else if (title == CARLENGTH)
			{
				config.erase(config.begin(), config.begin() + CARLENGTH.size());
				if (!config.empty())
				{
					_vehicle->_length = stod(config);
				}
				continue;
			}
			else if (title == WHEELACCL)
			{
				config.erase(config.begin(), config.begin() + WHEELACCL.size());
				if (!config.empty())
				{
					_vehicle->_rotationAccl = stod(config);
					_vehicle->_rotationAccl *= TO_RADDIAN;
				}
				continue;
			}

			flag = config;
			flag = getTillFirstSpaceandToUpper(flag);
			continue;
		}

		//Setting Road
		const string ROADPIC = "ROADPIC";
		const string ROADTXT = "ROADTXT";
		const string ROADDT = "DENSITY";
		const string WALLH = "WALLHEIGHT";
		const string ROADLANE = "ROADLANES";
		const string WALLPIC = "WALLPIC";
		const string SCALE = "SCALEVECTOR";
		while (flag == ROAD && !in.eof())
		{
			byPassSpace(in, config);
			const string title = getTillFirstSpaceandToUpper(config);
			if (title == ROADLANE)
			{
				config.erase(config.begin(), config.begin() + ROADLANE.size());
				if (!config.empty())
				{
					_roads->_roadLane = stod(config);
				}
				continue;
			}
			else if (title == ROADPIC)
			{
				config.erase(config.begin(), config.begin() + ROADPIC.size());
				config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				_roads->_texture = config;
				continue;
			}
			else if (title == ROADTXT)
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
					}
					_roads->_roadTxt.push_back(txtRoad);
					config.erase(config.begin(), config.begin() + found_to);
					config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				}
				continue;
			}
			else if (title == ROADDT)
			{
				config.erase(config.begin(), config.begin() + ROADDT.size());
				if (!config.empty())
				{
					_roads->_density = stoi(config);
				}
				continue;
			}
			else if (title == WALLH)
			{
				config.erase(config.begin(), config.begin() + WALLH.size());
				if (!config.empty())
				{
					_roads->_wallHeight = stod(config);
				}
				continue;
			}
			else if (title == SCALE)
			{
				config.erase(config.begin(), config.begin() + SCALE.size());
				std::string::size_type sz;
				std::vector<double> scale;
				while (!config.empty())
				{
					scale.push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				if (scale.size() == osg::Vec3d::num_components)
				{
					_roads->_scale.x() = scale.at(0);
					_roads->_scale.y() = scale.at(1);
					_roads->_scale.z() = scale.at(2);
				}
			}
			else if (title == WALLPIC)
			{
				config.erase(config.begin(), config.begin() + WALLPIC.size());
				config.erase(config.begin(), config.begin() + config.find_first_not_of(" "));
				_roads->_textureWall = config;
				continue;
			}

			flag = config;
			flag = getTillFirstSpaceandToUpper(flag);
			continue;
		}

		//Setting Camera
		const string CAM_OFFSET("OFFSET");
		const string CAM_EYE_TRACKER("EYETRACKER");
		while (flag == CAMERA && !in.eof())
		{
			byPassSpace(in, config);
			const string title = getTillFirstSpaceandToUpper(config);
			if (title == CAM_OFFSET)
			{
				config.erase(config.begin(), config.begin() + CAM_OFFSET.size());

				osg::ref_ptr<osg::DoubleArray> offset = new osg::DoubleArray;
				while (!config.empty())
				{
					std::string::size_type sz;
					offset->push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				if (offset->size() == 3)
				{
					_camset->_offset = offset.release();
				}
				continue;
			}
			else if (title == CAM_EYE_TRACKER)
			{
				config.erase(config.begin(), config.begin() + CAM_EYE_TRACKER.size());
				if (!config.empty())
				{
					_camset->_eyeTracker = (stoi(config) == 1);
				}
				continue;
			}

			flag = config;
			flag = getTillFirstSpaceandToUpper(flag);
			continue;
		}

		if (flag.empty() && !in.eof())
		{
			byPassSpace(in, flag);
			flag = getTillFirstSpaceandToUpper(flag);
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

	const string ENDLESS = "ENDLESSNURBS";
	string config;
	byPassSpace(filein, config);
	const string title = getTillFirstSpaceandToUpper(config);
	if (title != ENDLESS)
	{
		cout << "File Doesn't Match" << endl;
		filein.close();
		return NULL;
	}

	const string CTRLPOINT = "CTRLPOINTS";
	const string KNOTS = "KNOTS";
	const string ORDERS = "orders";
	string flag;
	while (!filein.eof())
	{
		byPassSpace(filein, flag);
		string title = getTillFirstSpaceandToUpper(flag);
		while (title == CTRLPOINT && !filein.eof())
		{
			byPassSpace(filein, config);
			if (!config.empty())
			{
				if (isletter(config.front()))
				{
					title = getTillFirstSpaceandToUpper(config);
					continue;;
				}

				double x, y, z(0.0f);
				std::string::size_type sz;
				x = stod(config, &sz);
				y = stod(config.substr(sz));
				_nurbs->_ctrlPoints->push_back(osg::Vec3d(x, y, z));
			}
		}

		while (title == KNOTS && !filein.eof())
		{
			byPassSpace(filein, config);
			if (!config.empty())
			{
				if (isletter(*config.begin()))
				{
					title = getTillFirstSpaceandToUpper(config);
					continue;;
				}
				std::vector<double> knot;
				while (!config.empty())
				{
					std::string::size_type sz;
					knot.push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				if (knot.empty()) continue;
				if (knot.size() == 1) knot.push_back(1);
				for (int i = 0; i < knot.back(); i++)
				{
					_nurbs->_knotVector->push_back(knot.front());
				}
			}
		}
	}

	_nurbs->_order = _nurbs->_knotVector->size() - _nurbs->_ctrlPoints->size();
	filein.close();
	return _nurbs.get();
}

void ReadConfig::scaleCtrlPoints()
{
	if (_nurbs)
	{
		arrayByMatrix(_nurbs->_ctrlPoints, osg::Matrix::scale(_roads->_scale));
	}
}

void ReadConfig::alignCtrlPoints(Nurbs *refNurbs)
{
	if (_nurbs)
	{
		osg::Vec3d p = _alignPoint;
		osg::Vec3d direction = UP_DIR;
		if (refNurbs)
		{
			p = *(refNurbs->_ctrlPoints->rbegin());
			direction = *(refNurbs->_ctrlPoints->rbegin()) - *(refNurbs->_ctrlPoints->rbegin()+1);
		}
		osg::Vec3d v = *(_nurbs->_ctrlPoints->begin() + 1) - *(_nurbs->_ctrlPoints->begin());
		osg::Vec3d m = *(_nurbs->_ctrlPoints->begin());

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

		refNB->setKnotVector(_nurbs->_knotVector);
		refNB->setDegree(_nurbs->_order - 1);
		refNB->setNumPath(density);

		refNB->setCtrlPoints(_nurbs->_ctrlPoints);
		refNB->update();
		_nurbs->_path = refNB->getPath();

		const double width = _roads->_width;
		const double halfW = width * 0.5f;

		_nurbs->_ctrl_left = project_Line(_nurbs->_ctrlPoints, halfW);
		refNB->setCtrlPoints(_nurbs->_ctrl_left);
		refNB->update();
		_nurbs->_path_left = refNB->getPath();

		_nurbs->_ctrl_right = project_Line(_nurbs->_ctrlPoints, -halfW);
		refNB->setCtrlPoints(_nurbs->_ctrl_right);
		refNB->update();
		_nurbs->_path_right = refNB->getPath();

		//Adjust "Left" and "Right"
		osg::Vec3d left = *_nurbs->_ctrl_left->begin() - *_nurbs->_ctrl_right->begin();
		osg::Vec3d right = *(_nurbs->_ctrl_right->begin() + 1) - *_nurbs->_ctrl_right->begin();
		osg::Vec3d dir = right^left;
		if (dir.z() < 0)
		{
			osg::ref_ptr<osg::Vec3dArray> tmp = _nurbs->_ctrl_left;
			_nurbs->_ctrl_left = _nurbs->_ctrl_right;
			_nurbs->_ctrl_right = tmp;

			tmp = _nurbs->_path_left;
			_nurbs->_path_left = _nurbs->_path_right;
			_nurbs->_path_right = tmp;
		}
	}
}

osg::Geode* ReadConfig::measuer()
{
	osg::Vec3d vector;
	vector.z() = _camset->_offset->at(2)*1.2;
	vector.x() = 0.0f;
	vector.y() = 0.0f;
	//vector.y() += 2.65f;

	osg::Vec3d A = vector;
	A.x() += -1.5f;
	osg::Vec3d B = vector;
	B.x() += 1.5f;
	
	osg::Vec3d AA = vector;
	AA.z() += -1.5;
	osg::Vec3d BB = vector;
	BB.z() += 1.5f;

	osg::Vec3 offset;
	offset.set(_camset->_offset->at(0), _camset->_offset->at(1), _camset->_offset->at(2));
	
	osg::Vec3 OA(A), OB(B), OAA(AA), OBB(BB);
	double x = ((OA + OB) / 2 - offset).length();
	  
	osg::Vec3 test = offset - (A + B) / 2;
	test.set(0.0f, -offset.y(), 0.0f);
	osg::Matrix mm = osg::Matrix::translate(test) * osg::Matrix::rotate(45 * TO_RADDIAN, osg::Vec3(0.0, 0.0, 1.0f)) * osg::Matrix::translate(-test);
	A = A * mm;
	B = B * mm;
	AA = AA * mm;
	BB = BB * mm;
	x = ((A + B) / 2 - offset).length();
	x = ((AA + BB) / 2 - offset).length();

	osg::Vec3 RA(OA), RB(OB), RAA(OAA), RBB(OBB);
	mm = osg::Matrix::translate(test) *  osg::Matrix::rotate(-45 * TO_RADDIAN, osg::Vec3(0.0, 0.0, 1.0f)) * osg::Matrix::translate(-test);
	RA = RA*mm;
	RB = RB*mm;
	RAA = RAA*mm;
	RBB = RBB*mm;
	x = ((RA + RB) / 2 - offset).length();
	x = ((RAA + RBB) / 2 - offset).length();

	_measuerment = new osg::Vec3dArray;
	_measuerment->push_back(A);
	_measuerment->push_back(B);
	_measuerment->push_back(AA);
	_measuerment->push_back(BB);

	_measuerment->push_back(OA);
	_measuerment->push_back(OB);
	_measuerment->push_back(OAA);
	_measuerment->push_back(OBB);

	_measuerment->push_back(RA);
	_measuerment->push_back(RB);
	_measuerment->push_back(RAA);
	_measuerment->push_back(RBB);

	osg::Geometry *gm = new osg::Geometry;
	gm->setVertexArray(_measuerment);
	osg::Vec4Array *color = new osg::Vec4Array;
	color->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	gm->setColorArray(color);
	gm->setColorBinding(osg::Geometry::BIND_OVERALL);
	gm->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, _measuerment->size()));
	gm->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	osg::Geode *gd = new osg::Geode;
	gd->addDrawable(gm);

	return gd;
}