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

// 	osg::ref_ptr<osgViewer::ViewerBase> viewer = new MulitViewer(readConfig);
// 	osg::ref_ptr<MulitViewer> mViewer(NULL);
// 	if (!readConfig->getScreens()->_HMD)
// 	{
// 		viewer = new MulitViewer(readConfig);
// 		mViewer = static_cast<MulitViewer*>(&(*viewer));
// 		mViewer->genMainView();
// 		mViewer->createHUDView();
// 		mViewer->createBackgroundView();
// 		mViewer->setRunMaxFrameRate(frameRate::instance()->getDesignfRate());
// 		osgViewer::ViewerBase::ThreadingModel th = osgViewer::ViewerBase::ThreadPerCamera;
// 		mViewer->setThreadingModel(th);
// 	}
// 	else
// 	{
// 		viewer = new MulitViewer(readConfig);
// 		osg::ref_ptr<MulitViewer> mViewer = static_cast<MulitViewer*>(&(*viewer));
// 		mViewer->hmd_Initialise();
// 	}

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

/*
osg::Camera* createRTTCamera(osg::Camera::BufferComponent buffer, osg::Texture* tex)
{
	osg::ref_ptr<osg::Camera> camera = new osg::Camera;
	camera->setClearColor(osg::Vec4(135.f / 255.f, 206.f / 255.f, 250.f / 255.f, 1.0f));
	camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
	camera->setRenderOrder(osg::Camera::PRE_RENDER);
	if (tex)
	{
		tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
		tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
		camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
		camera->attach(buffer, tex);
		camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	}

	return camera.release();
}

osg::Camera *createSlaveCamera(osgViewer::Viewer *viewer, osg::ref_ptr<osg::Texture2D> tex, const int startX)
{
	osg::GraphicsContext *context = viewer->getCamera()->getGraphicsContext();

	osg::ref_ptr<osg::Camera> camera = createRTTCamera(osg::Camera::COLOR_BUFFER, tex);
	camera->setGraphicsContext(context);
	camera->setView(viewer);

	return camera.release();
}

std::vector<osg::ref_ptr<ReadConfig>> readConfig;
std::vector<osg::ref_ptr<Road>> road;
std::vector<osg::ref_ptr<Car>> car;
std::vector<osg::ref_ptr<osg::MatrixTransform>> carMatrix;
std::vector<osg::ref_ptr<osg::Group>> root;
std::vector<osg::ref_ptr<Collision>> colldetect;
std::vector<osg::ref_ptr<RoadSwitcher>> roadSwitcher;
std::vector<osg::ref_ptr<CameraEvent>> camMatrix;
std::vector<osg::ref_ptr<Recorder>> recorder;
std::vector<osg::ref_ptr<ExperimentCallback>> expcontroller;

void createScene()
{
	//obtain filename
	std::string configFile = "..\\Resources\\config.txt";
	readConfig.push_back(new ReadConfig(configFile));

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

	//Record Car
	recorder.push_back(new Recorder(readConfig.back().get()));
	car.back()->addUpdateCallback(recorder.back().get());

	//ExperimentControl
	expcontroller.push_back(new ExperimentCallback(readConfig.back()));
	expcontroller.back()->setCar(car.back().get());
	root.back()->addEventCallback(expcontroller.back().get());
}


int g_DistortionCaps = 0
| ovrDistortionCap_Vignette
| ovrDistortionCap_Chromatic
| ovrDistortionCap_Overdrive
| ovrDistortionCap_TimeWarp // Turning this on gives ghosting???
;

ovrHmd g_Hmd;
ovrGLConfig g_Cfg;
ovrEyeRenderDesc g_EyeRenderDesc[2];
ovrVector3f g_EyeOffsets[2];
ovrPosef g_EyePoses[2];
ovrTexture g_EyeTextures[2];
OVR::Matrix4f g_ProjectionMatrici[2];
OVR::Sizei g_RenderTargetSize;


int main()
{
	extern bool init_joystick();
	init_joystick();

	ovrBool ini = ovr_Initialize();
	g_Hmd = ovrHmd_Create(0);
	if (!g_Hmd)
	{
		g_Hmd = ovrHmd_CreateDebug(ovrHmd_DK2);
	}

	uint32_t l_SupportedSensorCaps = ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection;
	uint32_t l_RequiredTrackingCaps = 0;
	ovrBool l_TrackingResult = ovrHmd_ConfigureTracking(g_Hmd, l_SupportedSensorCaps, l_RequiredTrackingCaps);

	ovrSizei windowsR = g_Hmd->Resolution;
	windowsR.w /= 2;
	windowsR.h /= 2;

	osgViewer::Viewer viewer;
	viewer.setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
	viewer.setUpViewInWindow(0, 0, windowsR.w, windowsR.h, 0);
	createScene();
	viewer.setSceneData(root.back());
	viewer.setCameraManipulator(camMatrix.back());

	ovrSizei l_EyeTextureSizes[2];
	l_EyeTextureSizes[ovrEye_Left] = ovrHmd_GetFovTextureSize(g_Hmd, ovrEye_Left, g_Hmd->DefaultEyeFov[ovrEye_Left], 1.0f);
	l_EyeTextureSizes[ovrEye_Right] = ovrHmd_GetFovTextureSize(g_Hmd, ovrEye_Right, g_Hmd->DefaultEyeFov[ovrEye_Right], 1.0f);

	ovrSizei TargetSize;
	TargetSize.w = l_EyeTextureSizes[ovrEye_Left].w + l_EyeTextureSizes[ovrEye_Right].w;
	TargetSize.h = max(l_EyeTextureSizes[ovrEye_Left].h, l_EyeTextureSizes[ovrEye_Right].h);

	osg::ref_ptr<osg::Texture2D> master_tex = new osg::Texture2D;
	master_tex->setTextureSize(TargetSize.w, TargetSize.h);
	master_tex->setInternalFormat(GL_RGBA);
	master_tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
	master_tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

	viewer.getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
	viewer.getCamera()->setViewport(0, 0, master_tex->getTextureWidth(), master_tex->getTextureHeight());
	viewer.getCamera()->setRenderOrder(osg::Camera::PRE_RENDER);
	viewer.getCamera()->attach(osg::Camera::COLOR_BUFFER, master_tex);

	osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits(*viewer.getCamera()->getGraphicsContext()->getTraits());
	traits->vsync = false;
	osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits);
	viewer.getCamera()->setGraphicsContext(gc);

	osg::Camera *leftCam = createSlaveCamera(&viewer, master_tex, 0);
	osg::Camera *rightCam = createSlaveCamera(&viewer, master_tex, 1);

	viewer.addSlave(leftCam, osg::Matrix(), osg::Matrix(), true);
	viewer.addSlave(rightCam, osg::Matrix(), osg::Matrix(), true);

	GLenum drawbuffer = viewer.getCamera()->getDrawBuffer();
	GLenum readbuffer = viewer.getCamera()->getReadBuffer();
	drawbuffer = leftCam->getDrawBuffer();
	readbuffer = leftCam->getReadBuffer();
	drawbuffer = rightCam->getDrawBuffer();
	readbuffer = rightCam->getReadBuffer();

	viewer.getCamera()->setDrawBuffer(GL_NONE);
	leftCam->setDrawBuffer(GL_NONE);
	rightCam->setDrawBuffer(GL_NONE);
	viewer.getCamera()->setReadBuffer(GL_NONE);
	leftCam->setReadBuffer(GL_NONE);
	rightCam->setReadBuffer(GL_NONE);
	gc->setSwapCallback(new swapcallback);

	viewer.realize();
	osgViewer::ViewerBase::Windows windows;
	viewer.getWindows(windows);
	if (windows.size() > 1)
	{
		return 1;
	}
	osgViewer::GraphicsHandleWin32 *osgwin = dynamic_cast<osgViewer::GraphicsHandleWin32*>(windows.front());
	HWND hWnd = osgwin->getHWND();
	HDC hDc = GetDC(hWnd);

	ovrHmd_AttachToWindow(g_Hmd, hWnd, NULL, NULL);

	viewer.frame();

	viewer.getCamera()->getGraphicsContext()->makeCurrent();

	std::cout << glGetString(GL_VERSION) << std::endl;

	g_Cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	g_Cfg.OGL.Header.BackBufferSize = windowsR;
	g_Cfg.OGL.Header.Multisample = 0;
	g_Cfg.OGL.Window = hWnd;
	g_Cfg.OGL.DC = GetDC(hWnd);

	ovrHmd_ConfigureRendering(g_Hmd, &g_Cfg.Config, g_DistortionCaps, g_Hmd->MaxEyeFov, g_EyeRenderDesc);

	g_ProjectionMatrici[ovrEye_Left] = ovrMatrix4f_Projection(g_EyeRenderDesc[ovrEye_Left].Fov, 0.1f, 10000.f, true);
	g_ProjectionMatrici[ovrEye_Right] = ovrMatrix4f_Projection(g_EyeRenderDesc[ovrEye_Right].Fov, 0.1f, 10000.f, true);

	g_EyeOffsets[ovrEye_Left] = g_EyeRenderDesc[ovrEye_Left].HmdToEyeViewOffset;
	g_EyeOffsets[ovrEye_Right] = g_EyeRenderDesc[ovrEye_Right].HmdToEyeViewOffset;

	osg::Matrix l_proj_M, r_proj_M, l_view_M, r_view_M;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			l_proj_M(i, j) = g_ProjectionMatrici[ovrEye_Left].Transposed().M[i][j];
			r_proj_M(i, j) = g_ProjectionMatrici[ovrEye_Right].Transposed().M[i][j];
		}
	}
	l_view_M = osg::Matrix::translate(g_EyeOffsets[ovrEye_Left].x, g_EyeOffsets[ovrEye_Left].y, g_EyeOffsets[ovrEye_Left].z);
	r_view_M = osg::Matrix::translate(g_EyeOffsets[ovrEye_Right].x, g_EyeOffsets[ovrEye_Right].y, g_EyeOffsets[ovrEye_Right].z);

	unsigned contextID = viewer.getCamera()->getGraphicsContext()->getState()->getContextID();
	GLuint master_TexID = 20;
	if (master_tex->getTextureObject(contextID))
	{
		master_TexID = master_tex->getTextureObject(contextID)->id();
	}

	ovrGLTexture leftTex;
	leftTex.OGL.Header.API = ovrRenderAPI_OpenGL;
	leftTex.OGL.Header.TextureSize = TargetSize;
	leftTex.OGL.Header.RenderViewport.Pos.x = 0;
	leftTex.OGL.Header.RenderViewport.Pos.y = 0;
	leftTex.OGL.Header.RenderViewport.Size = l_EyeTextureSizes[ovrEye_Left];
	leftTex.OGL.TexId = master_TexID;

	ovrGLTexture rightTex;
	rightTex.OGL.Header.API = ovrRenderAPI_OpenGL;
	rightTex.OGL.Header.TextureSize = TargetSize;
	rightTex.OGL.Header.RenderViewport.Pos.x = l_EyeTextureSizes[ovrEye_Left].w;
	rightTex.OGL.Header.RenderViewport.Pos.y = 0;
	rightTex.OGL.Header.RenderViewport.Size = l_EyeTextureSizes[ovrEye_Right];
	rightTex.OGL.TexId = master_TexID;

	g_EyeTextures[ovrEye_Left] = leftTex.Texture;
	g_EyeTextures[ovrEye_Right] = rightTex.Texture;

	leftCam->setViewport(g_EyeTextures[ovrEye_Left].Header.RenderViewport.Pos.x,
		g_EyeTextures[ovrEye_Left].Header.RenderViewport.Pos.y,
		g_EyeTextures[ovrEye_Left].Header.RenderViewport.Size.w,
		g_EyeTextures[ovrEye_Left].Header.RenderViewport.Size.h);
	rightCam->setViewport(g_EyeTextures[ovrEye_Left].Header.RenderViewport.Size.w,
		g_EyeTextures[ovrEye_Right].Header.RenderViewport.Pos.y,
		g_EyeTextures[ovrEye_Right].Header.RenderViewport.Size.w,
		g_EyeTextures[ovrEye_Right].Header.RenderViewport.Size.h);

	leftCam->setProjectionMatrix(l_proj_M);
	rightCam->setProjectionMatrix(r_proj_M);

	ovrHmd_RecenterPose(g_Hmd);
	ovrFrameTiming l_HmdFrameTiming;

	typedef BOOL(GL_APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);
	PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");

	while (!viewer.done())
	{
		const unsigned &frameIndex = viewer.getFrameStamp()->getFrameNumber();
		l_HmdFrameTiming = ovrHmd_BeginFrame(g_Hmd, frameIndex);
		ovrHmd_GetEyePoses(g_Hmd, frameIndex, g_EyeOffsets, g_EyePoses, NULL);

		OVR::Quatd leftOrientation = OVR::Quatd(g_EyePoses[ovrEye_Left].Orientation);
		OVR::Matrix4d leftMVMatrix = OVR::Matrix4d(leftOrientation.Inverted());
		OVR::Quatd rightOrientation = OVR::Quatd(g_EyePoses[ovrEye_Right].Orientation);
		OVR::Matrix4d rightMVMatrix = OVR::Matrix4d(rightOrientation.Inverted());
		osg::Matrixd leftMatrix, rightMatrix;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				leftMatrix(i, j) = leftMVMatrix.Transposed().M[i][j];
				rightMatrix(i, j) = rightMVMatrix.Transposed().M[i][j];
			}
		}

		leftCam->setViewMatrix(viewer.getCamera()->getViewMatrix() * leftMatrix * l_view_M);
		rightCam->setViewMatrix(viewer.getCamera()->getViewMatrix() * rightMatrix * r_view_M);

		if (wglSwapIntervalEXT)
		{
			wglSwapIntervalEXT(0);
		}

		viewer.frame();
		viewer.getCamera()->getGraphicsContext()->makeCurrent();
		ovrHmd_EndFrame(g_Hmd, g_EyePoses, g_EyeTextures);
	}

	ovrHmd_Destroy(g_Hmd);
	ovr_Shutdown();

	extern void close_joystick();
	close_joystick();

	return 0;
}
*/
