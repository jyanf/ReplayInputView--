## 故事模式仍然失败，未hook

## ~~一本滞留问题~~

显示胜方无敌？

## ~~无敌计时器自然归零问题~~

## 霸体、当身、防反？

## ~~Box空心问题~~

~~配合hitstop渐变？渐变色？~~

- 9的2C
- 共用判定类

奇异弹幕

- 有hit和type，残留，但是没有实际攻击
- ~~魔改铁轮~~
  - 子A是否需要递归？×
  - 子A但是prev是自己的

- ~~L3A但是prev是父A的~~
- 厌川
- 连续lag，如何分辨？

~~身代也要空心~~





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

