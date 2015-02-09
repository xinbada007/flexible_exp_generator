// Go!.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "headers.h"

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
	osg::ref_ptr<CameraEvent> camMatrix = new CameraEvent;
	camMatrix->genCamera(rc);
	camMatrix->setUserData(car);

	return camMatrix.release();
}

int main(int argc, char** argv)
{
	bool replayM(false);
	std::string configFile = "..\\Resources\\config.txt";
	std::string replayFile = "";
	if (argc == 2)
	{
		configFile = argv[1];
	}
	else if (argc == 3)
	{
		configFile = argv[1];
		replayFile = argv[2];
		replayM = true;
	}
	configFile = osgDB::fileExists(configFile) ? configFile : "";
	replayFile = osgDB::fileExists(replayFile) ? replayFile : "";
	if (configFile.empty() || (replayM == true && replayFile.empty()))
	{
		osg::notify(osg::FATAL) << "cannot find config file or replay file!" << std::endl;
		system("pause");
		return 0;
	}

	extern bool init_joystick();
	init_joystick();

	osgAudio::SoundManager::instance()->init(16, true);

	osg::ref_ptr<ReadConfig> readConfig;
	if (replayM)
	{
		readConfig = new ReadConfig(configFile, replayFile);
	}
	else
	{
		readConfig = new ReadConfig(configFile);
	}

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

	//Build Car & Render Car && Obtain carMatrix
	
	osg::ref_ptr<Car> car;
	car = obtainCar(readConfig);
	if (car->getVehicle()->_visibility)
	{
		rv->reset();
		rv->setBeginMode(GL_POINTS);
		car->accept(*rv);
	}
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

	//Camera event callback
	osg::ref_ptr<CameraEvent> camMatrix;
	camMatrix = obtainCamMatrix(readConfig, car);

	//Viewer Setup
	osg::ref_ptr<MulitViewer> mViewer;
	mViewer = new MulitViewer;
	mViewer->genMainView(readConfig);
	mViewer->getMainView()->setCameraManipulator(camMatrix);
	//mViewer->getMainView()->addEventHandler(new osgViewer::StatsHandler);
	mViewer->getMainView()->setSceneData(root);
	mViewer->createHUDView();
	mViewer->createBackgroundView();
	mViewer->setRunMaxFrameRate(frameRate::instance()->getDesignfRate());
	osgViewer::ViewerBase::ThreadingModel th = osgViewer::ViewerBase::ThreadPerCamera;
	mViewer->setThreadingModel(th);

	//Record Car
	osg::ref_ptr<Recorder> recorder;
	recorder = new Recorder(readConfig);
	recorder->setHUDCamera(mViewer->getHUDCamera(MulitViewer::LEFT));
	car->addUpdateCallback(recorder);

	//ExperimentControl
	osg::ref_ptr<ExperimentCallback> expcontroller;
	expcontroller = new ExperimentCallback(readConfig);
	expcontroller->setCar(car);
	expcontroller->setHUDCamera(mViewer->getHUDCamera(MulitViewer::CENTRE));
	expcontroller->setMultiViewer(mViewer);
	root->addEventCallback(expcontroller);

//	osgUtil::Optimizer optimizer;
//	optimizer.optimize(root);
// 	optimizer.optimize(root, osgUtil::Optimizer::SHARE_DUPLICATE_STATE|osgUtil::Optimizer::OPTIMIZE_TEXTURE_SETTINGS|
// 		osgUtil::Optimizer::INDEX_MESH|osgUtil::Optimizer::VERTEX_PRETRANSFORM|osgUtil::Optimizer::VERTEX_POSTTRANSFORM);

	//Ready to Run
	CollVisitor::instance()->reset();
	CollVisitor::instance()->setMode(ROADTAG::ROAD);
	road->accept(*CollVisitor::instance());
	CollVisitor::instance()->setMode(ROADTAG::RWALL);
	road->accept(*CollVisitor::instance());
	CollVisitor::instance()->setMode(ROADTAG::LWALL);
	road->accept(*CollVisitor::instance());
	//Run
	mViewer->run();

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

	//after run shutdown sound and joystick
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
