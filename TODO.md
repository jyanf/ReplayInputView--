显示胜方无敌？

颜色微调：墨绿



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

## Tag

弹幕等级、擦弹状态、

无敌倒计时、

CtCl

不可防；不可擦

数值：身代HP、damage、lv、ProjCollideCounter、GrazeTimer

标签：体术/弹幕；
