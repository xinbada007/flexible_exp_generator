#pragma once
#include "eulerPoly.h"
#include "readConfig.h"

class NurbsCurve;
struct edgeFlag;

typedef enum ROADTAG
{
	MIDQUAD,
	CTRL,
	LWALL, RWALL,
	ROAD
}ROADTAG;

typedef enum FLAGTYPE
{
	NAFLAG,EDGEFLAG
}FLAGTYPE;

typedef std::vector<osg::ref_ptr<osg::Vec3Array>> FlagEdgeArrayList;

class LogicRoad :
	public EulerPoly
{
public:
	LogicRoad();
	LogicRoad(const LogicRoad &copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY);
	META_Node(LogicRoad, LogicRoad);

	void line1D(const osg::Vec3Array *V1, const osg::Vec3Array *V2);
	void line1D(const osg::Vec3Array *refV);
	void sweep1D(const osg::Vec3Array *refV);

	void traverse();

	inline LogicRoad * getNext() const { return _next; };
	inline void setNext(LogicRoad *ref) 
	{
		this->_next = ref;
		Solid *s = dynamic_cast<Solid*>(this);
		if (s)
		{
			s->setNext(dynamic_cast<Solid*>(ref));
		}
	};
	inline LogicRoad * getPrev() const { return _prev; };
	inline void setPrev(LogicRoad *ref)
	{
		this->_prev = ref; 
		Solid *s = dynamic_cast<Solid*>(this);
		if (s)
		{
			Solid *prv = dynamic_cast<Solid*>(ref);
			s->setPrev(prv);
			if (prv)
			{
				s->setIndex(prv->getIndex() + 1);
			}
		}
	};
	inline ROADTAG getTag() const { return _tag; };
	inline void setTag(ROADTAG ref) { _tag = ref; };
	inline void setEFlag(edgeFlag *ref) { _eFlag = ref; };

	inline void setRawInformation(RoadSet *refRS,int i)
	{
		_roadTxt = refRS->_roadTxt.at(i);
		_texFile = refRS->_texture;
		_width = refRS->_width;
		_density = refRS->_density;
		_nurbs = new osg::Vec3Array(refRS->_nurbs.at(i)->_path->begin(), refRS->_nurbs.at(i)->_path->end());
	}

	inline osg::ref_ptr<osg::Vec3Array> getRawPoints() const
	{
		return _nurbs;
	}

	inline void setEdgeFlagArray(FlagEdgeArrayList &ref) { _eFlagArray = ref; };

	void setUpFlag();

protected:
	~LogicRoad();
private:	
	edgeFlag *_eFlag;
	FlagEdgeArrayList _eFlagArray;

	osg::ref_ptr<osg::Vec3Array> _project_LineV;
	osg::ref_ptr<LogicRoad> _next;
	osg::ref_ptr<LogicRoad> _prev;
	ROADTAG _tag;

	//Raw information for logicRoad
	std::string _roadTxt;
	double _width;
	unsigned _density;
	osg::ref_ptr<osg::Vec3Array> _nurbs;
};

