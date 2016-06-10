#include "stdafx.h"
#include "mulitViewer.h"
#include "cameraEvent.h"

#include <osg/Notify>
#include <osgViewer/Viewer>
#include <osg/Depth>
#include <osgDB/ReadFile>
#include <osg/Multisample>
#include <osgViewer/api/Win32/GraphicsHandleWin32>

#include <assert.h>

MulitViewer::MulitViewer(osg::ref_ptr<ReadConfig> refRC):
_screens(refRC->getScreens()), _normalView(NULL), _HUDView(NULL), 
_HUDText(NULL), _BGView(NULL), _hFOV(0.0f), _HMDViewer(NULL), _OculusViewer(NULL)
{
}

MulitViewer::~MulitViewer()
{
	std::cout << "Deconstruct MulitViewer" << std::endl;

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

void MulitViewer::genMainView(osg::ref_ptr<osg::Node> node)
{
	if (!_screens)
	{
		osg::notify(osg::FATAL) << "Error Creating Screens!" << std::endl;
		return;
	}

	if (_normalView || _HMDViewer || _OculusViewer)
	{
		return;
	}

	if (_screens->_HMD)
	{
		createHMD(node);
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
		if (_screens->_refreshRate)
		{
			frameRate::instance()->setDesignedfRate(_screens->_refreshRate);
		}
		else
		{
			frameRate::instance()->setDesignedfRate(refreshRate);
		}
		
		this->setRunMaxFrameRate(frameRate::instance()->getDesignfRate());
		_hFOV = _screens->_hFov;

		_normalView->setSceneData(node);
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
			if (_screens->_resWidth != 0 && _screens->_resHeight != 0)
			{
				ss.height = _screens->_resHeight;
				ss.width = _screens->_resWidth;
			}
			ss.height = ss.height / numRows;
			ss.width = ss.width / si_count;

			unsigned index = si_count - sn_count;

			osg::ref_ptr<osg::Camera> camera = createSlaveCamerainNormalView(si, ss, ss.width * (--index));

			osg::Matrix proOffset = osg::Matrix::scale(numColumns, numRows, 1.0f)
				* osg::Matrix::translate(int(numColumns - 2 * i - 1), 0.0f, 0.0f);
			
			_slaveCamerasinNormalView.push_back(camera.get());
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
	traits->supportsResize = false;
	traits->doubleBuffer = true;
	traits->sharedContext = 0;
	traits->vsync = true;
	traits->samples = _screens->_fxaa;
	traits->useCursor = false;

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
	camera->setClearMask(GL_DEPTH_BUFFER_BIT);

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
	if (_normalView && !_HMDViewer)
	{
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
}

void MulitViewer::createBackgroundView()
{
	if (_normalView && !_HMDViewer)
	{
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
}

bool MulitViewer::setMainViewSceneData(osg::Node *node)
{
	if (_normalView)
	{
		_normalView->setSceneData(node);

		return true;
	}

	return false;
}

int MulitViewer::go()
{
	if (_normalView && !_HMDViewer)
	{
		return run();
	}

	if (_HMDViewer && !_normalView)
	{
		return _HMDViewer->run();
	}

	return -1;
}

void MulitViewer::createHMD(osg::ref_ptr<osg::Node> node)
{
	float nearClip = (_screens->_zNear) ? _screens->_zNear : 0.01;
	float farClip = (_screens->_zFar) ? _screens->_zFar : 10000.0f;
	float pixelsPerDisplayPixel = 1.0;
	float worldUnitsPerMetre = 1.0f;
	int samples = 4;
	osg::ref_ptr<OculusDevice> oculusDevice = new OculusDevice(nearClip, farClip, pixelsPerDisplayPixel, worldUnitsPerMetre, samples);

	// Exit if we do not have a valid HMD present
	if (!oculusDevice->hmdPresent())
	{
		osg::notify(osg::FATAL) << "Error: No valid HMD present!" << std::endl;
		return;
	}

	// Get the suggested context traits
	osg::ref_ptr<osg::GraphicsContext::Traits> traits = oculusDevice->graphicsContextTraits();
	traits->windowName = "GO_HMD";

	// Create a graphic context based on our desired traits
	osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits);

	if (!gc)
	{
		osg::notify(osg::NOTICE) << "Error, GraphicsWindow has not been created successfully" << std::endl;
		return;
	}

	if (gc.valid())
	{
		gc->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
		gc->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	_HMDViewer = new osgViewer::Viewer;
	_HMDViewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
	_HMDViewer->getCamera()->setGraphicsContext(gc);
	_HMDViewer->getCamera()->setViewport(0, 0, traits->width, traits->height);

	// Disable automatic computation of near and far plane
	_HMDViewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	_HMDViewer->getCamera()->setClearColor(_screens->_bgColor);

	// Things to do when viewer is realized
	osg::ref_ptr<OculusRealizeOperation> oculusRealizeOperation = new OculusRealizeOperation(oculusDevice);
	_HMDViewer->setRealizeOperation(oculusRealizeOperation.get());

	_OculusViewer = new OculusViewer(_HMDViewer.get(), oculusDevice, oculusRealizeOperation);
	_OculusViewer->addChild(node);
	_HMDViewer->setSceneData(_OculusViewer);

	_HMDViewer->addEventHandler(new OculusEventHandler(oculusDevice));
}