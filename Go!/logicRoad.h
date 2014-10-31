#pragma once
#include "eulerPoly.h"
#include "readConfig.h"
#include "edge.h"

class NurbsCurve;

typedef enum ROADTAG
{
	RT_UNSPECIFIED,
	MIDQUAD,
	CTRL,
	LWALL, RWALL,
	ROAD,
	OBS
}ROADTAG;

typedef enum FLAGTYPE
{
	NAFLAG,EDGEFLAG
}FLAGTYPE;

typedef std::vector<osg::ref_ptr<osg::Vec3dArray>> FlagEdgeArrayList;

class LogicRoad :
	public EulerPoly
{
public:
	LogicRoad();
	LogicRoad(const LogicRoad &copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY);
	META_Node(LogicRoad, LogicRoad);

	void line1D(const osg::Vec3dArray *V1, const osg::Vec3dArray *V2);
	void line1D(const osg::Vec3dArray *refV);
	void sweep1D(const osg::Vec3dArray *refV);

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
	inline void setEFlag(osg::ref_ptr<edgeFlag> ref) { _eFlag = ref; };

	inline void setRawInformation(RoadSet *refRS,int i, ROADTAG refTag)
	{
		_tag = refTag;
		_roadTxt = refRS->_roadTxt.at(i);
		_density = refRS->_density;
		if (refTag ==  ROAD)
		{
			_width = refRS->_width;
			_imgTexture = refRS->_imgRoad;
		}
		else if (refTag == LWALL || refTag == RWALL)
		{
			_width = refRS->_wallHeight;
			_imgTexture = refRS->_imgWall;
		}
		else
		{
			_width = 0.0f;
			_imgTexture = NULL;
		}
	}

	inline void setEdgeFlagArray(FlagEdgeArrayList &ref) { _eFlagArray = ref; };

	void setUpFlag();

protected:
	~LogicRoad();
	ROADTAG _tag;

private:	
	osg::ref_ptr<edgeFlag> _eFlag;
	FlagEdgeArrayList _eFlagArray;

	osg::ref_ptr<osg::Vec3dArray> _project_LineV;
	osg::ref_ptr<LogicRoad> _next;
	osg::ref_ptr<LogicRoad> _prev;

	//Raw information for logicRoad
	std::string _roadTxt;
	double _width;
	int _density;
};

