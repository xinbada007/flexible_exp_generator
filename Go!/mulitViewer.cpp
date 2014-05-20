#include "stdafx.h"
#include "mulitViewer.h"
#include "math.h"

#include <osg/Notify>

MulitViewer::MulitViewer()
{
	_screens = NULL;
}


MulitViewer::~MulitViewer()
{
}

void MulitViewer::genMainViewer(osg::ref_ptr<ReadConfig> refRC)
{
	_screens = refRC->getScreens();
	if (!_screens)
	{
		osg::notify(osg::FATAL) << "Error Creating Screens!" << std::endl;
		return;
	}
	
	osg::GraphicsContext::WindowingSystemInterface *wsi;
	wsi = osg::GraphicsContext::getWindowingSystemInterface();

	unsigned numScreensTxt = _screens->_scrs->getNumElements();
	unsigned numScreensSys = wsi->getNumScreens();

	if (!numScreensTxt || numScreensTxt > numScreensSys)
	{
		osg::notify(osg::FATAL) << "Num of screens are wrong!" << std::endl;
		return;
	}

	this->addView(createPowerWall());

	return;
}

osgViewer::View * MulitViewer::createPowerWall()
{
	const double aspect = _screens->_aspect;
	double fovy = (_screens->_realworld->empty()) ? _screens->_fovy : _screens->_realworld->front();

	osg::ref_ptr<osgViewer::View> view = new osgViewer::View;	
	view->getCamera()->setProjectionMatrixAsPerspective(fovy, aspect, _screens->_horDistance, 1000.0f);
	view->getCamera()->setClearColor(osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f));

	osg::GraphicsContext::WindowingSystemInterface *wsi;
	wsi = osg::GraphicsContext::getWindowingSystemInterface();
	osg::GraphicsContext::ScreenSettings ss;
	if (_screens->_realworld->empty())
	{
		//multiple screens with one camera
		const unsigned numColumns = _screens->_scrs->getNumElements(), numRows(1);
		for (unsigned i = 0; i < numColumns; i++)
		{
			unsigned si = *(_screens->_scrs->begin() + i);
			wsi->getScreenSettings(si, ss);
			osg::ref_ptr<osg::Camera> camera = createSlaveCamera(si, ss);

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
			osg::ref_ptr<osg::Camera> camera = createSlaveCamera(si, ss, tileWidth*i);

			const double rotation = _screens->_realworld->back() * (i - 1);

			osg::Matrix viewOffset = osg::Matrix::translate(-eye) *
				osg::Matrix::rotate(rotation * TO_RADDIAN, UP_DIR) *
				osg::Matrix::translate(eye);

			view->addSlave(camera.release(), osg::Matrix(), viewOffset, true);
		}
	}

	return view.release();
}

osg::Camera * MulitViewer::createSlaveCamera(const unsigned screenNum, const osg::GraphicsContext::ScreenSettings &ss, const int startX /* = 0 */, const int startY /* = 0 */)
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

	osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());

	osg::ref_ptr<osg::Camera> camera = new osg::Camera;
	camera->setGraphicsContext(gc.get());
	camera->setViewport(new osg::Viewport(0,0,ss.width,ss.height));

	GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
	camera->setDrawBuffer(buffer);
	camera->setReadBuffer(buffer);

	return camera.release();
}