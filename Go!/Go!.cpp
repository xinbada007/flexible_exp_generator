// Go!.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "headers.h"

#include <osgViewer/api/Win32/GraphicsHandleWin32>

#include <OVR.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

Road * obtainRoad(ReadConfig *rc)
{
	osg::ref_ptr<Road> road = new Road;
	road->genRoad(rc);

	return road.release();
}

Car * obtainCar(ReadConfig *rc)
{
	osg::ref_ptr<Car> car = new Car;
	car->genCar(rc);

	if (car->getVehicle()->_visibility)
	{
		RenderVistor rv;
		rv.reset();
		rv.setBeginMode(GL_QUADS);
		car->accept(rv);
	}

	osg::ref_ptr<osg::Node> carNode = rc->getVehicle()->_carNode;
	if (carNode)
	{
		osg::ref_ptr<osg::StateSet> ss = carNode->getOrCreateStateSet();
 		ss->setMode(GL_LIGHTING, osg::StateAttribute::ON|osg::StateAttribute::PROTECTED);
		osg::ref_ptr<osg::PolygonMode> pm = new osg::PolygonMode(osg::PolygonMode::FRONT, osg::PolygonMode::FILL);
		ss->setAttribute(pm, osg::StateAttribute::PROTECTED);

		carNode->setDataVariance(osg::Object::STATIC);
		osgUtil::Optimizer op;
		op.optimize(carNode, osgUtil::Optimizer::ALL_OPTIMIZATIONS);
		car->addChild(carNode);
	}

	return car.release();
}

osg::MatrixTransform *obtainCarMatrix(Car *car)
{
	osg::ref_ptr<osg::MatrixTransform> carMatrix = new osg::MatrixTransform;
	if (!car->getCarState()->_saveState)
	{
		osg::ref_ptr<CarEvent> carEvent = new CarEvent;
		carMatrix->addEventCallback(carEvent);
	}
	else
	{
		osg::ref_ptr<CarReplay> carReplay = new CarReplay;
		carMatrix->addEventCallback(carReplay);
	}
	carMatrix->setUserData(car);
	carMatrix->addChild(car);

	return carMatrix.release();
}

CameraEvent *obtainCamMatrix(ReadConfig *rc, Car *car)
{
	osg::ref_ptr<CameraEvent> camMatrix = new CameraEvent(rc);
	camMatrix->setUserData(car);

	return camMatrix.release();
}

void runScene(ReadConfig *readConfig)
{
	//Always Render first and then texture
	osg::ref_ptr<RenderVistor> rv = new RenderVistor;
	osg::ref_ptr<TextureVisitor> tv = new TextureVisitor;

	//Build Road && Render Road && Texture Road
	osg::ref_ptr<Road> road;
	road = obtainRoad(readConfig);
	if (road->getRoadSet()->_visible)
	{
		rv->setBeginMode(GL_QUADS);
		road->accept(*rv);
		road->accept(*tv);
	}
	//Visit to find walls and roads
	CollVisitor::instance()->reset();
	CollVisitor::instance()->setMode(ROADTAG::ROAD);
	road->accept(*CollVisitor::instance());
	CollVisitor::instance()->setMode(ROADTAG::RWALL);
	road->accept(*CollVisitor::instance());
	CollVisitor::instance()->setMode(ROADTAG::LWALL);
	road->accept(*CollVisitor::instance());

	//Build Car & Render Car && Obtain carMatrix
	osg::ref_ptr<Car> car;
	car = obtainCar(readConfig);
	osg::ref_ptr<osg::MatrixTransform> carMatrix;
	carMatrix = obtainCarMatrix(car);

	//Root Node && collect information for tracing car and collision detection
	osg::ref_ptr<osg::Group> root;
	root = new osg::Group;
	root->addChild(road);
	root->addChild(carMatrix);
	//Measure
//	root.back()->addChild(readConfig.back()->measuer());

	//Collision detect && Trace Car
	osg::ref_ptr<Collision> colldetect;
	colldetect = new Collision;
	car->addUpdateCallback(colldetect);
	osg::ref_ptr<RoadSwitcher> roadSwitcher;
	roadSwitcher = new RoadSwitcher;
	roadSwitcher->setCarState(car->getCarState());
	road->addUpdateCallback(roadSwitcher);

	//Record Car
	osg::ref_ptr<Recorder> recorder;
	recorder = new Recorder(readConfig);
	car->addUpdateCallback(recorder);

	//ExperimentControl
	osg::ref_ptr<ExperimentCallback> expcontroller;
	expcontroller = new ExperimentCallback(readConfig);
	expcontroller->setCar(car);
	root->addEventCallback(expcontroller);

	//Camera event callback
	osg::ref_ptr<CameraEvent> camMatrix;
	camMatrix = obtainCamMatrix(readConfig, car);

	//Viewer Setup
	osg::ref_ptr<MulitViewer> mViewer = new MulitViewer(readConfig);
	mViewer->genMainView();
//	mViewer->getMainView()->addEventHandler(new osgViewer::StatsHandler);
	mViewer->getMainView()->setCameraManipulator(camMatrix);
	mViewer->setMainViewSceneData(root);
	mViewer->createHUDView();
	mViewer->createBackgroundView();

	recorder->setHUDCamera(mViewer->getHUDCamera(MulitViewer::LEFT));
	expcontroller->setHUDCamera(mViewer->getHUDCamera(MulitViewer::CENTRE));
	expcontroller->setViewer(mViewer);

	mViewer->go();

	//after run output records
	recorder->output();
	//after run set ref_ptr to null
	root->accept(*new DeConstructerVisitor);
	mViewer = NULL;
	recorder = NULL;
	readConfig = NULL;
	root = NULL;
	road = NULL;
	car = NULL;
	carMatrix = NULL;
	colldetect = NULL;
	roadSwitcher = NULL;
	camMatrix = NULL;
	expcontroller = NULL;
	CollVisitor::instance()->reset();
}

int main(int argc, char** argv)
{
	bool replayM(false);
	std::string configFile = "..\\Resources\\config.txt";
	configFile = osgDB::fileExists(configFile) ? configFile : "";
	std::string replayFile = "";
	if (argc >= 2)
	{
		configFile = osgDB::fileExists(argv[1]) ? argv[1] : "";
		if (argc == 3)
		{
			replayFile = osgDB::fileExists(argv[2]) ? argv[2] : "";
			replayM = true;
		}
	}
	if (configFile.empty() || (replayM == true && replayFile.empty()))
	{
		osg::notify(osg::FATAL) << "cannot find config file or replay file!" << std::endl;
		osg::notify(osg::FATAL) << "configFile:\t" << configFile
								<< "\nreplayFile:\t" << replayFile << std::endl;
		system("pause");
		return 0;
	}

	osg::ref_ptr<ReadConfig> readConfig = replayM ? new ReadConfig(configFile, replayFile) :
													new ReadConfig(configFile);

	//before running open joystick and sound
	extern bool init_joystick();
	init_joystick();
	osgAudio::SoundManager::instance()->init(16, true);

	runScene(readConfig);

	//after running shutdown sound and joystick
	osgAudio::SoundManager::instance()->stopAllSources();
	extern void close_joystick();
	close_joystick();

	if (osg::Referenced::getDeleteHandler())
	{
		osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
		osg::Referenced::getDeleteHandler()->flushAll();
	}

	osgAudio::SoundManager::instance()->instance()->shutdown();

	//exit the main function
	return 0;
}

//Debug Node
// 		osg::ref_ptr<DebugNode> debugger = new DebugNode;
// 		debugger->setUserData(cv.get());
// 		root.back()->addEventCallback(debugger.get());
// 		root.back()->addChild(readConfig.back()->measuer());