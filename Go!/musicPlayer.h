#pragma once
#include <osgViewer/ViewerEventHandlers>
#include <osgDB/FileUtils>

#include <osgAudio/SoundState.h>

class MusicPlayer :
	public osgGA::GUIEventHandler
{
public:
	MusicPlayer();
	virtual ~MusicPlayer();

	bool handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa);

private:
	bool joystick();
	void loadMusic();
	void playMusic();

	bool _ifPlay;

	osg::ref_ptr<osg::UIntArray> _buttons;
	osg::ref_ptr<osgAudio::SoundState> _music;
	osg::ref_ptr<osgAudio::FileStream> _fileMusic;
	osgDB::DirectoryContents _mList;
	osgDB::DirectoryContents::const_iterator _nthMusic;
	
	static const unsigned MUSICBUTTON;
	static const unsigned CHANGEMUSIC;
};

