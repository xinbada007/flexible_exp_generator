#pragma once
#include <string>
#include <vector>

#include <osg/Array>
#include "Nurbs.h"
#include "math.h"

typedef struct Nurbs:public osg::Referenced
{
	Nurbs()
	{
		_path = new osg::Vec3Array;
		_path_left = new osg::Vec3Array;
		_path_right = new osg::Vec3Array;

		_ctrl_left = new osg::Vec3Array;
		_ctrl_right = new osg::Vec3Array;
		_ctrlPoints = new osg::Vec3Array;

		_knotVector = new osg::DoubleArray;
		_order = 0;
	};
	osg::ref_ptr <osg::Vec3Array> _path;
	osg::ref_ptr <osg::Vec3Array> _path_left;
	osg::ref_ptr <osg::Vec3Array> _path_right;

	osg::ref_ptr<osg::Vec3Array> _ctrl_left;
	osg::ref_ptr<osg::Vec3Array> _ctrl_right;

	osg::ref_ptr <osg::Vec3Array> _ctrlPoints;
	osg::ref_ptr <osg::DoubleArray> _knotVector;
	unsigned _order;

private:
	~Nurbs(){};
}Nurbs;

typedef std::vector<std::string> stringList;
typedef std::vector<osg::ref_ptr<Nurbs>> nurbsList;
typedef struct RoadSet:public osg::Referenced
{
	RoadSet()
	{
		_width = 0.15f;
		_density = 1000;
	}
	
	double _width;
	std::string _texture;
	stringList _roadTxt;
	unsigned _density;
	nurbsList _nurbs;

private:
	~RoadSet(){};
}RoadSet;

typedef struct Vehicle:public osg::Referenced
{
	Vehicle()
	{
		_V = new osg::Vec3Array;
		this->_O.set(O_POINT);
	}
	osg::ref_ptr<osg::Vec3Array> _V;
	osg::Vec3 _O;

	double _width;
	double _length;
	double _height;
	double _lwratio;
	double _speed;
	double _rotate;
	bool _acceleration;
	std::string _texture;

}Vehicle;

typedef struct Screens:public osg::Referenced
{
	Screens()
	{
		_cameras = 0;
		_background = "";
		_scrs = new osg::UIntArray;
	}

	unsigned _cameras;
	std::string _background;
	osg::ref_ptr<osg::UIntArray> _scrs;
}Screens;

class ReadConfig:public osg::Referenced
{
public:
	ReadConfig(const std::string &filename);

	Nurbs * getNurbs() const { return _nurbs.get(); };
	RoadSet * getRoadSet() const { return _roads.get(); };
	Vehicle * getVehicle() const { return _vehicle.get(); };
	Screens * getScreens() const { return _screens.get(); };
private:
	ReadConfig();
	~ReadConfig();
	void assignConfig();

	void readTrial();
	
	Nurbs * readNurbs();
	void alignCtrlPoints(Nurbs *refNurbs);
	void updateNurbs(osg::ref_ptr<NurbsCurve> refNB);
	
	osg::ref_ptr<Nurbs> _nurbs;
	osg::ref_ptr<Vehicle> _vehicle;
	osg::ref_ptr<Screens> _screens;
	osg::ref_ptr<RoadSet> _roads;
	std::string _filename;
};