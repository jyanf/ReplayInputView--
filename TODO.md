timeGetTime精度不足，动画卡顿

随机种子遭到影响，初始化时固定54

SokuLib::appendDatFile固定随机数种子

appendDatFile必须在setup之前做

【！】

无windows resizer情况下：切全屏前后未重置交换链导致崩溃、全屏无法创建swapchain、黑边得到利用？进独占全屏前主动隐藏（或重置交换链？）
D3DPresent_paramters.Windowed?

hook resetd3d9device

        param_1->mmSavedConfig->fullscreen = param_1->mmSavedConfig->fullscreen == false;
        hWnd = (HWND)mSokuGame.HWND2;
        SendMessageA(hWnd,0x104,0xd,0);//a
        break;

box渲染次序问题：拆解drawPlayerBoxes

是否处于蓄力状态（压键）



显示胜方无敌？

颜色微调：墨绿×?

~~第一帧仍然需要break~~

~~replay倒放情况~~



```cpp
flags = atker->boxData.mFrameData->attackFlags;
if (!flags.isBullet) {
    if (victm->meleeInvulTimer != 0
        || victm->gameData.frameData->frameFlags.meleeInvul)
        return false;
}
else if (flags & 0x40000 == 0)
    if (victm->projectileInvulTimer != 0
        || victm->gameData.frameData->frameFlags.bulletInvul) {
        return false;
    }
}

GRAB:
if (flags.grab) {
    
}
```

| isBullet\\unk40000 | y                      | n                      |
| ------------------ | ---------------------- | ---------------------- |
| y                  | grabInvul              | projinvuls & grabInvul |
| n                  | meleeInvul & grabInvul | meleeInvul & grabInvul |

noSuperArmor→powerMultiplier

## record

~~重进刷新~~



~~镜像从左到右~~

## CommandFade



## F6-MouseTag

AlwaysShown框，另开窗口？

弹幕等级、擦弹状态、

无敌倒计时、

CtCl

不可防；不可擦

数值：身代HP、damage、lv、ProjCollideCounter、GrazeTimer

标签：体术/弹幕；

友伤

## [ColorProfile] .A .R .G .B .RenderMode？

Panel.LU

Panel.LD

Panel.RU

Panel.RD

HurtBox.Entity

HitBox.Bullet

Hitbox.Melee

Hitbox.Grab

Hitbox.



Hurtbox.Bullet

Hurtbox.Reflector

Hurtbox.Gap

HurtBox.Guard

Hurtbox.Parry = true;当身

CollisionBox

[Extra]

GroundBox

## [Settings]

HollowLight

Hitbox.ShadeByHitstop = true

accer method = default



[blend]
$$
源*(,,,) + 目*(1,1,1,1-A_s)//普通\\
源*(1,1,1,1) + 目*(1,1,1,1)//加性\\
(C_s*1 - C_d*1)*A_s + C_d*(1-A_s)//\\
(1-C_d)*A_s + C_d*(1-A)
$$
【AddtionalChain】

hook scene vtable

CBattle

org→enterCritical？→save state→switch→render→（present）→switch→restore→leave？

WM_PAINT→tryloc.

k→`chain->present`

ShowCursor(0) 线程差异		
