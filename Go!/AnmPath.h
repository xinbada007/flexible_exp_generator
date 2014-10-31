#pragma once

#include <osg/AnimationPath>
#include <osg/Array>
class ReadConfig;

class anmPath :
	public osg::AnimationPath
{
public:
	anmPath();
	anmPath(const ReadConfig *rc);

protected:
	virtual ~anmPath();

private:
	osg::ref_ptr<osg::Vec3dArray> _path;
};

