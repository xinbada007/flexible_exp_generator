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
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
_maxAnsiotropy(1.0f)
{
}

TextureVisitor::~TextureVisitor()
{
}

void TextureVisitor::reset()
{
	_maxAnsiotropy = 1.0f;
}

void TextureVisitor::apply(osg::Group &refNode)
{
	Solid *refS = dynamic_cast<Solid*>(&refNode);

	if (refS)
	{
		if (refS->getImageTexture().valid())
		{
			osg::notify(osg::NOTICE) << "Texture caught \t" << refNode.libraryName() << "\t" << refNode.className() << std::endl;
			if (texCoord(refS)) { texture(refS); }
		}
	}
	
	traverse(refNode);
}

bool TextureVisitor::texCoord(Solid *refS)
{
	if (!refS)
	{
		return false;
	}
	if (!refS->getTexMode())
	{
		return false;
	}

	//Every geometry should bind with a tex coord which includes 4 tex pixels
	//So we should first check if the size of the tex coord array is consistent with the Num of geometry of the Solid
	const unsigned numTexPixels = 4;
	const unsigned numGeometry = refS->getNumGeometry();
	osg::ref_ptr<osg::Vec2Array> refTex = refS->getTexCoord();
	const bool perGmtry = !refTex;
	if (!perGmtry)
	{
		if (refTex->getNumElements() / numTexPixels != numGeometry)
		{
			osg::notify(osg::FATAL) << "Tex Coord Array is not consistent with Solid" << std::endl;
		}

		//obviously if numTexcoord != numChildren the texcorrd is wrong
// 		if (refTex->size() / numTexPixels < numGeometry && refS->getAbstract())
// 		{
// 			osg::notify(osg::WARN) << "Tex Coord Array is not consistent with Solid" << std::endl;
// 			return false;
// 		}
// 		if (refTex->size() / numTexPixels < numGeometry && !refS->getAbstract())
// 		{
// 			osg::notify(osg::WARN) << "Tex Coord Array is not consistent with Solid" << std::endl;
// 			return false;
// 		}
	}

	unsigned geoIndex(0);
	while (geoIndex < numGeometry)
	{
		osg::ref_ptr<osg::Geometry> refGeo = refS->getGeometry(geoIndex);
		if (refGeo)
		{
			if (!perGmtry)
			{
				osg::Vec2Array::const_iterator from = refTex->begin() + geoIndex*numTexPixels;
				osg::Vec2Array::const_iterator end = from + numTexPixels;
				osg::ref_ptr<osg::Vec2Array> tex = new osg::Vec2Array(from, end);
				refGeo->setTexCoordArray(0, tex.release());
			}
		}
		geoIndex++;
	}

	return true;
}

void TextureVisitor::texture(Solid *refS)
{
	_maxAnsiotropy = refS->getMaxAnisotropy();
	osg::ref_ptr<osg::Image> image = refS->getImageTexture();
	osg::ref_ptr<osg::StateSet> ss = createTex2DStateSet(image.release());

	refS->setStateSet(ss.release());
}

osg::ref_ptr<osg::StateSet> TextureVisitor::createTex2DStateSet(osg::ref_ptr<osg::Image> image)
{
	osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
	tex->setImage(image);

	tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
	tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

	tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
	tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
	tex->setMaxAnisotropy(_maxAnsiotropy);
	
	osg::TexEnv *decalTexEnv = new osg::TexEnv();
	decalTexEnv->setMode(osg::TexEnv::DECAL);

	osg::ref_ptr<osg::PolygonMode> pm = new osg::PolygonMode;
	pm->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL);

	osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
	ss->setAttribute(pm);
	ss->setTextureAttributeAndModes(0, tex);
	ss->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
	ss->setTextureAttribute(0, decalTexEnv);

	return ss.release();
}