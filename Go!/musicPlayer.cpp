#include "stdafx.h"
#include "musicPlayer.h"

#include <osgAudio/SoundManager.h>

#include <osgDB/FileUtils>
#include <osg/Notify>

const unsigned MusicPlayer::MUSICBUTTON = 3;
const unsigned MusicPlayer::CHANGEMUSIC = 4;

MusicPlayer::MusicPlayer():
_music(NULL), _fileMusic(NULL), _ifPlay(false)
{
	_buttons = new osg::UIntArray;
	_buttons->assign(10, 0);

	std::string dir("..\\Resources\\sound\\bgm");
	_mList = osgDB::getSortedDirectoryContents(dir);
	_mList.erase(_mList.begin(), _mList.begin() + 2);
	_nthMusic = _mList.cbegin();

	_music = new osgAudio::SoundState;
	_music->setAmbient(true);
	_music->setLooping(false);
	_music->setPlay(false);
	_music->allocateSource(10);
	osgAudio::SoundManager::instance()->addSoundState(_music);
}


MusicPlayer::~MusicPlayer()
{
	if (_music)
	{
		_music.release();
	}
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
	if (joystick())
	{
		if (_buttons->at(MUSICBUTTON) == 2)
		{
			if (_music)
			{
				_ifPlay = (!_music->isPlaying());
			}
		}
		if (_buttons->at(CHANGEMUSIC) == 2)
		{
			if (_music)
			{
				_nthMusic++;
				_ifPlay = true;
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
			}
		}
	}

	loadMusic();
	playMusic();
	return false;
}

void MusicPlayer::loadMusic()
{
	if (_nthMusic == _mList.cend())
	{
		_nthMusic = _mList.cbegin();
	}

	try
	{
		_fileMusic = new osgAudio::FileStream(*_nthMusic);
	}
	catch (std::exception &e)
	{
		osg::notify(osg::WARN) << "Error:  " << e.what() << std::endl;
		osg::notify(osg::WARN) << "File Corrupted" << *_nthMusic << std::endl;
		_fileMusic = NULL;
	}

	if (_fileMusic)
	{
		_music->setStream(_fileMusic.release());
	}
}

void MusicPlayer::playMusic()
{
	if (_music->getStream())
	{
		_music->setPlay(_ifPlay);
	}
}