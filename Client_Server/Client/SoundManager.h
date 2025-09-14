#pragma once
#include "stdafx.h"

// SoundManager Using Fmod

class CSoundManager {
public:
	CSoundManager();
	CSoundManager(size_t numChannel);
	virtual ~CSoundManager();
protected:
	FMOD::System* m_System;
	FMOD::Channel* m_BGMChannel = 0;
	FMOD::Channel* m_EnmChannel = 0;
	std::vector<FMOD::Channel*> m_vFxChannels;

	size_t m_numChannel;
	size_t m_current_channel{};
};



class CMonsterChaserSoundManager : public CSoundManager {
public:
	CMonsterChaserSoundManager();
	CMonsterChaserSoundManager(size_t numChannel);
	~CMonsterChaserSoundManager();

	void SetupSounds();

	void StartBGM(ESOUND sound);
	void StopBGM();
	void StartAMB(ESOUND sound);
	void StartFx(ESOUND sound);

	void AllStop();
protected:
	std::vector<FMOD::Sound*> m_vSounds;
};