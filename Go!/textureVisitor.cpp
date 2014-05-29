#include "stdafx.h"
#include "textureVisitor.h"
#include "logicroad.h"
#include "car.h"
#include "plane.h"
#include "loop.h"
#include "edge.h"

#include <osg/PolygonMode>
#include <osgdb/ReadFile>
#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osg/Notify>

TextureVisitor::TextureVisitor():
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}

TextureVisitor::~TextureVisitor()
{
}

void TextureVisitor::reset()
{

}

void TextureVisitor::apply(osg::Group &refNode)
{
	Solid *refS = dynamic_cast<Solid*>(&refNode);

	if (refS)
	{
		osg::notify(osg::NOTICE) << "Texture caught \t" << refNode.libraryName() <<"\t" <<refNode.className() << std::endl;
		if(texCoord(refS)) {texture(refS);}
	}
	
	traverse(refNode);
}

bool TextureVisitor::texCoord(Solid *refS)
{
	if (!refS)
	{
		return false;
	}
	osg::ref_ptr<osg::Vec2Array> refTex = refS->getTexCoord();
	if (!refTex)
	{
		return false;
	}

	//Every geometry should bind with a tex coord which includes 4 tex pixels
	//So we should first check if the size of the tex coord array is consistent with the Num of geometry of the Solid
	const unsigned numTexPixels = 4;
	const unsigned numGeometry = refS->getNumGeometry();
	if (refTex->size()/numTexPixels <= numGeometry)
	{
		osg::notify(osg::FATAL) << "Tex Coord Array is not consistent with Solid" << std::endl;
		return false;
	}

	unsigned geoIndex(0);
	while (geoIndex < numGeometry)
	{
		osg::ref_ptr<osg::Geometry> refGeo = refS->getGeometry(geoIndex);
		if (refGeo)
		{
			osg::Vec2Array::const_iterator from = refTex->begin() + geoIndex*numTexPixels;
			osg::Vec2Array::const_iterator end = from + numTexPixels;
			osg::ref_ptr<osg::Vec2Array> tex = new osg::Vec2Array(from,end);
			refGeo->setTexCoordArray(0,tex.release());
		}
		geoIndex++;
	}

	return true;
}

void TextureVisitor::texture(Solid *refS)
{
	if (!refS)
	{
		return;
	}
	osg::Group *refG = refS->asGroup();
	if (!refG)
	{
		return;
	}

	osg::StateSet *ss = refG->getOrCreateStateSet();
	osg::ref_ptr<osg::PolygonMode> pm = dynamic_cast<osg::PolygonMode*>(ss->getAttribute(osg::StateAttribute::POLYGONMODE));
	if (pm)
	{
		pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);
	}

	const std::string &filename(refS->getTexFile());
	osg::ref_ptr<osg::Image> image = osgDB::readImageFile(filename);
	if (!image.valid())
	{
		osg::notify(osg::FATAL) << "Unable to load texture file. Exiting." << std::endl;
		return;
	}
	osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
	tex->setImage(image);
	tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
	tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
	ss->setTextureAttributeAndModes(0, tex);
	ss->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
	osg::TexEnv *decalTexEnv = new osg::TexEnv();
	decalTexEnv->setMode(osg::TexEnv::DECAL);
	ss->setTextureAttribute(0, decalTexEnv);
}