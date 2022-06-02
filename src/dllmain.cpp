#include "includes.h"
#include "fmod.hpp"

bool _ignoreStop = false;

class MusicFadeOut : public CCActionInterval {
public:
    static MusicFadeOut* create(float d) {
        auto ret = new MusicFadeOut;
        if(ret->initWithDuration(d)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
    virtual void update(float time) {
        auto target = (gd::FMODAudioEngine*)getTarget();
        if(!target || target->m_bFading)
            return;
        if(time >= 1.f) {
            _ignoreStop = false;
            target->m_pGlobalChannel->stop();
            return;
        }
        target->m_pGlobalChannel->setPitch(1.f - time);
    }
};

void (__thiscall* PlayLayer_destroyPlayer)(gd::PlayLayer* self, gd::PlayerObject*, gd::GameObject*);
void __fastcall PlayLayer_destroyPlayer_H(gd::PlayLayer* self, void*, gd::PlayerObject* player, gd::GameObject* obj) {
    _ignoreStop = true;
    PlayLayer_destroyPlayer(self, player, obj);
    _ignoreStop = false;
}

// ChannelControl::stop has some kind of protection that doesn't let me hook it
// so i stepped through it and found out which call inside it actually stops the music
// and i can actually hook this one
int (__thiscall* ChannelControl_stop_internal)(void*, unsigned int);
int __fastcall ChannelControl_stop_internal_H(void* self, void*, unsigned int idk) {
    if(_ignoreStop) {
        auto gm = gd::GameManager::sharedState();
        if(!gm)
            return 0;
        gm->getActionManager()->addAction(MusicFadeOut::create(0.4f), gd::FMODAudioEngine::sharedEngine(), false);
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