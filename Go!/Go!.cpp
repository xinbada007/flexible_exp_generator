// Go!.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "headers.h"
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

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
	int curRep(1);
	int totalRep(1);
	bool replyM(false);
// 	if (argc >= 3)
// 	{
// 		totalRep = stoi((argv[1]));
// 		if (!totalRep)
// 		{
// 			totalRep = 1;
// 			replyM = true;
// 		}
// 	}
// 	else
// 	{
// 		std::cout << ("Require at least 1 input\n") << std::endl;
// 		return 0;
// 	}
	totalRep = 2;

	extern bool init_joystick();
	init_joystick();

	osgAudio::SoundManager::instance()->init(16, true);

	std::vector<osg::ref_ptr<ReadConfig>> readConfig;
	std::vector<osg::ref_ptr<Road>> road;
	std::vector<osg::ref_ptr<Car>> car;
	std::vector<osg::ref_ptr<osg::MatrixTransform>> carMatrix;
	std::vector<osg::ref_ptr<osg::Group>> root;
	std::vector<osg::ref_ptr<Collision>> colldetect;
	std::vector<osg::ref_ptr<RoadSwitcher>> roadSwitcher;
	std::vector<osg::ref_ptr<CameraEvent>> camMatrix;
	std::vector<osg::ref_ptr<MulitViewer>> mViewer;
	std::vector<osg::ref_ptr<Recorder>> recorder;
	std::vector<osg::ref_ptr<ExperimentCallback>> expcontroller;
	
	while (curRep <= totalRep)
	{
		//obtain filename
//		string configFile = argv[1 + curRep];
		string configFile = "..\\Resources\\config.txt";
		string replayFile;
		if (replyM)
		{
			replayFile = argv[1 + curRep + 1];
			readConfig.push_back(new ReadConfig(configFile, replayFile));
		}
		else
		{
			readConfig.push_back(new ReadConfig(configFile));
		}

		//Always Render first and then texture
		osg::ref_ptr<RenderVistor> rv = new RenderVistor;
		osg::ref_ptr<TextureVisitor> tv = new TextureVisitor;

		//Build Road && Render Road && Texture Road
		road.push_back(obtainRoad(readConfig.back()));
		if (road.back()->getRoadSet()->_visible)
		{
			rv->setBeginMode(GL_QUADS);
			road.back()->accept(*rv);
			road.back()->accept(*tv);
		}
		CollVisitor *cv = CollVisitor::instance();
		cv->setMode(ROADTAG::ROAD);
		road.back()->accept(*cv);
		cv->setMode(ROADTAG::RWALL);
		road.back()->accept(*cv);
		cv->setMode(ROADTAG::LWALL);
		road.back()->accept(*cv);

		//Build Car & Render Car && Obtain carMatrix
		car.push_back(obtainCar(readConfig.back()));
		if (car.back()->getVehicle()->_visibility)
		{
			rv->reset();
			rv->setBeginMode(GL_POINTS);
			car.back()->accept(*rv);
		}
		carMatrix.push_back(obtainCarMatrix(car.back()));

		//Root Node && collect information for tracing car and collision detection
		root.push_back(new osg::Group());
		root.back()->addChild(road.back().get());
		root.back()->addChild(carMatrix.back().get());

		//Collision detect && Trace Car
		colldetect.push_back(new Collision);
		car.back()->addUpdateCallback(colldetect.back().get());
		roadSwitcher.push_back(new RoadSwitcher);
		roadSwitcher.back()->setCarState(car.back()->getCarState());
		road.back()->addUpdateCallback(roadSwitcher.back().get());

		//Camera event callback
		camMatrix.push_back(obtainCamMatrix(readConfig.back(), car.back().get()));

		//Viewer Setup
		mViewer.push_back(new MulitViewer);
		mViewer.back()->genMainView(readConfig.back().get());
		mViewer.back()->getMainView()->setCameraManipulator(camMatrix.back().get());
		//	mViewer->getMainView()->addEventHandler(new osgViewer::StatsHandler);
		mViewer.back()->getMainView()->setSceneData(root.back().get());
		mViewer.back()->createHUDView();
		mViewer.back()->createBackgroundView();

		//Record Car
		recorder.push_back(new Recorder(readConfig.back().get()));
		recorder.back()->setHUDCamera(mViewer.back()->getHUDCamera());
		car.back()->addUpdateCallback(recorder.back().get());

		//ExperimentControl
		expcontroller.push_back(new ExperimentCallback(readConfig.back()));
		expcontroller.back()->setCarState(car.back()->getCarState());
		expcontroller.back()->setHUDCamera(mViewer.back()->getHUDCamera());
		expcontroller.back()->setMultiViewer(mViewer.back().get());
		root.back()->addEventCallback(expcontroller.back().get());

		root.back()->setDataVariance(osg::Object::DYNAMIC);

		osgUtil::Optimizer optimizer;
//	 	optimizer.optimize(root.back().get(), osgUtil::Optimizer::SHARE_DUPLICATE_STATE|osgUtil::Optimizer::OPTIMIZE_TEXTURE_SETTINGS|
//	 		osgUtil::Optimizer::INDEX_MESH|osgUtil::Optimizer::VERTEX_PRETRANSFORM|osgUtil::Optimizer::VERTEX_POSTTRANSFORM);
		optimizer.optimize(root.back().get());

		mViewer.back()->setRunMaxFrameRate(frameRate::instance()->getDesignfRate());
		osgViewer::ViewerBase::ThreadingModel th = osgViewer::ViewerBase::ThreadPerCamera;
		mViewer.back()->setThreadingModel(th);

		++curRep;
	}

	//Sound&Music
	osg::ref_ptr<MusicPlayer> musicplayer = new MusicPlayer;

	//Ready to Run
	int i = 0;
	while (i < totalRep)
	{
		MulitViewer *viewer = mViewer.at(i);
		musicplayer->setHUDCamera(viewer->getHUDCamera());
		viewer->getMainView()->addEventHandler(musicplayer);
		viewer->run();

		Recorder *rec = recorder.at(i);
		ReadConfig *rconfig = readConfig.at(i);
		rec->output();

		osg::Group *rt = root.at(i);
		rt->accept(*new DeConstructerVisitor);

		mViewer.at(i) = NULL;
		recorder.at(i) = NULL;
		readConfig.at(i) = NULL;
		root.at(i) = NULL;
		road.at(i) = NULL;
		car.at(i) = NULL;
		carMatrix.at(i) = NULL;
		colldetect.at(i) = NULL;
		roadSwitcher.at(i) = NULL;
		camMatrix.at(i) = NULL;
		expcontroller.at(i) = NULL;

		if (osg::Referenced::getDeleteHandler()) {
			osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
			osg::Referenced::getDeleteHandler()->flushAll();
		}

		osgAudio::SoundState *sound = osgAudio::SoundManager::instance()->findSoundState("GOsiren");
		if (sound)
		{
			sound->setPlay(false);
		}
		else
		{
			osgAudio::SoundManager::instance()->stopAllSources();
		}

		++i;
	}

	extern void close_joystick();
	close_joystick();

	osgAudio::SoundManager::instance()->instance()->shutdown();

	//exit the main function
	return 0;
}


//Debug Node
// 		osg::ref_ptr<DebugNode> debugger = new DebugNode;
// 		debugger->setUserData(cv.get());
// 		root.back()->addEventCallback(debugger.get());
// 		root.back()->addChild(readConfig.back()->measuer());
