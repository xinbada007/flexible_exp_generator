// Go!.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "renderVistor.h"
#include "road.h"
#include "car.h"
#include "carEvent.h"
#include "cameraEvent.h"
#include "mulitViewer.h"
#include "collVisitor.h"
#include "collision.h"
#include "switchVisitor.h"
#include "textureVisitor.h"
#include "recorder.h"
#include "debugNode.h"
#include "pickHandler.h"

#include <iostream>

#include <osgViewer/Viewer>
#include <osg/MatrixTransform>

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	//first read config from txt file
	string filename = "../Resources/config.txt";
	osg::ref_ptr<ReadConfig> readConfig = new ReadConfig(filename);

	//Build Road
	osg::ref_ptr<Road> road = new Road;
	road->genRoad(readConfig);

	//This has to be first to be executed
	//set RenderVistor
	osg::ref_ptr<RenderVistor> rv = new RenderVistor;
	rv->setBeginMode(GL_QUADS);
	
	//Render Road
	road->accept(*rv);

	//Always Render first and then texture
	//Texture Road
	osg::ref_ptr<TextureVisitor> tv = new TextureVisitor;
	road->accept(*tv);

	//Build Car & Render Car
	osg::ref_ptr<Car> car = new Car;
	car->genCar(readConfig);
	car->accept(*rv);

	//Car event callback
	osg::ref_ptr<osg::MatrixTransform> carMatrix = new osg::MatrixTransform;
	carMatrix->addEventCallback(new CarEvent);
	carMatrix->setUserData(car.get());
	carMatrix->addChild(car->asGroup());

	//Root Node
	osg::ref_ptr<osg::Group> root = new osg::Group();
	root->addChild(road);
	root->addChild(carMatrix);
	root->addChild(readConfig->measuer());

	//Collision detect && Trace Car
	osg::ref_ptr<CollVisitor> cv = new CollVisitor;
	root->accept(*cv);
	osg::ref_ptr<Collision> colldetect = new Collision;
	colldetect->setUserData(cv.get());
	car->addUpdateCallback(colldetect.get());

	//Record Car
	osg::ref_ptr<Recorder> recorder = new Recorder;
	car->addUpdateCallback(recorder.get());

// 	//Debug Node
	osg::ref_ptr<DebugNode> debugger = new DebugNode;
	debugger->setUserData(cv.get());
	root->addEventCallback(debugger.get());
// 
// 	//Camera event callback
	osg::ref_ptr<CameraEvent> camMatrix = new CameraEvent;
	camMatrix->genCamera(readConfig);
	camMatrix->setUserData(car.get());

	//Viewer Setup
	osg::ref_ptr<MulitViewer> mViewer = new MulitViewer;
	mViewer->genMainViewer(readConfig);
	mViewer->getView(0)->setCameraManipulator(camMatrix.get());
	mViewer->getView(0)->setSceneData(root->asGroup());

	mViewer->run();

	recorder->output(readConfig);
}