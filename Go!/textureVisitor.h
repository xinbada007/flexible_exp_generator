#pragma once
#include <osg/NodeVisitor>
#include "math.h"
#include "solid.h"

class TextureVisitor :
	public osg::NodeVisitor
{
public:
	TextureVisitor();
	virtual ~TextureVisitor();

	virtual void reset();
	virtual void apply(osg::Group &refNode);

private:
	void texture(Solid *refS);
	bool texCoord(Solid *refS);
	osg::ref_ptr<osg::StateSet> createTex2DStateSet(osg::ref_ptr<osg::Image> image);
};

