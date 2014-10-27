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
	void texturePerGeometry(Solid *refS);
	bool texCoord(Solid *refS);
};

