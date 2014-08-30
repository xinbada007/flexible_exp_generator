#pragma once
#include <osgViewer/ViewerEventHandlers>
#include <osgDB/FileUtils>

#include <osgAudio/SoundState.h>

#include <vector>

typedef std::vector<osg::ref_ptr<osgAudio::FileStream>> MusicFileList;

class MusicPlayer :
	public osgGA::GUIEventHandler
{
public:
	MusicPlayer();
	virtual ~MusicPlayer();

	bool handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa);
	void setHUDCamera(osg::Camera *cam);
private:
	bool joystick();
	void loadMusic();
	void playMusic();

	bool _ifPlay;

	osg::ref_ptr<osg::UIntArray> _buttons;
	osg::ref_ptr<osgAudio::SoundState> _music;
	MusicFileList _fileMusic;
	MusicFileList::const_iterator _nthFileStream;
	osgDB::DirectoryContents _mList;
	osgDB::DirectoryContents::const_iterator _nthMusic;
	
	osg::Camera *_cameraHUD;
	osg::ref_ptr<osg::Geode> _geodeHUD;
	osg::ref_ptr<osgText::Text> _textHUD;

	static const unsigned MUSICBUTTON;
	static const unsigned CHANGEMUSIC;
};

