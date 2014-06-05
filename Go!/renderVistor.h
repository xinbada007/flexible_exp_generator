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

	inline void setBeginMode(int ref){ _beginMode = ref; };
private:
	osg::Geode::DrawableList _drawableList;
	int _beginMode;

	void setStateset(osg::ref_ptr<osg::StateSet> stateSet);
	void draw() const;
	void render(osg::Drawable *refD) const;
};

