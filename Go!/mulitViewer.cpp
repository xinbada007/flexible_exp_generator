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
	osg::GraphicsContext::WindowingSystemInterface *wsi;
	wsi = osg::GraphicsContext::getWindowingSystemInterface();
	osg::GraphicsContext::ScreenSettings ss;

	int min_height = INT_MAX;
	int sum_width = 0;
	for (unsigned i = 0; i < _screens->_scrs->getNumElements(); i++)
	{
		unsigned si = *(_screens->_scrs->begin() + i);
		wsi->getScreenSettings(si, ss);
		min_height = ss.height < min_height ? ss.height : min_height;
		sum_width += ss.width;
	}
	const double aspect = (_screens->_aspect == 0) ? (double)(sum_width) / (double)(min_height) : _screens->_aspect;
	double fovy = (_screens->_realworld->empty()) ? 30.0f : _screens->_realworld->front();

	osg::ref_ptr<osgViewer::View> view = new osgViewer::View;	
	view->getCamera()->setProjectionMatrixAsPerspective(fovy, aspect, 0.1f, 0.5f);
	view->getCamera()->setClearColor(osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f));

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
		
		return view.release();
	}
	else
	{
		//multiple cameras with one screen
		osg::Vec3 center, eye, updir;
		view->getCamera()->getViewMatrixAsLookAt(eye, center, updir);
		int numColumns(3), numRows(1);
		int tileWidth = sum_width / numColumns;
		int tileHeight = min_height / numRows;
		unsigned si = _screens->_scrs->front();
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

		return view.release();
	}
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