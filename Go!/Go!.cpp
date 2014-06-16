// Go!.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "headers.h"
#include <fstream>

using namespace std;

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
		carMatrix->addEventCallback(new CarEvent);
	}
	else
	{
		carMatrix->addEventCallback(new CarReplay);
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

int _tmain(int argc, char* argv[])
{
	extern bool init_joystick();
	init_joystick();

	//obtain filename
	string configFile;
	string replayFile;
	osg::ref_ptr<ReadConfig> readConfig;
	if (argc == 1)
	{
		configFile = "..//Resources//config.txt";
		replayFile = "..//Resources//savestate.txt";
		readConfig = new ReadConfig(configFile);
//		readConfig = new ReadConfig(configFile, replayFile);
	}
	else if (argc == 2)
	{
		configFile = argv[1];
		readConfig = new ReadConfig(configFile);
	}
	else if (argc > 2)
	{
		configFile = argv[1];
		replayFile = argv[2];
		readConfig = new ReadConfig(configFile, replayFile);
	}

	//Always Render first and then texture
	osg::ref_ptr<RenderVistor> rv = new RenderVistor;
	rv->setBeginMode(GL_QUADS);
	osg::ref_ptr<TextureVisitor> tv = new TextureVisitor;

	//Build Road && Render Road && Texture Road
	osg::ref_ptr<Road> road = obtainRoad(readConfig);
	road->accept(*rv);
	road->accept(*tv);

	//Build Car & Render Car && Obtain carMatrix
	osg::ref_ptr<Car> car = obtainCar(readConfig);
	rv->reset();
	rv->setBeginMode(GL_POINTS);
	car->accept(*rv);
	osg::ref_ptr<osg::MatrixTransform> carMatrix = obtainCarMatrix(car);

	//Root Node && collect information for tracing car and collision detection
	osg::ref_ptr<osg::Group> root = new osg::Group();
	root->addChild(road.get());
	root->addChild(carMatrix.get()); 
	osg::ref_ptr<CollVisitor> cv = new CollVisitor;
	root->accept(*cv);

	//Collision detect && Trace Car
	osg::ref_ptr<Collision> colldetect = new Collision;
	colldetect->setUserData(cv.get());
	car->addUpdateCallback(colldetect.get());
	osg::ref_ptr<RoadSwitcher> roadSwitcher = new RoadSwitcher;
	roadSwitcher->setUserData(cv.get());
	road->addUpdateCallback(roadSwitcher.get());

	//Record Car
	osg::ref_ptr<Recorder> recorder = new Recorder;
	car->addUpdateCallback(recorder.get());

	//Camera event callback
	osg::ref_ptr<CameraEvent> camMatrix = obtainCamMatrix(readConfig, car);

	//Viewer Setup
	osg::ref_ptr<MulitViewer> mViewer = new MulitViewer;
	mViewer->genMainView(readConfig.get());
	mViewer->getMainView()->setCameraManipulator(camMatrix.get());
	mViewer->getMainView()->setSceneData(root.get());

	mViewer->createHUDView();
	mViewer->setHUDContent(recorder->getStatus());
	mViewer->createBackgroundView();

	//Debug Node
	osg::ref_ptr<DebugNode> debugger = new DebugNode;
	debugger->setUserData(cv.get());
	root->addEventCallback(debugger.get());

	//ExperimentControl
	osg::ref_ptr<ExperimentCallback> expcontroller = new ExperimentCallback(readConfig);
	expcontroller->setUserData(cv.get());
	expcontroller->setHUDCamera(mViewer->getHuDView()->getCamera());
	root->addUpdateCallback(expcontroller);

	mViewer->setRunMaxFrameRate(frameRate);
	mViewer->run();

	//final work
	recorder->output(readConfig.get());
	root->accept(*new DeConstructerVisitor);

	extern void close_joystick();
	close_joystick();
}

/*
int main()
{
	const double R(2.0f);
	const int gear(96);
	const double theta(2 * PI / gear);
	ofstream fileout("circularRoad.txt");
	for (int i = 0; i <= gear;i++)
	{
		double x = R*sin(theta*i);
		double y = R*cos(theta*i);

		std::cout << x << "\t" << y << std::endl;
		fileout << x << "\t" << y << std::endl;
	}
	fileout.close();
	getchar();
}
*/