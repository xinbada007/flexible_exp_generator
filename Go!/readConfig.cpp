#include "stdafx.h"
#include "readConfig.h"
#include "obstacle.h"

#include <string>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <io.h>
#include <direct.h>

#include <osg/Notify>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osg/MatrixTransform>
#include <osgUtil/Optimizer>
#include <osg/PolygonMode>
#include <osg/Light>
#include <osg/LightSource>

#include <sisl.h>

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
_filename(filename), _replay(false), _vehicle(NULL), _screens(NULL),
_roads(NULL), _camset(NULL), _subjects(NULL), _saveState(NULL), _dynamicState(NULL)
{
	assignConfig();
}

ReadConfig::ReadConfig(const std::string &config, const std::string &replay):
_replay(true), _vehicle(NULL), _screens(NULL),_roads(NULL), 
_camset(NULL), _subjects(NULL), _saveState(NULL), _dynamicState(NULL)
{
	_filename = config;
	assignConfig();

	_filename = replay;
	assignConfig();
}

ReadConfig::~ReadConfig()
{
	std::cout << "Deconstruct ReadConfig" << std::endl;

	_vehicle = NULL;
	_screens = NULL;
	_roads = NULL;
	_camset = NULL;
	_subjects = NULL;
	_experiment = NULL;
	_saveState = NULL;
	_dynamicState = NULL;
	_measuerment = NULL;
}

void ReadConfig::assignConfig()
{
	ifstream filein(_filename.c_str());

	if (!filein)
	{
		cout << "File Open Failed" << endl;
		cout << _filename << " does not exist!" << endl;
		return;
	}

	const string TRAIL = "TRIALCONFIG";
	string config;
	byPassSpace(filein, config);
	const string title = getTillFirstSpaceandToUpper(config);
	const string REPLAY = "TRIALREPLAY";
	if (title == TRAIL)
	{
		_screens = new Screens;
		_vehicle = new Vehicle;
		_roads = new RoadSet;
		_camset = new CameraSet;
		_subjects = new Subjects;
		_experiment = new Experiment;

		//read trial config
		readTrial(filein);
		initializeAfterReadTrial();
		return;
	}
	else if (title == REPLAY)
	{
		_saveState = new osg::MatrixdArray;
		_dynamicState = new osg::IntArray;
		readReply(filein);
		_replay = true;
		return;
	}
	else
		filein.close();
}

void ReadConfig::readReply(std::ifstream &filein)
{
	const string FRAME = "FRAME";
	const string DYNAMIC = "DYNAMIC";
	string title;
	string config;
	osg::Matrixd m;
	while (!filein.eof())
	{
		byPassSpace(filein, config);
		title = getTillFirstSpaceandToUpper(config);
		if (title == FRAME)
		{
			string sNum;
			for (int i = 0; i < 4;i++)
			{
				byPassSpace(filein, sNum);
				for (int j = 0; j < 4;j++)
				{
					std::string::size_type sz;
					double num = stod(sNum, &sz);
					m(i, j) = num;
					sNum.erase(sNum.begin(), sNum.begin() + sz);
				}
			}
			_saveState->push_back(m);
			continue;
		}
		else if (title == DYNAMIC)
		{
			config.erase(config.begin(), config.begin() + DYNAMIC.size());
			if (!config.empty())
			{
				int num = stoi(config);
				_dynamicState->push_back(num);
			}
		}
	}
	return;
}

void ReadConfig::initializeAfterReadTrial()
{
	//Initialize Subjects
	//have to be first executed
	if (_subjects)
	{
		if (!_subjects->_roads.empty())
		{
			_roads->_roadTxt.clear();
			_roads->_roadTxt = _subjects->_roads;
		}
		else
		{
			const int numRoads = _roads->_roadTxt.size();
			if (numRoads > 1 && _subjects->getRandomRoads())
			{
				srand((unsigned)time(NULL));
				std::vector<int> roads;
				while (roads.size() != numRoads)
				{
					int r = rand() % numRoads;
					bool drop(false);
					std::vector<int>::const_iterator i = roads.cbegin();
					while (i != roads.cend())
					{
						if (r == *i)
						{
							drop = true;
							break;
						}
						i++;
					}
					if (!drop)
					{
						roads.push_back(r);
					}
				}

				std::vector<int>::const_iterator iroads = roads.cbegin();
				stringList newRoadTxt;
				while (newRoadTxt.size() != _roads->_roadTxt.size())
				{
					newRoadTxt.push_back(_roads->_roadTxt[*iroads++]);
				}

				_roads->_roadTxt.clear();
				_roads->_roadTxt = newRoadTxt;
				newRoadTxt.clear();
			}
		}

		if (!_replay)
		{
			const string subD = "..\\Subjects";
			if (!osgDB::makeDirectory(subD))
			{
				osg::notify(osg::FATAL) << "Cannot create folder\n" << subD << std::endl;
				return;
			}
			const string dirPath = subD + "\\" + _subjects->getName() + "\\" + _subjects->getDirectoryName();
			if (!osgDB::makeDirectory(dirPath))
			{
				osg::notify(osg::FATAL) << "Cannot create folder\n" << dirPath << std::endl;
				return;
			}
			string path;
			path = dirPath + "\\" + _subjects->getTrialName();
			if (!osgDB::makeDirectory(path))
			{
				osg::notify(osg::FATAL) << "Cannot create folder\n" << path << std::endl;
				return;
			}
			const string out = path + "\\" + _subjects->getName() + ".txt";
			fstream fileout(out.c_str(), ios::out);
			ifstream filein(_subjects->getFilePath().c_str());
			if (!fileout.is_open() || !filein)
			{
				osg::notify(osg::FATAL) << "Cannot copy config file" << std::endl;
				fileout.close();
				filein.close();
				return;
			}
			else
			{
				filein.seekg(0, std::ios::beg);
				fileout << filein.rdbuf();
				stringList::const_iterator iRoads = _roads->_roadTxt.cbegin();
				while (iRoads != _roads->_roadTxt.cend())
				{
					if (iRoads == _roads->_roadTxt.cbegin())
					{
						fileout << "\n" << "#" << "Roads:\t";
					}
					fileout << *iRoads << ";";
					iRoads++;
				}
				_subjects->setRecPath(path);
				filein.close();
				fileout.close();
			}
		}
	}

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
	if (_screens->_realworld->empty())
	{
		_screens->_hFov = 2.0f*atan(_screens->_aspect*tan(_screens->_fovy*0.5f*TO_RADDIAN)) / TO_RADDIAN;
	}

	if (osgDB::fileExists(_screens->_background))
	{
		_screens->_imgBg = osgDB::readImageFile(_screens->_background);
		if (!_screens->_imgBg.valid())
		{
			osg::notify(osg::WARN) << "Unable to load Background texture file. Exiting." << std::endl;
			_screens->_imgBg = NULL;
		}
	}	

	//Initialize Vehicle
	osg::ref_ptr<osg::Node> carNode = osgDB::readNodeFile(_vehicle->_carModel);
	if (carNode)
	{
		BoundingboxVisitor bbV;
		carNode->accept(bbV);
		const osg::BoundingBox &bb = bbV.getBoundingBox();
		_vehicle->_width = bbV.getDimention_X();
		_vehicle->_length = bbV.getDimention_Y();
		osg::ref_ptr<osg::Vec3dArray> zMin = new osg::Vec3dArray;
		osg::ref_ptr<osg::Vec3dArray> zMax = new osg::Vec3dArray;
		getCCWCubefromLBfromBBox(bb, zMin, zMax);

		osg::Vec3d left_bottom(zMin->at(0));
		osg::Vec3d left_top(zMin->at(1));
		osg::Vec3d right_top(zMin->at(2));
		osg::Vec3d right_bottom(zMin->at(3));
		_vehicle->_V->clear();
		_vehicle->_V->push_back(right_bottom); _vehicle->_V->push_back(right_top);
		_vehicle->_V->push_back(left_top); _vehicle->_V->push_back(left_bottom);
		_alignPoint = (right_bottom + left_bottom) * 0.5f;
		_alignPoint += (_alignPoint - _vehicle->_O) * eps_100;
		_alignPoint.z() = 0.0f;

 		{
// 			osg::ref_ptr<osg::StateSet> ss = carNode->getOrCreateStateSet();
// 			ss->setMode(GL_LIGHTING, osg::StateAttribute::ON);
// 			ss->setMode(GL_LIGHT0, osg::StateAttribute::ON);
// 			ss->setMode(GL_LIGHT1, osg::StateAttribute::ON);
// 
// 			osg::ref_ptr<osg::Light> light = new osg::Light;
// 			light->setLightNum(1);
// 			light->setDiffuse(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
// 			light->setAmbient(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
// 			light->setConstantAttenuation(0.8f);
// 			light->setPosition(osg::Vec4(0.0, 0.1f, bbV.getDimention_Z()*0.9f, 1.0f));
// 			osg::ref_ptr<osg::LightSource> lightsource = new osg::LightSource;
// 			lightsource->setLight(light);
// 			lightsource->setStateSetModes(*ss, osg::StateAttribute::ON);
// 
// 			carNode->asGroup()->addChild(lightsource);
// 			osgDB::writeNodeFile(*carNode, "test.osg");
		}

		_vehicle->_carNode = carNode.release();
	}
	else
	{
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
	}
	//convert speed from KM/H to M/S
	_vehicle->_speed /= 3.6f;
	_vehicle->_speedincr /= 3.6f;

	//Initialize Roads
	_roads->_width = _roads->_roadLane * _roads->_laneWidth;
	//generate CtrlPoints from Txt file
	stringList::const_iterator i = _roads->_roadTxt.cbegin();
	while (i != _roads->_roadTxt.cend())
	{
		_filename = *i++;
		osg::ref_ptr<Nurbs> thisNurbs = readNurbs(_roads->_length);
		Nurbs *prveNurbs = (_roads->_nurbs.empty() ? NULL : _roads->_nurbs.back());
		alignCtrlPoints(thisNurbs, prveNurbs);
		_roads->_nurbsMethod == 1 ? updateNurbs(thisNurbs, _roads->_density,_roads->_width) : updateNurbs(thisNurbs, new NurbsCurve, _roads->_density, _roads->_width);
		
		if (_roads->_wallRoadWidth)
		{
			osg::ref_ptr<Nurbs> newNurbs = new Nurbs(*thisNurbs);
			newNurbs->_path_left->clear();
			newNurbs->_path_right->clear();
			_roads->_nurbsMethod == 1 ? updateNurbs(newNurbs, _roads->_density, _roads->_wallRoadWidth) : updateNurbs(newNurbs, new NurbsCurve, _roads->_density, _roads->_wallRoadWidth);
			thisNurbs->_wall_left = newNurbs->_path_left.release();
			thisNurbs->_wall_right = newNurbs->_path_right.release();
		}
		
		_roads->_nurbs.push_back(thisNurbs.release());
	}
	//load the pic as texture
	if (osgDB::fileExists(_roads->_texture))
	{
		_roads->_imgRoad = osgDB::readImageFile(_roads->_texture);
		if (!_roads->_imgRoad.valid())
		{
			osg::notify(osg::WARN) << "Unable to load Road texture file. Exiting." << std::endl;
			_roads->_imgRoad = NULL;
		}
	}
	if (osgDB::fileExists(_roads->_textureWall))
	{
		_roads->_imgWall = osgDB::readImageFile(_roads->_textureWall);
		if (!_roads->_imgWall.valid())
		{
			osg::notify(osg::WARN) << "Unable to load Wall texture file. Exiting." << std::endl;
			_roads->_imgWall = NULL;
		}
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

	//Initialize Experiment
	if (osgDB::fileExists(_experiment->_dynamicPic))
	{
		_experiment->_imgDynamic = osgDB::readImageFile(_experiment->_dynamicPic);
		if (!_experiment->_imgDynamic.valid())
		{
			//		osg::notify(osg::WARN) << "Unable to load Dynamic texture file. Exiting." << std::endl;
			_experiment->_imgDynamic = NULL;
		}
	}
	//Initialize Obstacles
	const unsigned &numObs = _experiment->_obstaclesTime->size();
	if (_experiment->_obstacleRange->size() != numObs)
	{
		const unsigned numRange = _experiment->_obstacleRange->getNumElements();
		_experiment->_obstacleRange->resize(numObs);
		if (numObs > numRange)
		{
			osg::DoubleArray::const_iterator assign_start = _experiment->_obstacleRange->begin();
			osg::DoubleArray::const_iterator assign_end = assign_start + numRange;
			osg::DoubleArray::iterator i = _experiment->_obstacleRange->begin() + numRange;
			while (i != _experiment->_obstacleRange->end())
			{
				if (assign_start == assign_end)
				{
					assign_start = _experiment->_obstacleRange->begin();
				}
				*i = *assign_start;
				++i;
				++assign_start;
			}
		}
		osg::notify(osg::NOTICE) << "Obstacles Range Resized" << std::endl;
	}
	if (_experiment->_obstaclePos->size() != numObs)
	{
		const unsigned numPos = _experiment->_obstaclePos->getNumElements();
		_experiment->_obstaclePos->resize(numObs);
		if (numObs > numPos)
		{
			osg::IntArray::const_iterator assign_start = _experiment->_obstaclePos->begin();
			osg::IntArray::const_iterator assign_end = assign_start + numPos;
			osg::IntArray::iterator i = _experiment->_obstaclePos->begin() + numPos;
			while (i != _experiment->_obstaclePos->end())
			{
				if (assign_start == assign_end)
				{
					assign_start = _experiment->_obstaclePos->begin();
				}
				*i = *assign_start;
				++i;
				++assign_start;
			}
		}
		osg::notify(osg::NOTICE) << "Obstacles Position Resized" << std::endl;
	}
	if (_experiment->_obsPosOffset->size() != numObs)
	{
		const unsigned numOffset = _experiment->_obsPosOffset->getNumElements();
		_experiment->_obsPosOffset->resize(numObs);
		if (numObs > numOffset)
		{
			osg::DoubleArray::const_iterator assign_start = _experiment->_obsPosOffset->begin();
			osg::DoubleArray::const_iterator assign_end = assign_start + numOffset;
			osg::DoubleArray::iterator i = _experiment->_obsPosOffset->begin() + numOffset;
			while (i != _experiment->_obsPosOffset->end())
			{
				if (assign_start == assign_end)
				{
					assign_start = _experiment->_obsPosOffset->begin();
				}
				*i = *assign_start;
				++i;
				++assign_start;
			}
		}
		osg::notify(osg::NOTICE) << "Obstacles PosOffset Resized" << std::endl;
	}
	if (_experiment->_obsCollision->getNumElements() != numObs)
	{
		const unsigned numCollision = _experiment->_obsCollision->getNumElements();
		_experiment->_obsCollision->resize(numObs);
		if (numObs > numCollision)
		{
			osg::UIntArray::const_iterator assign_start = _experiment->_obsCollision->begin();
			osg::UIntArray::const_iterator assign_end = assign_start + numCollision;
			osg::UIntArray::iterator i = _experiment->_obsCollision->begin() + numCollision;
			while (i != _experiment->_obsCollision->end())
			{
				if (assign_start == assign_end)
				{
					assign_start = _experiment->_obsCollision->begin();
				}
				*i = *assign_start;
				++i;
				++assign_start;
			}
		}
		osg::notify(osg::NOTICE) << "Obstacles Collision Detection Resized" << std::endl;
	}
	_experiment->_imgOBS = osgDB::readImageFile(_experiment->_obsPic);
	_experiment->_imgObsArray = osgDB::readImageFile(_experiment->_obsArrayPic);
	if (!_experiment->_imgOBS.valid())
	{
		_experiment->_imgOBS = NULL;
	}
	_alignPoint = osg::Vec3d(0.0f, 0.0f, 0.0f);
	i = _experiment->_obsArray.cbegin();
	while (i != _experiment->_obsArray.cend())
	{
		_filename = *i++;
		osg::ref_ptr<Nurbs> thisNurbs = readNurbs(_experiment->_obsArrayLength);
		Nurbs *prveNurbs = (_experiment->_nurbs.empty() ? NULL : _experiment->_nurbs.back());
		alignCtrlPoints(thisNurbs, prveNurbs,_experiment->_obsArrayOFFSET);
		_experiment->_obsArrayNurbsMethod == 1 ? updateNurbs(thisNurbs, _experiment->_numObsinArray, 0.0f) : updateNurbs(thisNurbs, new NurbsCurve, _experiment->_numObsinArray, 0.0f);
		_experiment->_nurbs.push_back(thisNurbs.release());
	}
	_experiment->_speed = _vehicle->_speed;

	_experiment->_offset = _roads->_width;
	osg::Vec3d startOffset = X_AXIS * _experiment->_offset * _experiment->_startLane * 0.25f;
	osg::Matrix m = osg::Matrix::translate(startOffset);
	m *= osg::Matrix::translate(X_AXIS * _experiment->_laneOffset);

	{
		arrayByMatrix(_vehicle->_V, m);
		_vehicle->_O = _vehicle->_O * m;
		_vehicle->_initialState = m;
		_vehicle->_baseline = _experiment->_offset * _experiment->_deviationBaseline * 0.25f;

		osg::ref_ptr<osg::MatrixTransform> mT = new osg::MatrixTransform;
		mT->addChild(_vehicle->_carNode);
		mT->setMatrix(m);
		mT->setDataVariance(osg::Object::STATIC);
		osgUtil::Optimizer op;
		op.optimize(mT, osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS);
		mT = NULL;
	}

	//Initialize Triggers
	const unsigned &numTriggers = _experiment->_triggerTimer->getNumElements();
	if (_experiment->_triggerEnable.size() != numTriggers)
	{
		const unsigned &numTriggerEnables = _experiment->_triggerEnable.size();
		_experiment->_triggerEnable.resize(numTriggers);
		if (numTriggers > numTriggerEnables)
		{
			std::vector<triggerEnablePair>::const_iterator assign_start = _experiment->_triggerEnable.cbegin();
			std::vector<triggerEnablePair>::const_iterator assign_end = assign_start + numTriggerEnables;
			std::vector<triggerEnablePair>::iterator i = _experiment->_triggerEnable.begin() + numTriggerEnables;
			while (i != _experiment->_triggerEnable.end())
			{
				if (assign_start == assign_end)
				{
					assign_start = _experiment->_triggerEnable.begin();
				}
				*i = *assign_start;
				++i;
				++assign_start;
			}
		}
	}

	//Initialize Text
	const unsigned &numText = _experiment->_textTime->getNumElements();
	if (_experiment->_textPeriod->getNumElements() != numText)
	{
		const unsigned &numTextPeriod = _experiment->_textPeriod->getNumElements();
		_experiment->_textPeriod->resize(numText);
		if (numText > numTextPeriod)
		{
			osg::DoubleArray::const_iterator assign_start = _experiment->_textPeriod->begin();
			osg::DoubleArray::const_iterator assign_end = assign_start + numTextPeriod;
			osg::DoubleArray::iterator i = _experiment->_textPeriod->begin() + numTextPeriod;
			while (i != _experiment->_textPeriod->end())
			{
				if (assign_start == assign_end)
				{
					assign_start = _experiment->_textPeriod->begin();
				}
				*i = *assign_start;
				++i;
				++assign_start;
			}
		}
	}
	if (_experiment->_textContent.size() != numText)
	{
		const unsigned &numTextContent = _experiment->_textContent.size();
		_experiment->_textContent.resize(numText);
		if (numText > numTextContent)
		{
			stringList::const_iterator assign_start = _experiment->_textContent.begin();
			stringList::const_iterator assign_end = assign_start + numTextContent;
			stringList::iterator i = _experiment->_textContent.begin() + numTextContent;
			while (i != _experiment->_textContent.end())
			{
				if (assign_start == assign_end)
				{
					assign_start = _experiment->_textContent.begin();
				}
				*i = *assign_start;
				++i;
				++assign_start;
			}
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
	const string EXPERIMENT = "[EXPERIMENT]";
	const string SPACE = " ";

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
		static const string NAME = "NAME";
		static const string AGE = "AGE";
		static const string GENDER = "GENDER";
		static const string DRIVING = "DRIVING";
		static const string RROAD = "RANDOMROADS";
		static const string SUBROADS = "ROADS";
		static const string TRIALNAME = "TRIALNAME";
		static const string DIRNAME = "DIRECTORYNAME";
		while (flag == SUBJECTS && !in.eof())
		{
			byPassSpace(in, config);
			const string title = getTillFirstSpaceandToUpper(config);
			_subjects->setFilePath(_filename);
			if (title == NAME)
			{
				config.erase(config.begin(), config.begin() + NAME.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
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
			else if (title == RROAD)
			{
				config.erase(config.begin(), config.begin() + RROAD.size());
				if (!config.empty())
				{
					int rr = stoi(config);
					bool RR = (rr == 1) ? true : false;
					_subjects->setRandomRoads(RR);
				}
				continue;
			}
			else if (title == SUBROADS)
			{
				config.erase(config.begin(), config.begin() + SUBROADS.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				while (!config.empty())
				{
					std::string txtRoad;
					std::size_t found_to = config.find_first_of(" ");
					if (found_to != config.npos)
					{
						std::copy(config.begin(), config.begin() + found_to, std::back_inserter(txtRoad));
						config.erase(config.begin(), config.begin() + found_to);
						std::size_t found = config.find_first_not_of(SPACE);
						if (found != config.npos) config.erase(config.begin(), config.begin() + found);
					}
					else
					{
						txtRoad = config;
						config.clear();
					}
					_subjects->_roads.push_back(txtRoad);
				}
				continue;
			}
			else if (title == TRIALNAME)
			{
				config.erase(config.begin(), config.begin() + TRIALNAME.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					std::string::size_type sz;
					int tn = stoi(config, &sz);
					if (tn == 0)
					{
						_subjects->setTrialName(osgDB::getSimpleFileName(osgDB::getNameLessAllExtensions(_filename)));
					}
					else if (tn == 1)
					{
						config.erase(config.begin(), config.begin() + sz);
						std::size_t found = config.find_first_not_of(SPACE);
						if (found != config.npos) config.erase(config.begin(), config.begin() + found);
						_subjects->setTrialName(config);
					}
				}
				continue;
			}
			else if (title == DIRNAME)
			{
				config.erase(config.begin(), config.begin() + DIRNAME.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					_subjects->setDirectoryName(config);
				}
				continue;
			}

			flag = config;
			flag = getTillFirstSpaceandToUpper(flag);
			continue;
		}

		//Set Screen
		static const string SCR = "SCREEN";
		static const string USEHMD = "USE_HMD";
		static const string HMDREFRESHRATE = "HMD_REFRESHRATE";
		static const string ASPECT = "ASPECT";
		static const string REALWORD = "REALWORLD";
		static const string BGPIC = "BACKGROUND";
		static const string HDISTANCE = "HDISTANCE";
		static const string VDISTANCE = "VDISTANCE";
		static const string VOFFSET = "VOFFSET";
		static const string FOVY = "FOVY";
		static const string DISTOR = "DISTORTION";
		static const string FXAA = "MULTISAMPLES";
		static const string ZNEAR = "ZNEAR";
		static const string ZFAR = "ZFAR";
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
			else if (title == USEHMD)
			{
				config.erase(config.begin(), config.begin() + USEHMD.size());
				if (!config.empty())
				{
					_screens->_HMD = stoi(config);
				}
				continue;
			}
			else if (title == HMDREFRESHRATE)
			{
				config.erase(config.begin(), config.begin() + HMDREFRESHRATE.size());
				if (!config.empty())
				{
					_screens->_HMD_RefreshRate = stod(config);
				}
				continue;
			}
			else if (title == ASPECT)
			{
				config.erase(config.begin(), config.begin() + ASPECT.size());
				_screens->_aspect = stod(config);
				continue;
			}
			else if (title == ZNEAR)
			{
				config.erase(config.begin(), config.begin() + ZNEAR.size());
				if (!config.empty())
				{
					_screens->_zNear = stod(config);
				}
				continue;
			}
			else if (title == ZFAR)
			{
				config.erase(config.begin(), config.begin() + ZFAR.size());
				if (!config.empty())
				{
					_screens->_zFar = stod(config);
				}
				continue;
			}
			else if (title == REALWORD)
			{
				config.erase(config.begin(), config.begin() + REALWORD.size());		
				std::string::size_type sz;
				_screens->_hFov = stod(config, &sz);
				if(!_screens->_hFov) continue;
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

				std::stringstream ss;
				double r,g,b;
				ss << config;
				ss >> r >> g >> b;
				if (ss.fail())
				{
					std::size_t found = config.find_first_not_of(SPACE);
					if (found != config.npos) config.erase(config.begin(), config.begin() + found);
					_screens->_background = config;
				}
				else
				{
					_screens->_bgColor.set(r, g, b, 1.0f);
				}
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
			else if (title == FXAA)
			{
				config.erase(config.begin(), config.begin() + FXAA.size());
				if (!config.empty())
				{
					_screens->_fxaa = stoi(config);
				}
				continue;
			}

			flag = config;
			flag = getTillFirstSpaceandToUpper(flag);
			continue;
		}

		//Set Vehicle
		static const string ACCEL = "ACCELERATION";
		static const string DEADBAND = "STEERINGDEADBAND";
		static const string CARSPEED = "CARSPEED";
		static const string CARWHEEL = "CARWHEEL";
		static const string CARMODEL = "CARMODEL";
		static const string CARWIDTH = "CARWIDTH";
		static const string CARHEIGHT = "CARHEIGHT";
		static const string CARLENGTH = "CARLENGTH";
		static const string WHEELBASE = "WHEELBASE";
		static const string WHEELACCL = "WHEELACCL";
		static const string SPEEDINCR = "SPEEDINCR";
		static const string DYNAMICSENSITIVE = "DYNAMICSENSITIVELEVEL";
		static const string STARTDELAY = "STARTDELAY";
		static const string CARVISIBLE = "CARVISIBLE";
		static const string CARRESET = "CARRESET";
		static const string RESETMODE = "RESETMODE";
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
			if (title == DEADBAND)
			{
				config.erase(config.begin(), config.begin() + DEADBAND.size());
				if (!config.empty())
				{
					_vehicle->_deadband = stod(config);
				}
				continue;
			}
			if (title == DYNAMICSENSITIVE)
			{
				config.erase(config.begin(), config.begin() + DYNAMICSENSITIVE.size());
				if (!config.empty())
				{
					_vehicle->_dynamicSensitive = stod(config);
				}
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
			else if (title == CARMODEL)
			{
				config.erase(config.begin(), config.begin() + CARMODEL.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				_vehicle->_carModel = config;
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
			else if (title == WHEELBASE)
			{
				config.erase(config.begin(), config.begin() + WHEELBASE	.size());
				if (!config.empty())
				{
					_vehicle->_wheelBase = stod(config);
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
			else if (title == SPEEDINCR)
			{
				config.erase(config.begin(), config.begin() + SPEEDINCR.size());
				if (!config.empty())
				{
					_vehicle->_speedincr = stod(config);
				}
				continue;
			}
			else if (title == STARTDELAY)
			{
				config.erase(config.begin(), config.begin() + STARTDELAY.size());
				if (!config.empty())
				{
					_vehicle->_startDelay = stod(config);
				}
				continue;
			}
			else if (title == CARVISIBLE)
			{
				config.erase(config.begin(), config.begin() + CARVISIBLE.size());
				if (!config.empty())
				{
					_vehicle->_visibility = (stoi(config) == 0) ? false : true;
				}
				continue;
			}
			else if (title == CARRESET)
			{
				config.erase(config.begin(), config.begin() + CARRESET.size());
				if (!config.empty())
				{
					_vehicle->_carReset = stoi(config);
				}
				continue;
			}
			else if (title == RESETMODE)
			{
				config.erase(config.begin(), config.begin() + RESETMODE.size());
				if (!config.empty())
				{
					_vehicle->_resetMode = stoi(config);
				}
				continue;
			}
			
			flag = config;
			flag = getTillFirstSpaceandToUpper(flag);
			continue;
		}

		//Setting Road
		static const string ROADPIC = "ROADPIC";
		static const string TEXTUREWIDTH = "TEXTUREWIDTH";
		static const string ROADTXT = "ROADTXT";
		static const string ROADDT = "DENSITY";
		static const string WALLH = "WALLHEIGHT";
		static const string ROADLANE = "ROADLANES";
		static const string WALLPIC = "WALLPIC";
		static const string SCALE = "SCALEVECTOR";
		static const string METHOD = "METHOD";
		static const string ROADVISIBLE = "ROADVISIBLE";
		static const string ROADLENGTH = "ROADLENGTH";
		static const string WALLROADWIDTH = "WALLROADWIDTH";
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
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					std::size_t found_to = config.find_first_of(" ");
					if (found_to != config.npos)
					{
						std::copy(config.begin(), config.begin() + found_to, std::back_inserter(_roads->_texture));
						config.erase(config.begin(), config.begin() + found_to);
					}
					else
					{
						_roads->_texture = config;
						config.clear();
					}
				}
				if (!config.empty())
				{
					_roads->_imgRoadAnisotropy = stod(config);
				}
				continue;
			}
			else if (title == TEXTUREWIDTH)
			{
				config.erase(config.begin(), config.begin() + TEXTUREWIDTH.size());
				if (!config.empty())
				{
					_roads->_textureWidth = stod(config);
				}
				continue;
			}
			else if (title == ROADTXT)
			{
				config.erase(config.begin(), config.begin() + ROADTXT.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				while (!config.empty())
				{
					std::string txtRoad;
					std::size_t found_to = config.find_first_of(" ");
					if (found_to != config.npos)
					{
						std::copy(config.begin(), config.begin() + found_to, std::back_inserter(txtRoad));
						config.erase(config.begin(), config.begin() + found_to);
						std::size_t found = config.find_first_not_of(SPACE);
						if (found != config.npos) config.erase(config.begin(), config.begin() + found);
					}
					else
					{
						txtRoad = config;
						config.clear();
					}
					_roads->_roadTxt.push_back(txtRoad);
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
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					std::size_t found_to = config.find_first_of(" ");
					if (found_to != config.npos)
					{
						std::copy(config.begin(), config.begin() + found_to, std::back_inserter(_roads->_textureWall));
						config.erase(config.begin(), config.begin() + found_to);
					}
					else
					{
						_roads->_textureWall = config;
						config.clear();
					}
				}
				if (!config.empty())
				{
					_roads->_imgWallAnisotropy = stod(config);
				}
				continue;
			}
			else if (title == METHOD)
			{
				config.erase(config.begin(), config.begin() + METHOD.size());
				if (!config.empty())
				{
					_roads->_nurbsMethod = stoi(config);
				}
				continue;
			}
			else if (title == ROADLENGTH)
			{
				config.erase(config.begin(), config.begin() + ROADLENGTH.size());
				if (!config.empty())
				{
					_roads->_length = stod(config);
				}
				continue;
			}
			else if (title == WALLROADWIDTH)
			{
				config.erase(config.begin(), config.begin() + WALLROADWIDTH.size());
				if (!config.empty())
				{
					_roads->_wallRoadWidth = stod(config);
				}
				continue;
			}
			else if (title == ROADVISIBLE)
			{
				config.erase(config.begin(), config.begin() + ROADVISIBLE.size());
				if (!config.empty())
				{
					_roads->_visible = stoi(config);
				}
				continue;
			}

			flag = config;
			flag = getTillFirstSpaceandToUpper(flag);
			continue;
		}

		//Setting Camera
		static const string CAM_OFFSET("OFFSET");
		static const string CAM_EYE_TRACKER("EYETRACKER");
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

		//set Experiment
		static const string TIMERTRIGGER("TIMER-TRIGGER");
		static const string STARTLANE("START-LANE");
		static const string LANEOFFSET("LANE-OFFSET");
		static const string TEXTIME("TEXTIME");
		static const string PERIOD("PERIOD");
		static const string TEXT("TEXT");

		static const string DYNAMICHANGE("DYNAMIC-CHANGE");
		static const string DYNAMICCHANGECONDITION("DYNAMIC-CHANGE-CONDITION");
		static const string DYNAMICCHANGEPIC("DYNAMIC-CHANGE-PIC");

		static const string OBSTACLE("OBSTACLE");
		static const string OBSTACLERANGE("OBSTACLE-RANGE");
		static const string OBSTACLEPOSITION("OBSTACLE-POSITION");
		static const string OBSPOSOFFSET("OBS-POSITION-OFFSET");
		static const string OBSSIZE("OBS-SIZE");
		static const string OBSSHAPE("OBS-SHAPE");
		static const string OBSVISIBLE("OBS-VISIBLE");
		static const string OBSPIC("OBS-PIC");
		static const string OBSCOLLISION("OBS-COLLISION");

		static const string OBSARRAY("OBS-ARRAY");
		static const string OBSARRAYALIGNMENT("OBS-ARRAY-ALIGN");
		static const string OBSARRAYLENGTH("OBS-ARRAY-LENGTH");
		static const string OBSARRAYOFFSET("OBS-ARRAY-OFFSET");
		static const string OBSARRAYNUM("OBS-ARRAY-NUM");
		static const string OBSARRAYOBSSIZE("OBS-ARRAY-OBSSIZE");
		static const string OBSARRAYPIC("OBS-ARRAY-PIC");
		static const string OBSARRAYMODE("OBS-ARRAY-MODE");
		static const string OBSANIMELOOP("OBS-ANIME-LOOP");
		static const string OBSNURBSMETHOD("OBS-ARRAY-NURBSMETHOD");

		static const string OPTICFLOW("OPTICFLOW");
		static const string OPTICFLOWRANGE("OPTICFLOW-VISIBLE");
		static const string DEPTHDENSITY("DEPTHDENSITY");
		static const string OPTICFLOWWIDTH("OPTICFLOW-WIDTH");
		static const string OPTICFLOWHEIGHT("OPTICFLOW-HEIGHT");
		static const string OPTICFLOWDENSITY("OPTICFLOW-DENSITY");
		static const string OPTICFLOWFRAMECOUNTS("OPTICFLOW-FRAMECOUNTS");
		static const string OPTICFLOWVERSIONS("OPTICFLOW-VERSIONS");

		static const string	TRIGGER_ENABLE("TRIGGER-ENABLE");
		static const string TRIGGER_TIME("TRIGGER-TIME");

		static const string DEVIATION("DEVIATION");
		static const string DEVIATIONWARN("DEVIATION-WARN");
		static const string DEVIATIONSIREN("DEVIATION-SIREN");
		static const string DEVIATIONBASELINE("DEVIATION-BASELINE");
		while (flag == EXPERIMENT && !in.eof())
		{
			byPassSpace(in, config);
			const string title = getTillFirstSpaceandToUpper(config);
			if (title == TIMERTRIGGER)
			{
				config.erase(config.begin(), config.begin() + TIMERTRIGGER.size());
				if (!config.empty())
				{
					_experiment->_timer = stoi(config);
				}
				continue;
			}
			if (title == STARTLANE)
			{
				config.erase(config.begin(), config.begin() + STARTLANE.size());
				if (!config.empty())
				{
					_experiment->_startLane = stoi(config);
				}
				continue;
			}
			else if (title == LANEOFFSET)
			{
				config.erase(config.begin(), config.begin() + LANEOFFSET.size());
				if (!config.empty())
				{
					_experiment->_laneOffset = stod(config);
				}
				continue;
			}
			else if (title == TEXTIME)
			{
				config.erase(config.begin(), config.begin() + TEXTIME.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_experiment->_textTime->push_back(stoi(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == PERIOD)
			{
				config.erase(config.begin(), config.begin() + PERIOD.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_experiment->_textPeriod->push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == TEXT)
			{
				config.erase(config.begin(), config.begin() + TEXT.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					bool search(false);
					const std::string temp = "\\n";
					do 
					{
						std::size_t fd = config.find(temp);
						if (fd != config.npos)
						{
							std::string linefeed = "\n";
							config.replace(fd, temp.size(), linefeed);
							search = true;
						}
						else
						{
							search = false;
						}
					} while (search);
					_experiment->_textContent.push_back(config);
				}
				continue;
			}
			else if (title == DYNAMICHANGE)
			{
				config.erase(config.begin(), config.begin() + DYNAMICHANGE.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_experiment->_dynamicChange->push_back(stoi(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == DYNAMICCHANGECONDITION)
			{
				config.erase(config.begin(), config.begin() + DYNAMICCHANGECONDITION.size());
				std::vector<unsigned> read;
				while (!config.empty())
				{
					std::string::size_type sz;
					read.push_back(stoi(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				if (read.size() >= 1)
				{
					_experiment->_dynamicChangeCondition = read.front();
				}
				if (read.size() >= 2)
				{
					_experiment->_dynamicChangeLasting = read.back();
				}
				continue;
			}
			else if (title == DYNAMICCHANGEPIC)
			{
				config.erase(config.begin(), config.begin() + DYNAMICCHANGEPIC.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					_experiment->_dynamicPic = config;
				}
				continue;
			}
			else if (title == OBSTACLE)
			{
				config.erase(config.begin(), config.begin() + OBSTACLE.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_experiment->_obstaclesTime->push_back(stoi(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == OBSTACLERANGE)
			{
				config.erase(config.begin(), config.begin() + OBSTACLERANGE.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_experiment->_obstacleRange->push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == OBSTACLEPOSITION)
			{
				config.erase(config.begin(), config.begin() + OBSTACLEPOSITION.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_experiment->_obstaclePos->push_back(stoi(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == OBSPOSOFFSET)
			{
				config.erase(config.begin(), config.begin() + OBSPOSOFFSET.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_experiment->_obsPosOffset->push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == OBSCOLLISION)
			{
				config.erase(config.begin(), config.begin() + OBSCOLLISION.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_experiment->_obsCollision->push_back(stoi(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == OBSSIZE)
			{
				config.erase(config.begin(), config.begin() + OBSSIZE.size());
				osg::ref_ptr<osg::DoubleArray> read = new osg::DoubleArray;
				while (!config.empty())
				{
					std::string::size_type sz;
					read->push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				if (read->size() >= 3)
				{
					_experiment->_obsSize.set(read->at(0), read->at(1), read->at(2));
				}
				continue;
			}
			else if (title == OBSSHAPE)
			{
				config.erase(config.begin(), config.begin() + OBSSHAPE.size());
				if (!config.empty())
				{
					_experiment->_obsShape = stoi(config);
				}
				continue;
			}
			else if (title == OBSVISIBLE)
			{
				config.erase(config.begin(), config.begin() + OBSVISIBLE.size());
				if (!config.empty())
				{
					_experiment->_obsVisible = stod(config);
				}
				continue;
			}
			else if (title == OBSPIC)
			{
				config.erase(config.begin(), config.begin() + OBSPIC.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					std::size_t found_to = config.find_first_of(SPACE);
					if (found_to != config.npos)
					{
						std::copy(config.begin(), config.begin() + found_to, std::back_inserter(_experiment->_obsPic));
						config.erase(config.begin(), config.begin() + found_to);
					}
					else
					{
						_experiment->_obsPic = config;
						config.clear();
					}
				}
				if (!config.empty())
				{
					_experiment->_imgAnisotropy = stod(config);
				}
				continue;
			}
			else if (title == OBSARRAY)
			{
				config.erase(config.begin(), config.begin() + OBSARRAY.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				while (!config.empty())
				{
					std::string txtOBS;
					std::size_t found_to = config.find_first_of(" ");
					if (found_to != config.npos)
					{
						std::copy(config.begin(), config.begin() + found_to, std::back_inserter(txtOBS));
						config.erase(config.begin(), config.begin() + found_to);
						std::size_t found = config.find_first_not_of(SPACE);
						if (found != config.npos) config.erase(config.begin(), config.begin() + found);
					}
					else
					{
						txtOBS = config;
						config.clear();
					}
					_experiment->_obsArray.push_back(txtOBS);
				}
				continue;
			}
			else if (title == OBSARRAYOBSSIZE)
			{
				config.erase(config.begin(), config.begin() + OBSARRAYOBSSIZE.size());
				std::vector<double> temp;
				while (!config.empty())
				{
					std::string::size_type sz;
					temp.push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				if (temp.size() >= 3)
				{
					_experiment->_obsArraySize.x() = temp.at(0);
					_experiment->_obsArraySize.y() = temp.at(1);
					_experiment->_obsArraySize.z() = temp.at(2);
				}
				continue;
			}
			else if (title == OBSARRAYPIC)
			{
				config.erase(config.begin(), config.begin() + OBSARRAYPIC.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					_experiment->_obsArrayPic = config;
				}
				continue;
			}
			else if (title == OBSARRAYNUM)
			{
				config.erase(config.begin(), config.begin() + OBSARRAYNUM.size());
				if (!config.empty())
				{
					_experiment->_numObsinArray = stoi(config);
				}
				continue;
			}
			else if (title == OBSARRAYMODE)
			{
				config.erase(config.begin(), config.begin() + OBSARRAYMODE.size());
				if (!config.empty())
				{
					_experiment->_animationMode = stoi(config) == 0 ? true : false;
					_experiment->_GLPOINTSMODE = stoi(config) == 2 ? true : false;
				}
				continue;
			}
			else if (title == OBSNURBSMETHOD)
			{
				config.erase(config.begin(), config.begin() + OBSNURBSMETHOD.size());
				if (!config.empty())
				{
					_experiment->_obsArrayNurbsMethod = stoi(config);
				}
				continue;
			}
			else if (title == OBSANIMELOOP)
			{
				config.erase(config.begin(), config.begin() + OBSANIMELOOP.size());
				if (!config.empty())
				{
					_experiment->_animationLoop = stoi(config) % 3;
				}
				continue;
			}
			else if (title == OBSARRAYOFFSET)
			{
				config.erase(config.begin(), config.begin() + OBSARRAYOFFSET.size());
				if (!config.empty())
				{
					_experiment->_obsArrayOFFSET = stod(config);
				}
				continue;
			}
			else if (title == OBSARRAYALIGNMENT)
			{
				config.erase(config.begin(), config.begin() + OBSARRAYALIGNMENT.size());
				std::vector<double> temp;
				while (!config.empty())
				{
					std::string::size_type sz;
					temp.push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				if (temp.size() >= osg::Vec3d::num_components)
				{
					_experiment->_obsArrayAlign.x() = temp.at(0);
					_experiment->_obsArrayAlign.y() = temp.at(1);
					_experiment->_obsArrayAlign.z() = temp.at(2);
				}
				continue;
			}
			else if (title == OBSARRAYLENGTH)
			{
				config.erase(config.begin(), config.begin() + OBSARRAYLENGTH.size());
				if (!config.empty())
				{
					_experiment->_obsArrayLength = stod(config);
				}
				continue;
			}
			else if (title == OPTICFLOW)
			{
				config.erase(config.begin(), config.begin() + OPTICFLOW.size());
				if (!config.empty())
				{
					int enable = stoi(config);
					_experiment->_opticFlow = (enable == 1);
				}
				continue;
			}
			else if (title == OPTICFLOWRANGE)
			{
				config.erase(config.begin(), config.begin() + OPTICFLOWRANGE.size());
				if (!config.empty())
				{
					_experiment->_opticFlowRange = stoi(config);
				}
				continue;
			}
			else if (title == OPTICFLOWFRAMECOUNTS)
			{
				config.erase(config.begin(), config.begin() + OPTICFLOWFRAMECOUNTS.size());
				if (!config.empty())
				{
					_experiment->_opticFlowFrameCounts = stoi(config);
				}
				continue;
			}
			else if (title == OPTICFLOWVERSIONS)
			{
				config.erase(config.begin(), config.begin() + OPTICFLOWVERSIONS.size());
				if (!config.empty())
				{
					_experiment->_opticFlowVersions = stoi(config);
				}
				continue;
			}
			else if (title == DEPTHDENSITY)
			{
				config.erase(config.begin(), config.begin() + DEPTHDENSITY.size());
				if (!config.empty())
				{
					_experiment->_depthDensity = stoi(config);
				}
				continue;
			}
			else if (title == OPTICFLOWWIDTH)
			{
				config.erase(config.begin(), config.begin() + OPTICFLOWWIDTH.size());
				if (!config.empty())
				{
					_experiment->_opticFlowWidth = stod(config);
				}
				continue;
			}
			else if (title == OPTICFLOWHEIGHT)
			{
				config.erase(config.begin(), config.begin() + OPTICFLOWHEIGHT.size());
				if (!config.empty())
				{
					_experiment->_opticFlowHeight = stod(config);
				}
				continue;
			}
			else if (title == OPTICFLOWDENSITY)
			{
				config.erase(config.begin(), config.begin() + OPTICFLOWDENSITY.size());
				if (!config.empty())
				{
					_experiment->_opticFlowDensity = stoi(config);
				}
				continue;
			}
			else if (title == TRIGGER_ENABLE)
			{
				config.erase(config.begin(), config.begin() + TRIGGER_ENABLE.size());
				unsigned numCom = 0;
				while (!config.empty())
				{
					std::string::size_type sz;
					std::pair<int, enum TRIGGER_COM> temp;
					temp = std::make_pair(stoi(config, &sz), enum TRIGGER_COM(Experiment::TRIGGER_COM::ROAD + numCom));
					++numCom;
					numCom %= _experiment->_NUMTRIGGERCOM;
					_experiment->_triggerEnable.push_back(temp);
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == TRIGGER_TIME)
			{
				config.erase(config.begin(), config.begin() + TRIGGER_TIME.size());
				while (!config.empty())
				{
					std::string::size_type sz;
					_experiment->_triggerTimer->push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				continue;
			}
			else if (title == DEVIATION)
			{
				config.erase(config.begin(), config.begin() + DEVIATION.size());
				if (!config.empty())
				{
					_experiment->_deviation = stod(config);
				}
				continue;
			}
			else if (title == DEVIATIONWARN)
			{
				config.erase(config.begin(), config.begin() + DEVIATIONWARN.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					_experiment->_deviationWarn = config;
				}
				else
				{
					_experiment->_deviationWarn = "DEVIATE TOO MUCH";
				}
				continue;
			}
			else if (title == DEVIATIONSIREN)
			{
				config.erase(config.begin(), config.begin() + DEVIATIONSIREN.size());
				std::size_t found = config.find_first_not_of(SPACE);
				if (found != config.npos) config.erase(config.begin(), config.begin() + found);
				if (!config.empty())
				{
					_experiment->_deviationSiren = config;
				}
				continue;
			}
			else if (title == DEVIATIONBASELINE)
			{
				config.erase(config.begin(), config.begin() + DEVIATIONBASELINE.size());
				if (!config.empty())
				{
					_experiment->_deviationBaseline = stod(config);
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

osg::ref_ptr<Nurbs> ReadConfig::readNurbs(const double &customLength /*= 0.0f*/)
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

	osg::ref_ptr<Nurbs> nurbs = new Nurbs;
	const string CTRLPOINT = "CTRLPOINTS";
	const string KNOTS = "KNOTS";
	const string ORDERS = "orders";
	const string SCALE = "SCALE";
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
					continue;
				}

				double x, y, z(0.0f);
				std::string::size_type sz;
				x = stod(config, &sz);
				y = stod(config.substr(sz));
				nurbs->_ctrlPoints->push_back(osg::Vec3d(x, y, z));
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
					continue;
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
					nurbs->_knotVector->push_back(knot.front());
				}
			}
		}

		while (title == SCALE && !filein.eof())
		{
			byPassSpace(filein, config);
			if (!config.empty())
			{
				if (isletter((config.front())))
				{
					title = getTillFirstSpaceandToUpper(config);
					continue;
				}
				osg::ref_ptr<osg::DoubleArray> s = new osg::DoubleArray;
				while (!config.empty())
				{
					std::string::size_type sz;
					s->push_back(stod(config, &sz));
					config.erase(config.begin(), config.begin() + sz);
				}
				if (s->empty()) continue;
				if (s->getNumElements() == osg::Vec3d::num_components)
				{
					nurbs->_scale.x() = s->at(0);
					nurbs->_scale.y() = s->at(1);
					nurbs->_scale.z() = s->at(2);
				}
			}
		}
	}

	nurbs->_order = nurbs->_knotVector->size() - nurbs->_ctrlPoints->size();

	if (customLength > 0)
	{
		const unsigned &numPoints = nurbs->_ctrlPoints->getNumElements();
		const unsigned &order = nurbs->_order;
		const unsigned &numKnots = nurbs->_knotVector->getNumElements();
		double *knots = (double*)malloc(numKnots*sizeof(double));
		for (unsigned i = 0; i < numKnots; i++)
		{
			knots[i] = nurbs->_knotVector->at(i);
		}
		const unsigned dim = nurbs->_ctrlPoints->front().num_components;
		double *ctrlpts = (double*)malloc(numPoints*dim*sizeof(double));
		for (unsigned int i = 0; i < numPoints; i++)
		{
			int v = 0;
			for (unsigned j = (i*dim); j < (i + 1)*dim; j++)
			{
				ctrlpts[j] = nurbs->_ctrlPoints->at(i)._v[v];
				++v;
			}
		}
		const unsigned kind = 1;
		SISLCurve *sc = newCurve(numPoints, order, knots, ctrlpts, kind, dim, 1);

		double length(0.0f);
		int jstat(-1);
		s1240(sc, eps_1000, &length, &jstat);
		if (!jstat)
		{
			const double L = customLength / length;
			nurbs->_scale.x() = L;
			nurbs->_scale.y() = L;
			nurbs->_scale.z() = L;
		}

		freeCurve(sc);
		free(knots);
		free(ctrlpts);

		sc = NULL;
		knots = NULL;
		ctrlpts = NULL;
	}

	const osg::Vec3d P1 = nurbs->_ctrlPoints->front();
	arrayByMatrix(nurbs->_ctrlPoints, osg::Matrix::scale(nurbs->_scale));
	const osg::Vec3d P2 = nurbs->_ctrlPoints->front();
	arrayByMatrix(nurbs->_ctrlPoints, osg::Matrix::translate(P1 - P2));

	filein.close();

	return nurbs.release();
}

void ReadConfig::scaleCtrlPoints(Nurbs *nurbs)
{
	if (nurbs)
	{
		arrayByMatrix(nurbs->_ctrlPoints, osg::Matrix::scale(_roads->_scale));
	}
}

void ReadConfig::alignCtrlPoints(Nurbs *thisNurbs, Nurbs *prevNurbs,const double &offset /* = 0.0f */)
{
	if (thisNurbs)
	{
		if (offset == 0.0f)
		{
			osg::Vec3d p = _alignPoint;
			osg::Vec3d direction = UP_DIR;
			if (prevNurbs)
			{
				p = *(prevNurbs->_ctrlPoints->rbegin());
				direction = *(prevNurbs->_ctrlPoints->rbegin()) - *(prevNurbs->_ctrlPoints->rbegin() + 1);
			}
			osg::Vec3d v = *(thisNurbs->_ctrlPoints->begin() + 1) - *(thisNurbs->_ctrlPoints->begin());
			osg::Vec3d m = *(thisNurbs->_ctrlPoints->begin());

			osg::Matrix align = osg::Matrix::translate(-m) * osg::Matrix::rotate(v, direction) * osg::Matrix::translate(m);
			align = align * osg::Matrix::translate(p - m);

			//add a minor offset between ctrlpoints
			align = align * osg::Matrix::translate(direction*eps_100);

			arrayByMatrix(thisNurbs->_ctrlPoints, align);
		}

		else
		{
			if (prevNurbs)
			{
				osg::ref_ptr<osg::Vec3dArray> newV = new osg::Vec3dArray;
				newV = project_Line(prevNurbs->_ctrlPoints, offset);
				thisNurbs->_ctrlPoints = newV;
			}
		}
	}
}

void ReadConfig::updateNurbs(Nurbs *thisNurbs, osg::ref_ptr<NurbsCurve> refNB, const unsigned &density, const double &width /* = 0.0f */)
{
	if (!thisNurbs)
	{
		return;
	}

	refNB->setKnotVector(thisNurbs->_knotVector);
	refNB->setDegree(thisNurbs->_order - 1);
	refNB->setNumPath(density);

	refNB->setCtrlPoints(thisNurbs->_ctrlPoints);
	refNB->update();
	thisNurbs->_path = refNB->getPath();

	if (width)
	{
		const double halfW = width * 0.5f;
		thisNurbs->_ctrl_left = project_Line(thisNurbs->_ctrlPoints, halfW);
		thisNurbs->_ctrl_right = project_Line(thisNurbs->_ctrlPoints, -halfW);

		//Adjust "Left" and "Right"
		osg::Vec3d left = *thisNurbs->_ctrl_left->begin() - *thisNurbs->_ctrl_right->begin();
		osg::Vec3d right = *(thisNurbs->_ctrl_right->begin() + 1) - *thisNurbs->_ctrl_right->begin();
		osg::Vec3d dir = right^left;
		if (dir.z() < 0)
		{
			osg::ref_ptr<osg::Vec3dArray> tmp = thisNurbs->_ctrl_left;
			thisNurbs->_ctrl_left = thisNurbs->_ctrl_right;
			thisNurbs->_ctrl_right = tmp;
		}

		refNB->setCtrlPoints(thisNurbs->_ctrl_left);
		refNB->update();
		thisNurbs->_path_left = refNB->getPath();

		refNB->setCtrlPoints(thisNurbs->_ctrl_right);
		refNB->update();
		thisNurbs->_path_right = refNB->getPath();
	}
}

void ReadConfig::updateNurbs(Nurbs *thisNurbs, const unsigned &density, const double &width)
{
	const unsigned &numPoints = thisNurbs->_ctrlPoints->getNumElements();
	const unsigned &order = thisNurbs->_order;
	const unsigned degree = order - 1;

	const unsigned &numKnots = thisNurbs->_knotVector->getNumElements();
	double *knots = (double*)malloc(numKnots*sizeof(double));
	for (unsigned i = 0; i < numKnots; i++)
	{
		knots[i] = thisNurbs->_knotVector->at(i);
	}

	const unsigned &dim = thisNurbs->_ctrlPoints->front().num_components;
	double *ctrlpts = (double*)malloc(numPoints*dim*sizeof(double));

	for (unsigned i = 0; i < numPoints; i++)
	{
		int v = 0;
		for (unsigned j = (i*dim); j < (i + 1)*dim; j++)
		{
			ctrlpts[j] = thisNurbs->_ctrlPoints->at(i)._v[v];
			++v;
		}
	}

	const unsigned kind = 1;
	SISLCurve *sc = newCurve(numPoints, order, knots, ctrlpts, kind, dim, 1);

	const int der = 0;
	double *derive = (double*)calloc((der + 1)*dim, sizeof(double));
	double *curvature = (double*)calloc(dim, sizeof(double));
	double radius;
	int jstat;
	const double denS = ((double)(density) / (double)(numKnots - order - degree)) + 0.5f;
	for (unsigned i = degree; i < numKnots - order; i++)
	{
		int leftknot = i;
		const double left = knots[i];
		const double right = knots[i + 1];
		const double step = (right - left) / denS;
		for (int j = 0; j < denS; j++)
		{
			double k = left + step*j;
			if (k > right) k = right;
			s1225(sc, der, k, &leftknot, derive, curvature, &radius, &jstat);
			if (!jstat)
			{
				thisNurbs->_path->push_back(osg::Vec3d(derive[0], derive[1], derive[2]));
				thisNurbs->_radius->push_back(radius);
			}
			else
			{
				osg::notify(osg::FATAL) << "Cannot Evaluate Nurbs based on Given Condition" << std::endl;
				return;
			}
		}
	}
	int leftknot = numKnots - order - 1;
	int k = knots[leftknot + 1];
	s1225(sc, der, k, &leftknot, derive, curvature, &radius, &jstat);
	if (!jstat)
	{
		thisNurbs->_path->push_back(osg::Vec3d(derive[0], derive[1], derive[2]));
		thisNurbs->_radius->push_back(radius);
	}
	else
	{
		osg::notify(osg::FATAL) << "Cannot Evaluate Nurbs based on Given Condition" << std::endl;
		return;
	}


	if (width)
	{
		const double halfW = width * 0.5f;
		thisNurbs->_ctrl_left = project_Line(thisNurbs->_ctrlPoints, halfW);
		thisNurbs->_ctrl_right = project_Line(thisNurbs->_ctrlPoints, -halfW);

		//Adjust "Left" and "Right"
		osg::Vec3d left = *thisNurbs->_ctrl_left->begin() - *thisNurbs->_ctrl_right->begin();
		osg::Vec3d right = *(thisNurbs->_ctrl_right->begin() + 1) - *thisNurbs->_ctrl_right->begin();
		osg::Vec3d dir = right^left;
		if (dir.z() < 0)
		{
			osg::ref_ptr<osg::Vec3dArray> tmp = thisNurbs->_ctrl_left;
			thisNurbs->_ctrl_left = thisNurbs->_ctrl_right;
			thisNurbs->_ctrl_right = tmp;
		}

		double *ctrlptsL = (double*)malloc(numPoints*dim*sizeof(double));
		double *ctrlptsR = (double*)malloc(numPoints*dim*sizeof(double));

		for (unsigned int i = 0; i < numPoints; i++)
		{
			int v = 0;
			for (unsigned j = (i*dim); j < (i + 1)*dim; j++)
			{
				ctrlptsL[j] = thisNurbs->_ctrl_left->at(i)._v[v];
				ctrlptsR[j] = thisNurbs->_ctrl_right->at(i)._v[v];
				++v;
			}
		}

		SISLCurve *scL = newCurve(numPoints, order, knots, ctrlptsL, kind, dim, 1);
		SISLCurve *scR = newCurve(numPoints, order, knots, ctrlptsR, kind, dim, 1);

		double *derive = (double*)calloc((der + 1)*dim, sizeof(double));
		double *curvature = (double*)calloc(dim, sizeof(double));
		double radius;
		int jstatR, jstatL;
		const double denS = ((double)(density) / (double)(numKnots - order - degree)) + 0.5f;
		for (unsigned int i = degree; i < numKnots - order; i++)
		{
			int leftknot = i;
			const double left = knots[i];
			const double right = knots[i + 1];
			const double step = (right - left) / denS;
			for (int j = 0; j < denS; j++)
			{
				double k = left + step*j;
				if (k > right) k = right;
				s1225(scL, der, k, &leftknot, derive, curvature, &radius, &jstatR);
				if (!jstatR)
				{
					thisNurbs->_path_left->push_back(osg::Vec3d(derive[0], derive[1], derive[2]));
					thisNurbs->_radiusL->push_back(radius);
				}
				s1225(scR, der, k, &leftknot, derive, curvature, &radius, &jstatL);
				if (!jstatL)
				{
					thisNurbs->_path_right->push_back(osg::Vec3d(derive[0], derive[1], derive[2]));
					thisNurbs->_radiusR->push_back(radius);
				}
				if (jstatR || jstatL)
				{
					osg::notify(osg::FATAL) << "Cannot Evaluate Nurbs based on Given Condition" << std::endl;
					return;
				}
			}
		}
		int leftknot = numKnots - order - 1;
		int k = knots[leftknot + 1];
		s1225(scL, der, k, &leftknot, derive, curvature, &radius, &jstatR);
		if (!jstatR)
		{
			thisNurbs->_path_left->push_back(osg::Vec3d(derive[0], derive[1], derive[2]));
			thisNurbs->_radiusL->push_back(radius);
		}
		s1225(scR, der, k, &leftknot, derive, curvature, &radius, &jstatL);
		if (!jstatL)
		{
			thisNurbs->_path_right->push_back(osg::Vec3d(derive[0], derive[1], derive[2]));
			thisNurbs->_radiusR->push_back(radius);
		}
		if (jstatR || jstatL)
		{
			osg::notify(osg::FATAL) << "Cannot Evaluate Nurbs based on Given Condition" << std::endl;
			return;
		}

		freeCurve(scL);
		freeCurve(scR);
		free(ctrlptsL);
		free(ctrlptsR);

		scL = NULL;
		scR = NULL;
		ctrlptsL = NULL;
		ctrlptsR = NULL;
	}

	freeCurve(sc);
	free(knots);
	free(ctrlpts);
	free(derive);
	free(curvature);

	sc = NULL;
	knots = NULL;
	ctrlpts = NULL;
	derive = NULL;
	curvature = NULL;
}

osg::Geode* ReadConfig::measuer()
{
	osg::Vec3d vector;
	vector.z() =  _camset->_offset->at(2)*1.2 + 20.0f;
	vector.x() = 0.0f;
	vector.y() = 0.0f;
	//vector.y() += 2.65f;

	osg::Vec3d A = vector;
	A.x() += -0.5f;
	osg::Vec3d B = vector;
	B.x() += 0.5f;
	
	osg::Vec3d AA = vector;
	AA.z() += -0.5;
	osg::Vec3d BB = vector;
	BB.z() += 0.5f;

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

BoundingboxVisitor::BoundingboxVisitor() :
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}

BoundingboxVisitor::~BoundingboxVisitor()
{
}

void BoundingboxVisitor::apply(osg::Geode &gdNode)
{
	bb.expandBy(gdNode.getBoundingBox());
}

void BoundingboxVisitor::reset()
{
	bb.init();
}