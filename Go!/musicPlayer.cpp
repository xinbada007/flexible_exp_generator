#include "stdafx.h"
#include "musicPlayer.h"

#include <osgAudio/SoundManager.h>

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osg/Notify>

const unsigned MusicPlayer::MUSICBUTTON = 3;
const unsigned MusicPlayer::CHANGEMUSIC = 2;

MusicPlayer::MusicPlayer():
_music(NULL), _ifPlay(true), _cameraHUD(NULL)
, _geodeHUD(NULL), _textHUD(NULL)
{
	_buttons = new osg::UIntArray;
	_buttons->assign(10, 0);

	std::string dir("..\\Resources\\sound\\music\\");
	_mList = osgDB::getSortedDirectoryContents(dir);
	if (_mList.empty())
	{
		return;
	}
	osgDB::DirectoryContents::iterator del = std::find(_mList.begin(), _mList.end(), ".");
	if (del != _mList.end()) _mList.erase(del);
	del = std::find(_mList.begin(), _mList.end(), "..");
	if (del != _mList.end()) _mList.erase(del);

	if (_mList.empty())
	{
		return;
	}

	_nthMusic = _mList.cbegin();
	while (_nthMusic != _mList.cend())
	{
		osg::ref_ptr<osgAudio::FileStream> fm = NULL;
		try
		{
			fm = new osgAudio::FileStream(dir + *_nthMusic);
		}
		catch (std::exception &e)
		{
			osg::notify(osg::WARN) << "Error:  " << e.what() << std::endl;
			osg::notify(osg::WARN) << "File Corrupted: " << *_nthMusic << std::endl;
			fm = NULL;
		}

		if (fm)
		{
			_fileMusic.push_back(new osgAudio::FileStream(dir + *_nthMusic));
		}

		_nthMusic++;
	}

	_mList.push_back(dir);
	_nthMusic = _mList.cbegin();

	if (_fileMusic.empty())
	{
		return;
	}
	_nthFileStream = _fileMusic.cbegin();

	_music = osgAudio::SoundManager::instance()->findSoundState("GOmusic");
	if (!_music)
	{
		_music = new osgAudio::SoundState("GOmusic");
		_music->setAmbient(true);
		_music->setLooping(true);
		_music->setPlay(_ifPlay);
		_music->allocateSource(10);
		_music->setStream(*_nthFileStream);
		osgAudio::SoundManager::instance()->addSoundState(_music);
	}

	_textHUD = new osgText::Text;
	_geodeHUD = new osg::Geode;
	_geodeHUD->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	_geodeHUD->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

	playMusic();
}


MusicPlayer::~MusicPlayer()
{
// 	if (!_fileMusic.empty())
// 	{
// 		MusicFileList::iterator i = _fileMusic.begin();
// 		while (i != _fileMusic.end())
// 		{
// 			(*i).release();
// 			i++;
// 		}
// 	}
// 
// 	if (_music)
// 	{
// 		_music.release();
// 	}
}

bool MusicPlayer::joystick()
{
	extern bool poll_joystick(int &x, int &y, int &b);
	int x(0), y(0), b(-1);
	if (!poll_joystick(x, y, b))
	{
		return false;
	}

	if (b >= 0 && b < _buttons->size())
	{
		_buttons->at(b) = 1;
	}
	else if (b == -1)
	{
		if (_buttons->at(MUSICBUTTON) == 1)
		{
			b = MUSICBUTTON;
		}
		if (_buttons->at(CHANGEMUSIC) == 1)
		{
			b = CHANGEMUSIC;
		}
		_buttons->assign(_buttons->size(), 0);
		if (b >= 0 && b <_buttons->size())
		{
			_buttons->at(b) = 2;
		}
	}

	return true;
}

bool MusicPlayer::handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
{
	bool changedMusic(false);
	bool setPlay(false);

	if (joystick())
	{
		if (_buttons->at(MUSICBUTTON) == 2)
		{
			if (_music)
			{
				_ifPlay = (!_music->isPlaying());
				setPlay = true;
			}
		}
		if (_buttons->at(CHANGEMUSIC) == 2)
		{
			if (_music)
			{
				_nthMusic++;
				_nthFileStream++;
				changedMusic = true;
			}
		}
	}

	if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
	{
		const int &key = ea.getKey();
		if (key == ea.KEY_M)
		{
			if (_music)
			{
				_ifPlay = (!_music->isPlaying());
				setPlay = true;
			}
		}
		if (key == ea.KEY_Space)
		{
			if (_music)
			{
				_nthMusic++;
				_nthFileStream++;
				changedMusic = true;
			}
		}
	}

	if (changedMusic)
	{
		loadMusic();
		playMusic();
	}
	if (setPlay)
	{
		playMusic();
	}

	std::string display;
	if (_music)
	{
		if (_music->getStream() && _ifPlay && (!_music->isPlaying()))
		{
			display += osgDB::getNameLessAllExtensions(osgDB::getSimpleFileName(_music->getStream()->getFilename()));
			display += " - ENDED";
		}
	}
	if (!display.empty())
	{
		_textHUD->setText(display);
		_nthFileStream++;
		loadMusic();
		playMusic();
	}

	return false;
}

void MusicPlayer::loadMusic()
{
	if (_nthMusic == _mList.cend() - 1)
	{
		_nthMusic = _mList.cbegin();
	}

// 	std::string file = _mList.back() + *_nthMusic;
// 
// 	try
// 	{
// 		_fileMusic = new osgAudio::FileStream(file);
// 	}
// 	catch (std::exception &e)
// 	{
// 		osg::notify(osg::WARN) << "Error:  " << e.what() << std::endl;
// 		osg::notify(osg::WARN) << "File Corrupted" << file << std::endl;
// 		_fileMusic = NULL;
// 	}
// 
// 	if (_fileMusic)
// 	{
// 		_music->setStream(_fileMusic.release());
// 		_music->setPlay(_ifPlay);
// 
// 		_textHUD->setText(osgDB::getNameLessAllExtensions(*_nthMusic));
// 	}

	if (_nthFileStream == _fileMusic.cend())
	{
		_nthFileStream = _fileMusic.cbegin();
	}

	_music->setStream(*_nthFileStream);
}

void MusicPlayer::playMusic()
{
	if (!_music->getStream())
	{
		return;
	}

	_music->setPlay(_ifPlay);

	std::string display;
	display += osgDB::getNameLessAllExtensions(osgDB::getSimpleFileName(_music->getStream()->getFilename()));

	if (!_music->isPlaying())
	{
		display += " - NOT PLAYING";
	}
	else
	{
		display += " - PLAYING";
	}

	_textHUD->setText(display);
}

void MusicPlayer::setHUDCamera(osg::Camera *cam)
{
	if (!cam || !_music || !_textHUD || !_geodeHUD)
	{
		return;
	}

	_cameraHUD = cam;

	const double X = _cameraHUD->getViewport()->width();
	const double Y = _cameraHUD->getViewport()->height();

	osg::Vec3d position(0.5*X, 0.95*Y, 0.0f);
	std::string font("fonts/arial.ttf");
	_textHUD->setFont(font);
	_textHUD->setCharacterSize(24);
	_textHUD->setFontResolution(24, 24);
	_textHUD->setAlignment(osgText::TextBase::CENTER_TOP);
	_textHUD->setPosition(position);
	_textHUD->setDataVariance(osg::Object::DYNAMIC);

	_geodeHUD->addDrawable(_textHUD);
	_cameraHUD->addChild(_geodeHUD);
}