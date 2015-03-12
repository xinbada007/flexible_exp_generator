#include "stdafx.h"
#include "mulitViewer.h"
#include "cameraEvent.h"

#include <osg/Notify>
#include <osgViewer/Viewer>
#include <osg/Depth>
#include <osgDB/ReadFile>
#include <osg/Multisample>
#include <osgViewer/api/Win32/GraphicsHandleWin32>

#include <OVR.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

#include <assert.h>

MulitViewer::MulitViewer(osg::ref_ptr<ReadConfig> refRC):
_screens(refRC->getScreens()), _normalView(NULL), _HUDView(NULL), _HUDText(NULL), _BGView(NULL), _hmdView(NULL), _masterTex(NULL), _hFOV(0.0f)
{
}

MulitViewer::~MulitViewer()
{
	std::cout << "Deconstruct MulitViewer" << std::endl;

	if (_hmdView)
	{
		shutdownHMD();
	}

	_screens = NULL;
	_normalView = NULL;
	std::vector<osg::Camera*>::iterator cam_i = _slaveCamerasinNormalView.begin();
	while (cam_i != _slaveCamerasinNormalView.end())
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

	if (_hmdView || _normalView)
	{
		return;
	}

	if (_screens->_HMD)
	{
		hmd_Initialise();
		assert(_hmdView);
		this->addView(_hmdView);
		this->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
		frameRate::instance()->setDesignedfRate(_screens->_HMD_RefreshRate);
		this->setRunMaxFrameRate(_screens->_HMD_RefreshRate);
		assert(_hmd);
		_hFOV = max(_hmd->MaxEyeFov->LeftTan, _hmd->MaxEyeFov->RightTan);
		_hFOV *= 2.0f;
		_hFOV /= TO_RADDIAN;
	}
	else
	{
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

		osg::GraphicsContext::ScreenSettings ss;
		wsi->getScreenSettings(screenNumber->front(), ss);
		double refreshRate(ss.refreshRate);
		for (osg::UIntArray::iterator i = screenNumber->begin(); i != uni; i++)
		{
			wsi->getScreenSettings(*i, ss);
			if (!isEqual(refreshRate, ss.refreshRate, 1.0f))
			{
				osg::notify(osg::WARN) << "Refresh Rate is inconsistent please adjust otherwise program WILL be affected" << std::endl;
				system("pause");
			}
		}

		_normalView = createPowerWall();
		this->addView(_normalView);
		this->setThreadingModel(osgViewer::ViewerBase::ThreadPerCamera);
		frameRate::instance()->setDesignedfRate(refreshRate);
		this->setRunMaxFrameRate(refreshRate);
		_hFOV = _screens->_hFov;
	}

	
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
	view->getCamera()->setClearColor(_screens->_bgColor);

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

			osg::ref_ptr<osg::Camera> camera = createSlaveCamerainNormalView(si, ss, ss.width * (--index));

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
			osg::ref_ptr<osg::Camera> camera = createSlaveCamerainNormalView(si, ss, tileWidth*i);

			const double rotation = _screens->_realworld->back() * (i - 1);

			osg::Matrix viewOffset = osg::Matrix::rotate(rotation * TO_RADDIAN, UP_DIR);

			_slaveCamerasinNormalView.push_back(camera.get());
			view->addSlave(camera.release(), osg::Matrix(), viewOffset, true);
		}
	}

	return view.release();
}

osg::Camera * MulitViewer::createSlaveCamerainNormalView(const unsigned screenNum, const osg::GraphicsContext::ScreenSettings &ss, const int startX /* = 0 */, const int startY /* = 0 */)
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
	if (_normalView)
	{
		assert(!_hmdView);
		if (_HUDView)
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

		return;
	}

	else if (_hmdView)
	{
		assert(!_normalView);
		if (_HUDView)
		{
			return;
		}

// 		_HUDView = new osgViewer::View;
// /*		_HUDView->getCamera()->setGraphicsContext(_hmdView->getCamera()->getGraphicsContext());*/
// 
// 		osgViewer::ViewerBase::Windows windows;
// 		this->getWindows(windows);
// 		osg::ref_ptr<osg::Camera> camera = createHUDCamerainWindow(windows.front());
// 		osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
// 		ss->setAttributeAndModes(new osg::Depth(osg::Depth::ALWAYS, 1.0f, 1.0f));
// 		camera->setStateSet(ss);
// 		_HUDView->setCamera(camera.release());
// 
// 		_HUDView->getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
// 		_HUDView->getCamera()->setRenderOrder(osg::Camera::PRE_RENDER);
// 		_HUDView->getCamera()->attach(osg::Camera::COLOR_BUFFER, _masterTex);
// 
// 		osg::Camera *leftCam = new osg::Camera;
// 		{
// 			leftCam->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
// 			leftCam->setRenderOrder(osg::Camera::PRE_RENDER, RenderOrder::HUDDISPLAY);
// 			leftCam->setViewport(0, 0, _masterTex->getTextureWidth(), _masterTex->getTextureHeight());
// 			leftCam->attach(osg::Camera::COLOR_BUFFER, _masterTex);
// 			leftCam->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
// 			osg::GraphicsContext *context = _HUDView->getCamera()->getGraphicsContext();
// 			leftCam->setGraphicsContext(context);
// 			leftCam->setView(_HUDView);
// 		}
// 
// 		osg::Camera *rightCam = new osg::Camera;
// 		{
// 			rightCam->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
// 			rightCam->setRenderOrder(osg::Camera::PRE_RENDER, RenderOrder::HUDDISPLAY);
// 			rightCam->setViewport(0, 0, _masterTex->getTextureWidth(), _masterTex->getTextureHeight());
// 			rightCam->attach(osg::Camera::COLOR_BUFFER, _masterTex);
// 			rightCam->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
// 			osg::GraphicsContext *context = _HUDView->getCamera()->getGraphicsContext();
// 			rightCam->setGraphicsContext(context);
// 			rightCam->setView(_HUDView);
// 		}
// 
//  		_HUDView->addSlave(leftCam, osg::Matrix(), osg::Matrix(), true);
//  		_HUDView->addSlave(rightCam, osg::Matrix(), osg::Matrix(), true);
// 
// 		_HUDView->getCamera()->setDrawBuffer(GL_NONE);
// 		leftCam->setDrawBuffer(GL_NONE);
// 		rightCam->setDrawBuffer(GL_NONE);
// 		_HUDView->getCamera()->setReadBuffer(GL_NONE);
// 		leftCam->setReadBuffer(GL_NONE);
// 		rightCam->setReadBuffer(GL_NONE);
// 		_HUDView->getCamera()->getGraphicsContext()->setSwapCallback(new swapcallback);
// 
// 		float    orthoDistance = 0.8f; // 2D is 0.8 meter from camera
// 		OVR::Vector2f orthoScale0 = OVR::Vector2f(1.0f) / OVR::Vector2f(_eyeRenderDesc[ovrEye_Left].PixelsPerTanAngleAtCenter);
// 		OVR::Vector2f orthoScale1 = OVR::Vector2f(1.0f) / OVR::Vector2f(_eyeRenderDesc[ovrEye_Right].PixelsPerTanAngleAtCenter);
// 		OVR::Matrix4f _projection2D[ovrEye_Count];
// 		_projection2D[ovrEye_Left] = ovrMatrix4f_OrthoSubProjection(_projectionMatrici[ovrEye_Left], orthoScale0, orthoDistance, _eyeRenderDesc[ovrEye_Left].HmdToEyeViewOffset.x);
// 		_projection2D[ovrEye_Right] = ovrMatrix4f_OrthoSubProjection(_projectionMatrici[ovrEye_Right], orthoScale1, orthoDistance, _eyeRenderDesc[ovrEye_Right].HmdToEyeViewOffset.x);
// 		osg::Matrix l_proj_M, r_proj_M, l_view_M, r_view_M;
// 		for (int i = 0; i < 4; i++)
// 		{
// 			for (int j = 0; j < 4; j++)
// 			{
// 				l_proj_M(i, j) = _projection2D[ovrEye_Left].Transposed().M[i][j];
// 				r_proj_M(i, j) = _projection2D[ovrEye_Right].Transposed().M[i][j];
// 			}
// 		}
// 
// 		l_view_M = osg::Matrix::translate(_eyeOffsets[ovrEye_Left].x, _eyeOffsets[ovrEye_Left].y, _eyeOffsets[ovrEye_Left].z);
// 		r_view_M = osg::Matrix::translate(_eyeOffsets[ovrEye_Right].x, _eyeOffsets[ovrEye_Right].y, _eyeOffsets[ovrEye_Right].z);
// 
// 		leftCam->setViewport(_eyeTextures[ovrEye_Left].Header.RenderViewport.Pos.x,
// 			_eyeTextures[ovrEye_Left].Header.RenderViewport.Pos.y,
// 			_eyeTextures[ovrEye_Left].Header.RenderViewport.Size.w,
// 			_eyeTextures[ovrEye_Left].Header.RenderViewport.Size.h);
// 		rightCam->setViewport(_eyeTextures[ovrEye_Left].Header.RenderViewport.Size.w,
// 			_eyeTextures[ovrEye_Right].Header.RenderViewport.Pos.y,
// 			_eyeTextures[ovrEye_Right].Header.RenderViewport.Size.w,
// 			_eyeTextures[ovrEye_Right].Header.RenderViewport.Size.h);
// 
// 		leftCam->setProjectionMatrix(l_proj_M);
// 		rightCam->setProjectionMatrix(r_proj_M);
// 
// 		this->addView(_HUDView);

		return;
	}
}

void MulitViewer::createBackgroundView()
{
	if (_normalView)
	{
		assert(!_hmdView);
		assert(_screens);
		if (_BGView || !_screens->_imgBg)
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

		return;
	}

	else if (_hmdView)
	{
		assert(!_normalView);
		assert(_screens);
		if (_BGView)
		{
			return;
		}

		//Do something here
		//Do something here

		return;
	}
}

bool MulitViewer::hmd_Initialise()
{
	if (_hmdView)
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

	//Test
	unsigned hmdCaps = /*ovrHmdCap_NoMirrorToWindow |*/ ovrHmdCap_NoVSync;
	ovrHmd_SetEnabledCaps(_hmd, hmdCaps);
	//Test

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

	_hmdView = new osgViewer::View;
	_hmdView->setUpViewInWindow(0, 0, _windowsR.w, _windowsR.h, 0);

	_eyeTextureSize[ovrEye_Left] = ovrHmd_GetFovTextureSize(_hmd, ovrEye_Left, _hmd->MaxEyeFov[ovrEye_Left], 1.0f);
	_eyeTextureSize[ovrEye_Right] = ovrHmd_GetFovTextureSize(_hmd, ovrEye_Right, _hmd->MaxEyeFov[ovrEye_Right], 1.0f);

	_targetSize.w = _eyeTextureSize[ovrEye_Left].w + _eyeTextureSize[ovrEye_Right].w;
	_targetSize.h = max(_eyeTextureSize[ovrEye_Left].h, _eyeTextureSize[ovrEye_Right].h);

	_masterTex = new osg::Texture2D;
	_masterTex->setTextureSize(_targetSize.w, _targetSize.h);
	_masterTex->setInternalFormat(GL_RGBA);
	_masterTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
	_masterTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

	_hmdView->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	_hmdView->getCamera()->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
	_hmdView->getCamera()->setViewport(0, 0, _masterTex->getTextureWidth(), _masterTex->getTextureHeight());
	_hmdView->getCamera()->setRenderOrder(osg::Camera::PRE_RENDER);
	_hmdView->getCamera()->attach(osg::Camera::COLOR_BUFFER, _masterTex);

	osg::Camera *leftCam = createSlaveCamerainHMD(_hmdView, _masterTex);
	osg::Camera *rightCam = createSlaveCamerainHMD(_hmdView, _masterTex);

	_hmdView->addSlave(leftCam, osg::Matrix(), osg::Matrix(), true);
	_hmdView->addSlave(rightCam, osg::Matrix(), osg::Matrix(), true);

	_hmdView->getCamera()->setDrawBuffer(GL_NONE);
	leftCam->setDrawBuffer(GL_NONE);
	rightCam->setDrawBuffer(GL_NONE);
	_hmdView->getCamera()->setReadBuffer(GL_NONE);
	leftCam->setReadBuffer(GL_NONE);
	rightCam->setReadBuffer(GL_NONE);
	_hmdView->getCamera()->getGraphicsContext()->setSwapCallback(new swapcallback);

	//Test
// 	osg::ref_ptr<osg::Camera> backCam = new osg::Camera;
// 	{
// 		backCam->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
// 		backCam->setRenderOrder(osg::Camera::PRE_RENDER);
// 		backCam->setViewport(0, 0, _masterTex->getTextureWidth(), _masterTex->getTextureHeight());
// 		backCam->attach(osg::Camera::COLOR_BUFFER, _masterTex);
// 		backCam->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
// 
// 		backCam->setGraphicsContext(_hmdView->getCamera()->getGraphicsContext());
// 		backCam->setView(_hmdView);
// 
// 		_hmdView->addSlave(backCam, osg::Matrix(), osg::Matrix(), true);
// 		backCam->setDrawBuffer(GL_NONE);
// 		backCam->setReadBuffer(GL_NONE);
// 	}
	//Test

	return true;
}

bool MulitViewer::setHMDSceneData(osg::Node *node)
{
	if (!_hmdView)
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

	_hmdView->setSceneData(node);
	this->realize();

	osgViewer::ViewerBase::Windows windows;
	this->getWindows(windows);
	if (windows.size() > 1)
	{
		osg::notify(osg::FATAL) << "Currently Does NOT support multiple windows!" << std::endl;
		return false;
	}
	osgViewer::GraphicsHandleWin32 *osgwin = dynamic_cast<osgViewer::GraphicsHandleWin32*>(windows.front());
	HWND hWnd = osgwin->getHWND();
	HDC hDc = GetDC(hWnd);

	ovrBool attached = ovrHmd_AttachToWindow(_hmd, hWnd, NULL, NULL);
	if (!attached)
	{
		osg::notify(osg::FATAL) << "Attch to windows failed!" << std::endl;
		system("pause");
		return false;
	}

	this->frame();

	_hmdView->getCamera()->getGraphicsContext()->makeCurrent();

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

	unsigned contextID = _hmdView->getCamera()->getGraphicsContext()->getState()->getContextID();
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

	return true;
}

void MulitViewer::runHMD()
{
	if (!_hmdView)
	{
		osg::notify(osg::FATAL) << "Cannot find HMD Viewer!" << std::endl;
		return;
	}

	_projectionMatrici[ovrEye_Left] = ovrMatrix4f_Projection(_eyeRenderDesc[ovrEye_Left].Fov, _screens->_zNear, _screens->_zFar, true);
	_projectionMatrici[ovrEye_Right] = ovrMatrix4f_Projection(_eyeRenderDesc[ovrEye_Right].Fov, _screens->_zNear, _screens->_zFar, true);
	osg::Matrixd l_proj_M, r_proj_M, l_view_M, r_view_M;
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

	osg::Camera *leftcam = _hmdView->getSlave(ovrEye_Left)._camera;
	osg::Camera *rightcam = _hmdView->getSlave(ovrEye_Right)._camera;

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

// 	//Test
// 	osg::Camera *backCam = _hmdView->getSlave(ovrEye_Count)._camera;
// 	backCam->setViewport(_eyeTextures[ovrEye_Left].Header.RenderViewport.Pos.x,
// 		_eyeTextures[ovrEye_Left].Header.RenderViewport.Pos.y,
// 		_eyeTextures[ovrEye_Left].Header.RenderViewport.Size.w*0.2f,
// 		_eyeTextures[ovrEye_Left].Header.RenderViewport.Size.h*0.2f);
// 	OVR::Matrix4f proM = ovrMatrix4f_Projection(_eyeRenderDesc[ovrEye_Left].Fov, -_screens->_zNear, -_screens->_zFar, true);
// 	osg::Matrix projMosg;
// 	for (int i = 0; i < 4; i++)
// 	{
// 		for (int j = 0; j < 4; j++)
// 		{
// 			projMosg(i, j) = proM.Transposed().M[i][j];
// 		}
// 	}
// 	backCam->setProjectionMatrix(projMosg);
	//Test

	osg::Matrixd L, R;
	CameraEvent *camEvent = dynamic_cast<CameraEvent*>(_hmdView->getCameraManipulator());
	if (camEvent)
	{
		camEvent->addOffsetMatrixtoList(&L);
		camEvent->addCameratoList(leftcam);
		camEvent->addOffsetMatrixtoList(&R);
		camEvent->addCameratoList(rightcam);
	}

	typedef BOOL(GL_APIENTRY *PFNWGLSWAPINTERVALFARPROC)(int);
	PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT = 0;
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress("wglSwapIntervalEXT");
	assert(wglSwapIntervalEXT);

	ovrHmd_RecenterPose(_hmd);
	ovrHmd_DismissHSWDisplay(_hmd);
	ovrFrameTiming l_HmdFrameTiming;

	while (!this->done())
	{
		const unsigned &frameIndex = _hmdView->getFrameStamp()->getFrameNumber();
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

		L = leftMatrix * l_view_M;
		R = rightMatrix* r_view_M;

// 		//Test
//		backCam->setViewMatrix(_hmdView->getCamera()->getViewMatrix());
// 		proM = ovrMatrix4f_Projection(_eyeRenderDesc[ovrEye_Left].Fov, _screens->_zNear, _screens->_zFar, true);
// 		for (int i = 0; i < 4; i++)
// 		{
// 			for (int j = 0; j < 4; j++)
// 			{
// 				projMosg(i, j) = proM.Transposed().M[i][j];
// 			}
// 		}
// 		backCam->setProjectionMatrix(projMosg);
// 		//Test

//  	wglSwapIntervalEXT(0);

		this->frame();
		_hmdView->getCamera()->getGraphicsContext()->makeCurrent();
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

osg::Camera * MulitViewer::createSlaveCamerainHMD(osgViewer::View *view, osg::ref_ptr<osg::Texture2D> tex)
{
	osg::GraphicsContext *context = view->getCamera()->getGraphicsContext();

	osg::ref_ptr<osg::Camera> camera = createRTTCamera(osg::Camera::COLOR_BUFFER, tex);
	camera->setGraphicsContext(context);
	camera->setView(view);

	return camera.release();
}

bool MulitViewer::setMainViewSceneData(osg::Node *node)
{
	if (_normalView)
	{
		assert(!_hmdView);
		
		_normalView->setSceneData(node);

		return true;
	}

	else if (_hmdView)
	{
		assert(!_normalView);

		return setHMDSceneData(node);
	}

	return false;
}

int MulitViewer::go()
{
	if (_normalView)
	{
		assert(!_hmdView);
		return run();
// 		while (!this->done())
// 		{
// 			this->frame();
// 		}
// 		return 0;
	}
	else if (_hmdView)
	{
		assert(!_normalView);
		runHMD();
		return 0;
	}

	return -1;
}