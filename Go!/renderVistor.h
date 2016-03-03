#pragma once
#include <osg/NodeVisitor>
#include <osg/Geode>

class RenderVistor:public osg::NodeVisitor
{
public:
	RenderVistor();
	~RenderVistor();

	virtual void reset();
	virtual void apply(osg::Group &refNode);
	virtual void apply(osg::Geode &refGeode);

	inline void setBeginMode(int ref, const unsigned numLim = 0, const int candidate = GL_POINTS)
	{
		_beginMode = ref; 
		_numLim = numLim;
		_candidate = candidate;
	};

private:
	std::vector<osg::Drawable*> _drawableList;
//	osg::Geode::DrawableList _drawableList;
	int _beginMode;
	unsigned _numLim;
	unsigned _candidate;

	void setStateset(osg::ref_ptr<osg::StateSet> stateSet);
	void draw() const;
	void render(osg::Drawable *refD) const;
	void fetchDrawableList(osg::Geode &geode);
};

