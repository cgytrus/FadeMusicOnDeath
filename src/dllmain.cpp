#include "includes.h"
#include "fmod.hpp"

gd::PlayLayer* _playLayer = nullptr;

class MusicFadeOut : public CCActionInterval {
public:
    static MusicFadeOut* create(float d) {
        auto ret = new MusicFadeOut;

        if(!ret->initWithDuration(d)) {
            delete ret;
            return nullptr;
        }

        ret->autorelease();
        return ret;
    }

    virtual void update(float time) {
        auto target = (gd::FMODAudioEngine*)getTarget();
        if(!target || target->m_bFading) {
            target->stopAction(this);
            return;
        }

        if(_startPitch < 0.f)
            target->m_pGlobalChannel->getPitch(&_startPitch);

        if(time >= 1.f) {
            _playLayer = nullptr;
            target->m_pGlobalChannel->stop();
            target->m_pGlobalChannel->setPitch(_startPitch);
            return;
        }

        // reversed circular ease out
        float ease = 1.f - sqrt(1.f - pow(time - 1.f, 2.f));
        target->m_pGlobalChannel->setPitch(ease * _startPitch);
    }

    ~MusicFadeOut() {
        auto target = (gd::FMODAudioEngine*)getTarget();
        if(target && _startPitch >= 0.f)
            target->m_pGlobalChannel->setPitch(_startPitch);
    }

private:
    float _startPitch = -1.f;
};

void (__thiscall* PlayLayer_destroyPlayer)(gd::PlayLayer* self, gd::PlayerObject*, gd::GameObject*);
void __fastcall PlayLayer_destroyPlayer_H(gd::PlayLayer* self, void*, gd::PlayerObject* player, gd::GameObject* obj) {
    _playLayer = self;
    PlayLayer_destroyPlayer(self, player, obj);
    _playLayer = nullptr;
}

void (__thiscall* FMODAudioEngine_rewindBackgroundMusic)(gd::FMODAudioEngine* self);
void __fastcall FMODAudioEngine_rewindBackgroundMusic_H(gd::FMODAudioEngine* self) {
    self->stopAllActions();
    FMODAudioEngine_rewindBackgroundMusic(self);
}

// ChannelControl::stop has some kind of protection that doesn't let me hook it
// so i stepped through it and found out which call inside it actually stops the music
// and i can actually hook this one
int (__thiscall* ChannelControl_stop_internal)(void*, unsigned int);
int __fastcall ChannelControl_stop_internal_H(void* self, void*, unsigned int idk) {
    if(_playLayer) {
        auto gm = gd::GameManager::sharedState();
        if(!gm)
            return 0;

        float fadeOutTime = 0.5f;

        bool autoRetry = gm->getGameVariable("0026");
        bool fastPracticeReset = gm->getGameVariable("0052");
        if(autoRetry && _playLayer->m_isPracticeMode && !fastPracticeReset)
            fadeOutTime = 1.f;

        if(!autoRetry)
            fadeOutTime = 2.f;

        gm->getActionManager()->addAction(MusicFadeOut::create(fadeOutTime), gd::FMODAudioEngine::sharedEngine(), false);
        return 0;
    }
    return ChannelControl_stop_internal(self, idk);
}

DWORD WINAPI mainThread(void* hModule) {
    MH_Initialize();

    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    auto fmodBase = reinterpret_cast<uintptr_t>(GetModuleHandle("fmod.dll"));

    MH_CreateHook(reinterpret_cast<void*>(base + 0x20a1a0), PlayLayer_destroyPlayer_H,
        reinterpret_cast<void**>(&PlayLayer_destroyPlayer));

    MH_CreateHook(reinterpret_cast<void*>(base + 0x23f70), FMODAudioEngine_rewindBackgroundMusic_H,
        reinterpret_cast<void**>(&FMODAudioEngine_rewindBackgroundMusic));

    MH_CreateHook(reinterpret_cast<void*>(fmodBase + 0xb1c70), ChannelControl_stop_internal_H,
        reinterpret_cast<void**>(&ChannelControl_stop_internal));

    MH_EnableHook(MH_ALL_HOOKS);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
    if(reason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0x100, mainThread, handle, 0, 0);
    }
    return TRUE;
}