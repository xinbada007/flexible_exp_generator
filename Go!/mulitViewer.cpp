#include "stdafx.h"
#include "mulitViewer.h"

#include <osg/Notify>
#include <osgViewer/Viewer>
#include <osg/Depth>
#include <osgDB/ReadFile>
#include <osg/Multisample>

#include <osgViewer/api/Win32/GraphicsHandleWin32>

#include <OVR.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

MulitViewer::MulitViewer(osg::ref_ptr<ReadConfig> refRC):
_screens(refRC->getScreens()), _mainView(NULL), _HUDView(NULL), _HUDText(NULL), _BGView(NULL), _hmdViewer(NULL), _masterTex(NULL)
{
}

MulitViewer::~MulitViewer()
{
	std::cout << "Deconstruct MulitViewer" << std::endl;

	_screens = NULL;
	_mainView = NULL;
	std::vector<osg::Camera*>::iterator cam_i = _slaveCamerasinMainView.begin();
	while (cam_i != _slaveCamerasinMainView.end())
	{
		*cam_i = NULL;
		++cam_i;
	}
	_HUDView = NULL;
	_HUDText = NULL;
	_BGView = NULL;
}

void MulitViewer::genMainView()
{
	if (!_screens)
	{
		osg::notify(osg::FATAL) << "Error Creating Screens!" << std::endl;
		return;
	}
	
	osg::GraphicsContext::WindowingSystemInterface *wsi;
	wsi = osg::GraphicsContext::getWindowingSystemInterface();

	unsigned numScreensTxt = _screens->_scrs->getNumElements();
	osg::ref_ptr<osg::UIntArray> screenNumber = new osg::UIntArray(_screens->_scrs->begin(), _screens->_scrs->end());
	osg::UIntArray::iterator uni = std::unique(screenNumber->begin(), screenNumber->end());
	numScreensTxt -= (screenNumber->end() - uni);
	unsigned numScreensSys = wsi->getNumScreens();

	if (!numScreensTxt || numScreensTxt > numScreensSys)
	{
		osg::notify(osg::FATAL) << "Num of screens are wrong!" << std::endl;
		return;
	}

	if (!_mainView)
	{
		_mainView = createPowerWall();
	}

	this->addView(_mainView);

	return;
}

osgViewer::View * MulitViewer::createPowerWall()
{
	const double aspect = _screens->_aspect;
	double fovy = (_screens->_realworld->empty()) ? _screens->_fovy : _screens->_realworld->front();

	osg::ref_ptr<osgViewer::View> view = new osgViewer::View;
	if (_screens->_zNear && _screens->_zFar)
	{
		view->getCamera()->setComputeNearFarMode(osgUtil::CullVisitor::DO_NOT_COMPUTE_NEAR_FAR);
		view->getCamera()->setProjectionMatrixAsPerspective(fovy, aspect, _screens->_zNear, _screens->_zFar);
	}
	else
	{
		view->getCamera()->setProjectionMatrixAsPerspective(fovy, aspect, .1f, 1000.0f);
	}
	view->getCamera()->setClearColor(osg::Vec4(135.0f / 255.0f, 206.f / 255.f, 250.f / 255.f, 1.0f));

	osg::GraphicsContext::WindowingSystemInterface *wsi;
	wsi = osg::GraphicsContext::getWindowingSystemInterface();
	osg::GraphicsContext::ScreenSettings ss;
	if (_screens->_realworld->empty())
	{
		//multiple screens with one camera for each
		osg::ref_ptr<osg::UIntArray> screenNumber = new osg::UIntArray(_screens->_scrs->begin(), _screens->_scrs->end());
		const unsigned numColumns = _screens->_scrs->getNumElements(), numRows(1);
		for (unsigned i = 0; i < numColumns; i++)
		{
			unsigned si = *(_screens->_scrs->begin() + i);
			unsigned si_count = std::count(_screens->_scrs->begin(), _screens->_scrs->end(), si);
			osg::UIntArray::iterator ffo = std::find(screenNumber->begin(), screenNumber->end(),si);
			if (ffo != screenNumber->end())	screenNumber->erase(ffo);
			unsigned sn_count = std::count(screenNumber->begin(), screenNumber->end(), si);
			wsi->getScreenSettings(si, ss);
			ss.height = ss.height / numRows;
			ss.width = ss.width / si_count;

			unsigned index = si_count - sn_count;

			osg::ref_ptr<osg::Camera> camera = createSlaveCamerainMainView(si, ss, ss.width * (--index));

			osg::Matrix proOffset = osg::Matrix::scale(numColumns, numRows, 1.0f)
				* osg::Matrix::translate(int(numColumns - 2 * i - 1), 0.0f, 0.0f);
			
			view->addSlave(camera, proOffset, osg::Matrix(), true);
		}
	}
	else
	{
		//multiple cameras with one screen
		int numColumns(3), numRows(1);
		unsigned si = _screens->_scrs->front();
		wsi->getScreenSettings(si, ss);
		int tileWidth = ss.width / numColumns;
		int tileHeight = ss.height / numRows;

		osg::Vec3d center, eye, updir;
		view->getCamera()->getViewMatrixAsLookAt(eye, center, updir);

		for (int i = 0; i < numColumns; i++)
		{
			wsi->getScreenSettings(si, ss);
			ss.height = tileHeight;
			ss.width = tileWidth;
			osg::ref_ptr<osg::Camera> camera = createSlaveCamerainMainView(si, ss, tileWidth*i);

			const double rotation = _screens->_realworld->back() * (i - 1);

			osg::Matrix viewOffset = osg::Matrix::rotate(rotation * TO_RADDIAN, UP_DIR);

			_slaveCamerasinMainView.push_back(camera.get());
			view->addSlave(camera.release(), osg::Matrix(), viewOffset, true);
		}
	}

	return view.release();
}

osg::Camera * MulitViewer::createSlaveCamerainMainView(const unsigned screenNum, const osg::GraphicsContext::ScreenSettings &ss, const int startX /* = 0 */, const int startY /* = 0 */)
{
	osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
	traits->screenNum = screenNum;
	traits->x = startX;
	traits->y = startY;
	traits->width = ss.width;
	traits->height = ss.height;
	traits->windowDecoration = false;
	traits->doubleBuffer = true;
	traits->sharedContext = 0;
	traits->vsync = true;
	traits->samples = _screens->_fxaa;

	osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());

	osg::ref_ptr<osg::Camera> camera = new osg::Camera;
	camera->setGraphicsContext(gc.get());
	camera->setViewport(new osg::Viewport(0,0,ss.width,ss.height));

	GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
	camera->setDrawBuffer(buffer);
	camera->setReadBuffer(buffer);

	return camera.release();
}

osg::Camera * MulitViewer::createHUDCamerainWindow(osg::GraphicsContext *windows)
{
	// create a camera to set up the projection and model view matrices, and the subgraph to draw in the HUD
	osg::ref_ptr<osg::Camera> camera = new osg::Camera;

	camera->setGraphicsContext(windows);

	// set the view matrix    
	camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
	camera->setViewMatrix(osg::Matrix::identity());
	camera->setProjectionMatrix(osg::Matrix::ortho2D(0, windows->getTraits()->width, 0, windows->getTraits()->height));
	camera->setViewport(0, 0, windows->getTraits()->width, windows->getTraits()->height);

	//have to be set like this
	camera->setClearMask(0);

	// draw subgraph after main camera view.
	camera->setRenderOrder(osg::Camera::POST_RENDER);

	// we don't want the camera to grab event focus from the viewers main camera(s).
	camera->setAllowEventFocus(false);

	camera->setCullingActive(false);

	// turn lighting off for the text and disable depth test to ensure it's always ontop.
	osg::StateSet* stateset = camera->getOrCreateStateSet();
	stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

	return camera.release();
}

void MulitViewer::createHUDView()
{
	if (!_mainView && _HUDView)
	{
		return;
	}

	_HUDView = new osgViewer::View;

	osgViewer::Viewer::Windows windows;
	this->getWindows(windows);
	osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
	ss->setAttributeAndModes(new osg::Depth(osg::Depth::ALWAYS, 1.0f, 1.0f));
	for (unsigned i = 0; i < windows.size(); i++)
	{
		osg::ref_ptr<osg::Camera> camera = createHUDCamerainWindow(windows[i]);
		camera->setRenderOrder(osg::Camera::POST_RENDER, RenderOrder::HUDDISPLAY);
		camera->setStateSet(ss);

		_HUDView->addSlave(camera.release(), false);
	}

	this->addView(_HUDView);
}

void MulitViewer::createBackgroundView()
{
	if (!_mainView && _BGView)
	{
		return;
	}
	if (!_screens)
	{
		return;
	}
	if (!_screens->_imgBg)
	{
		return;
	}

	osg::Image *image = _screens->_imgBg;

	osg::ref_ptr<osg::Texture2D> background = new osg::Texture2D;
	background->setImage(image);
	background->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
	background->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
	osg::ref_ptr<osg::Drawable> quad = osg::createTexturedQuadGeometry(osg::Vec3d(0.0f, 0.0f, 0.0f),
		osg::Vec3d(1.0f, 0.0f, 0.0f), osg::Vec3d(0.0f, 1.0f, 0.0f));
	quad->getOrCreateStateSet()->setTextureAttributeAndModes(0, background);
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode->addDrawable(quad.release());

	_BGView = new osgViewer::View;
	_BGView->getCamera()->addChild(geode.get());
	osgViewer::Viewer::Windows windows;
	this->getWindows(windows);
	double x_start(0.0f);
	double x_step = 1.0f / double(windows.size());
	double x_end = x_start + x_step;
	for (unsigned i = 0; i < windows.size(); i++)
	{
		osg::Camera *camera = createHUDCamerainWindow(windows[i]);

		//set the projection matrix
		camera->setProjectionMatrixAsOrtho2D(x_start, x_end, 0.0f, 1.0f);
		x_start = x_end;
		x_end += x_step;

		camera->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth(osg::Depth::LEQUAL, 1.0f, 1.0f));
		camera->setRenderOrder(osg::Camera::POST_RENDER, RenderOrder::BACKGROUND);

		_BGView->addSlave(camera, true);
	}

	this->addView(_BGView);
}

bool MulitViewer::hmd_Initialise()
{
	if (_hmdViewer)
	{
		return true;
	}

	if (!ovr_Initialize())
	{
		osg::notify(osg::FATAL) << "Cannot Initialize Oculus Rift" << std::endl;
		system("pause");
		return false;
	}

	_hmd = ovrHmd_Create(0);
	if (!_hmd)
	{
		_hmd = ovrHmd_CreateDebug(ovrHmd_DK2);
	}

	uint32_t l_SupportedSensorCaps = ovrTrackingCap_Orientation | ovrTrackingCap_Position | ovrTrackingCap_MagYawCorrection;
	uint32_t l_RequiredTrackingCaps = 0;
	ovrBool l_TrackingResult = ovrHmd_ConfigureTracking(_hmd, l_SupportedSensorCaps, l_RequiredTrackingCaps);
	if (!l_TrackingResult)
	{
		osg::notify(osg::WARN) << "Cannot Config Oculus Rift Tracking" << std::endl;
		system("pause");
	}

	_windowsR = _hmd->Resolution;
	_windowsR.w /= 2;
	_windowsR.h /= 2;

	_hmdViewer = new osgViewer::Viewer;
	_hmdViewer->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
	_hmdViewer->setUpViewInWindow(0, 0, _windowsR.w, _windowsR.h, 0);

	_eyeTextureSize[ovrEye_Left] = ovrHmd_GetFovTextureSize(_hmd, ovrEye_Left, _hmd->DefaultEyeFov[ovrEye_Left], 1.0f);
	_eyeTextureSize[ovrEye_Right] = ovrHmd_GetFovTextureSize(_hmd, ovrEye_Right, _hmd->DefaultEyeFov[ovrEye_Right], 1.0f);

	_targetSize.w = _eyeTextureSize[ovrEye_Left].w + _eyeTextureSize[ovrEye_Right].w;
	_targetSize.h = max(_eyeTextureSize[ovrEye_Left].h, _eyeTextureSize[ovrEye_Right].h);

	_masterTex = new osg::Texture2D;
	_masterTex->setTextureSize(_targetSize.w, _targetSize.h);
	_masterTex->setInternalFormat(GL_RGBA);
	_masterTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
	_masterTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

	_hmdViewer->getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
	_hmdViewer->getCamera()->setViewport(0, 0, _masterTex->getTextureWidth(), _masterTex->getTextureHeight());
	_hmdViewer->getCamera()->setRenderOrder(osg::Camera::PRE_RENDER);
	_hmdViewer->getCamera()->attach(osg::Camera::COLOR_BUFFER, _masterTex);

	osg::Camera *leftCam = createSlaveCamerainHMD(_hmdViewer, _masterTex);
	osg::Camera *rightCam = createSlaveCamerainHMD(_hmdViewer, _masterTex);

	_hmdViewer->addSlave(leftCam, osg::Matrix(), osg::Matrix(), true);
	_hmdViewer->addSlave(rightCam, osg::Matrix(), osg::Matrix(), true);

	_hmdViewer->getCamera()->setDrawBuffer(GL_NONE);
	leftCam->setDrawBuffer(GL_NONE);
	rightCam->setDrawBuffer(GL_NONE);
	_hmdViewer->getCamera()->setReadBuffer(GL_NONE);
	leftCam->setReadBuffer(GL_NONE);
	rightCam->setReadBuffer(GL_NONE);
	_hmdViewer->getCamera()->getGraphicsContext()->setSwapCallback(new swapcallback);

	return true;
}

bool MulitViewer::setHMDSceneData(osg::Node *node)
{
	if (!_hmdViewer)
	{
		osg::notify(osg::FATAL) << "No HMD Viewer Found!" << std::endl;
		system("pause");
		return false;
	}

	int g_DistortionCaps = 0
		| ovrDistortionCap_Vignette
		| ovrDistortionCap_Chromatic
		| ovrDistortionCap_Overdrive
		| ovrDistortionCap_TimeWarp // Turning this on gives ghosting???
		;

	_hmdViewer->setSceneData(node);
	_hmdViewer->realize();

	osgViewer::ViewerBase::Windows windows;
	_hmdViewer->getWindows(windows);
	if (windows.size() > 1)
	{
		osg::notify(osg::FATAL) << "Currently Does NOT support multiple windows!" << std::endl;
		return false;
	}
	osgViewer::GraphicsHandleWin32 *osgwin = dynamic_cast<osgViewer::GraphicsHandleWin32*>(windows.front());
	HWND hWnd = osgwin->getHWND();
	HDC hDc = GetDC(hWnd);

	ovrHmd_AttachToWindow(_hmd, hWnd, NULL, NULL);

	_hmdViewer->frame();

	_hmdViewer->getCamera()->getGraphicsContext()->makeCurrent();

	std::cout << glGetString(GL_VERSION) << std::endl;

	_glCfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	_glCfg.OGL.Header.BackBufferSize = _windowsR;
	_glCfg.OGL.Header.Multisample = 0;
	_glCfg.OGL.Window = hWnd;
	_glCfg.OGL.DC = GetDC(hWnd);

	ovrBool configResult = ovrHmd_ConfigureRendering(_hmd, &_glCfg.Config, g_DistortionCaps, _hmd->MaxEyeFov, _eyeRenderDesc);
	if (!configResult)
	{
		osg::notify(osg::FATAL) << "Oculus Rift Config Render Failed!" << std::endl;
		system("pause");
		return false;
	}

	unsigned contextID = _hmdViewer->getCamera()->getGraphicsContext()->getState()->getContextID();
	GLuint master_TexID = 20;
	if (_masterTex->getTextureObject(contextID))
	{
		master_TexID = _masterTex->getTextureObject(contextID)->id();
	}

	ovrGLTexture leftTex;
	leftTex.OGL.Header.API = ovrRenderAPI_OpenGL;
	leftTex.OGL.Header.TextureSize = _targetSize;
	leftTex.OGL.Header.RenderViewport.Pos.x = 0;
	leftTex.OGL.Header.RenderViewport.Pos.y = 0;
	leftTex.OGL.Header.RenderViewport.Size = _eyeTextureSize[ovrEye_Left];
	leftTex.OGL.TexId = master_TexID;

	ovrGLTexture rightTex;
	rightTex.OGL.Header.API = ovrRenderAPI_OpenGL;
	rightTex.OGL.Header.TextureSize = _targetSize;
	rightTex.OGL.Header.RenderViewport.Pos.x = _eyeTextureSize[ovrEye_Left].w;
	rightTex.OGL.Header.RenderViewport.Pos.y = 0;
	rightTex.OGL.Header.RenderViewport.Size = _eyeTextureSize[ovrEye_Right];
	rightTex.OGL.TexId = master_TexID;

	_eyeTextures[ovrEye_Left] = leftTex.Texture;
	_eyeTextures[ovrEye_Right] = rightTex.Texture;
}

void MulitViewer::runHMD()
{
	if (!_hmdViewer)
	{
		osg::notify(osg::FATAL) << "Cannot find HMD Viewer!" << std::endl;
		return;
	}

	_projectionMatrici[ovrEye_Left] = ovrMatrix4f_Projection(_eyeRenderDesc[ovrEye_Left].Fov, _screens->_zNear, _screens->_zFar, true);
	_projectionMatrici[ovrEye_Right] = ovrMatrix4f_Projection(_eyeRenderDesc[ovrEye_Right].Fov, _screens->_zNear, _screens->_zFar, true);
	osg::Matrix l_proj_M, r_proj_M, l_view_M, r_view_M;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			l_proj_M(i, j) = _projectionMatrici[ovrEye_Left].Transposed().M[i][j];
			r_proj_M(i, j) = _projectionMatrici[ovrEye_Right].Transposed().M[i][j];
		}
	}

	_eyeOffsets[ovrEye_Left] = _eyeRenderDesc[ovrEye_Left].HmdToEyeViewOffset;
	_eyeOffsets[ovrEye_Right] = _eyeRenderDesc[ovrEye_Right].HmdToEyeViewOffset;
	l_view_M = osg::Matrix::translate(_eyeOffsets[ovrEye_Left].x, _eyeOffsets[ovrEye_Left].y, _eyeOffsets[ovrEye_Left].z);
	r_view_M = osg::Matrix::translate(_eyeOffsets[ovrEye_Right].x, _eyeOffsets[ovrEye_Right].y, _eyeOffsets[ovrEye_Right].z);

	osg::Camera *leftcam = _hmdViewer->getSlave(ovrEye_Left)._camera;
	osg::Camera *rightcam = _hmdViewer->getSlave(ovrEye_Right)._camera;

	leftcam->setViewport(_eyeTextures[ovrEye_Left].Header.RenderViewport.Pos.x,
		_eyeTextures[ovrEye_Left].Header.RenderViewport.Pos.y,
		_eyeTextures[ovrEye_Left].Header.RenderViewport.Size.w,
		_eyeTextures[ovrEye_Left].Header.RenderViewport.Size.h);
	rightcam->setViewport(_eyeTextures[ovrEye_Left].Header.RenderViewport.Size.w,
		_eyeTextures[ovrEye_Right].Header.RenderViewport.Pos.y,
		_eyeTextures[ovrEye_Right].Header.RenderViewport.Size.w,
		_eyeTextures[ovrEye_Right].Header.RenderViewport.Size.h);

	leftcam->setProjectionMatrix(l_proj_M);
	rightcam->setProjectionMatrix(r_proj_M);

	ovrHmd_RecenterPose(_hmd);
	ovrFrameTiming l_HmdFrameTiming;

	typedef BOOL(GL_APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);
	PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");

	while (!_hmdViewer->done())
	{
		const unsigned &frameIndex = _hmdViewer->getFrameStamp()->getFrameNumber();
		l_HmdFrameTiming = ovrHmd_BeginFrame(_hmd, frameIndex);
		ovrHmd_GetEyePoses(_hmd, frameIndex, _eyeOffsets, _eyePoses, NULL);

		OVR::Quatd leftOrientation = OVR::Quatd(_eyePoses[ovrEye_Left].Orientation);
		OVR::Matrix4d leftMVMatrix = OVR::Matrix4d(leftOrientation.Inverted());
		OVR::Quatd rightOrientation = OVR::Quatd(_eyePoses[ovrEye_Right].Orientation);
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

		leftcam->setViewMatrix(_hmdViewer->getCamera()->getViewMatrix() * leftMatrix * l_view_M);
		rightcam->setViewMatrix(_hmdViewer->getCamera()->getViewMatrix() * rightMatrix * r_view_M);

		if (wglSwapIntervalEXT)
		{
			wglSwapIntervalEXT(0);
		}

		_hmdViewer->frame();
		_hmdViewer->getCamera()->getGraphicsContext()->makeCurrent();
		ovrHmd_EndFrame(_hmd, _eyePoses, _eyeTextures);
	}
}

void MulitViewer::shutdownHMD()
{
	ovrHmd_Destroy(_hmd);
	ovr_Shutdown();
}

osg::Camera * MulitViewer::createRTTCamera(osg::Camera::BufferComponent buffer, osg::Texture *tex)
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

osg::Camera * MulitViewer::createSlaveCamerainHMD(osgViewer::Viewer *viewer, osg::ref_ptr<osg::Texture2D> tex)
{
	osg::GraphicsContext *context = viewer->getCamera()->getGraphicsContext();

	osg::ref_ptr<osg::Camera> camera = createRTTCamera(osg::Camera::COLOR_BUFFER, tex);
	camera->setGraphicsContext(context);
	camera->setView(viewer);

	return camera.release();
}