#include "SoundManager.h"

CSoundManager::CSoundManager()
{
	FMOD::System_Create(&m_System);

	m_numChannel = 32;
	m_System->init(m_numChannel, FMOD_INIT_NORMAL, nullptr);

	for (int i = 0; i < m_numChannel - 2; ++i) {
		m_vFxChannels.emplace_back();
		m_vFxChannels.back() = 0;
	}
}

CSoundManager::CSoundManager(size_t numChannel)
	: m_numChannel(numChannel)
{
	FMOD::System_Create(&m_System);

	m_System->init(m_numChannel, FMOD_INIT_NORMAL, nullptr);

	for (int i = 0; i < m_numChannel - 2; ++i) {
		m_vFxChannels.emplace_back();
		m_vFxChannels.back() = 0;
	}
}

CSoundManager::~CSoundManager()
{
	m_BGMChannel->stop();
	m_EnmChannel->stop();
	for (auto& p : m_vFxChannels)
		p->stop();

	m_System->close();
	m_System->release();
}

// ==========================================================

CMonsterChaserSoundManager::CMonsterChaserSoundManager()
	: CSoundManager()
{
	SetupSounds();
}

CMonsterChaserSoundManager::CMonsterChaserSoundManager(size_t numChannel)
	: CSoundManager(numChannel)
{
	SetupSounds();
}

CMonsterChaserSoundManager::~CMonsterChaserSoundManager()
{
	for (auto& p : m_vSounds)
		if (p)
			p->release();
}

void CMonsterChaserSoundManager::SetupSounds()
{
	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Title_bgm.mp3", FMOD_LOOP_NORMAL, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\click.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Ready.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\start_game.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\stage1_ambient.mp3", FMOD_LOOP_NORMAL, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Hit.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\SwordSlash.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\WandSwing.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\stage2_ambient.mp3", FMOD_LOOP_NORMAL, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Skill_Laser.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Parry.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Skill_M1.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Skill_M2.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Skill_P3.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\stage3_ambient.mp3", FMOD_LOOP_NORMAL, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\stage3_boss.mp3", FMOD_LOOP_NORMAL, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Roar.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\shield_attack.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\healing.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());

	m_vSounds.emplace_back();
	m_System->createSound("src\\sound\\Dead.mp3", FMOD_DEFAULT, 0, &m_vSounds.back());
}

void CMonsterChaserSoundManager::StartBGM(ESOUND sound)
{
	m_BGMChannel->stop();
	m_System->playSound(m_vSounds[sound], 0, false, &m_BGMChannel);
}

void CMonsterChaserSoundManager::StopBGM()
{
	m_BGMChannel->stop();
}

void CMonsterChaserSoundManager::StartAMB(ESOUND sound)
{
	m_EnmChannel->stop();
	m_System->playSound(m_vSounds[sound], 0, false, &m_EnmChannel);
}

void CMonsterChaserSoundManager::StartFx(ESOUND sound)
{
	m_vFxChannels[m_current_channel]->stop();
	m_System->playSound(m_vSounds[sound], 0, false, &m_vFxChannels[m_current_channel]);
	++m_current_channel;
	if (m_current_channel >= m_numChannel - 2)
		m_current_channel = 0;
}

void CMonsterChaserSoundManager::AllStop()
{
	m_BGMChannel->stop();
	m_EnmChannel->stop();
	for (auto& p : m_vFxChannels)
		p->stop();
	m_current_channel = 0;
}
