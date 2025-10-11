# Task

- [x] 铃仙214幻视涟漪滞留弹lag有误
  817|seq 03

  > pose0（动态框，红绿共享→有绿）→pose1 (特效，未消除动态框，红绿不共享→无绿)
  > spec.SharedBox根据prevFrameData可能滞后，check_lag时不应使用

- [x] 铃仙623分身(847)的counterhit没有身代

  > unknown1ac ==0 时身代无敌

- [ ] 暂停时，窗口最小化再恢复后，不dirty导致白屏
  切换窗口莫名丢失focus

- [ ] 暂停时立即显示combo info，跳过动画

- [x] 降低无限霸体圈透明度

---

- [x] timeGetTime精度不足，动画卡顿

- [x] appendDatFile必须在setup之前做

  随机种子遭到影响，初始化时固定54

  SokuLib::appendDatFile固定随机数种子

【！】

无windows resizer情况下：切全屏前后未重置交换链导致崩溃、全屏无法创建swapchain、黑边得到利用？进独占全屏前主动隐藏（或重置交换链？）
D3DPresent_paramters.Windowed?

hook resetd3d9device

        param_1->mmSavedConfig->fullscreen = param_1->mmSavedConfig->fullscreen == false;
        hWnd = (HWND)mSokuGame.HWND2;
        SendMessageA(hWnd,0x104,0xd,0);//a
        break;

~~box渲染次序问题：拆解drawPlayerBoxes~~

是否处于蓄力状态（压键）



显示胜方无敌？

- [x] 颜色微调：墨绿×?


~~第一帧仍然需要break~~

~~replay倒放情况~~

# Findings

>
>```cpp
>flags = atker->boxData.mFrameData->attackFlags;
>if (!flags.isBullet) {
>    if (victm->meleeInvulTimer != 0
>        || victm->gameData.frameData->frameFlags.meleeInvul)
>        return false;
>}
>else if (flags & 0x40000 == 0)
>    if (victm->projectileInvulTimer != 0
>        || victm->gameData.frameData->frameFlags.bulletInvul) {
>        return false;
>    }
>}
>
>GRAB:
>if (flags.grab) {
>
>}
>```
>
> | isBullet\\unk40000 | y                      | n                      |
> | ------------------ | ---------------------- | ---------------------- |
> | y                  | grabInvul              | projinvuls & grabInvul |
> | n                  | meleeInvul & grabInvul | meleeInvul & grabInvul |
>

- noSuperArmor→powerMultiplier

$$
源*(,,,) + 目*(1,1,1,1-A_s)//普通\\
源*(1,1,1,1) + 目*(1,1,1,1)//加性\\
(C_s*1 - C_d*1)*A_s + C_d*(1-A_s)//\\
(1-C_d)*A_s + C_d*(1-A)
$$

【AddtionalChain】

> hook scene vtable
>
> CBattle
>
> org→enterCritical？→save state→switch→render→（present）→switch→restore→leave？
>
> WM_PAINT→tryloc.
>
> k→`chain->present`
>

- ShowCursor(0) 线程差异		





## record

~~重进刷新~~



~~镜像从左到右~~

## CommandFade



## F6-Tags

- [ ] AlwaysShown框，另开窗口？
- [ ] 根据hover在固定位置显示说明


> - [ ] 弹幕等级
>
> - [ ] 擦弹状态和
>   无敌倒计时们
>
> - [ ] CtCl
>
>
> 不可防（空防不可）；不可擦
>
> 数值：身代HP、damage、lv、ProjCollideCounter、GrazeTimer
>
> - [ ] 标签：
>
>   - [x] 体术/弹幕；
>
>   - [ ] 友伤
>
>   - [x] AUB，UB
>
>   - [x] 投技
>
>   - [ ] 擦弹
>
>   - [ ] 无敌
>
>   - [ ] 不可反射
>
>   - [ ] 
>
>     
>
> - [ ] 嵌套窗口系统??
>   标题→小框→子内容
>   Base→
>   Player→
> - [ ] 点击跳转（地址 type→按钮）
> - [ ] 展开收起功能>
>   滚动条
> - [ ] 系统卡等级？？卡组？

```json
{
    "GameObjectBase": {
		"base": null,
    },
    "Player": {
        "base":"GameObjectBase",
        "Frames":{
            ""
        },
        "Fields": {
            "0": null,//
            "10": null,//
            "11": null,//
            //reserved id below
            "123": {//adjust pos in layout
                "Name_Icon":12,//str, or index of png chunks 
                "Description": "text to be shown",
                "Value": {
                    "offset":"0x1",
                    "size": 4,
                    "pointer": null,
                    "type": "dec"//hex, str, float
                }
            },
            "100": {
                
            }
        }

    },
    "GameObject": {
        "base": "GameObjectBase",
        "":null
    },
    "Suwako": {
        base: "Player",
        
    }
}
```



- [x] ## [ColorProfile] .A .R .G .B .RenderMode？


> Panel.LU
>
> Panel.LD
>
> Panel.RU
>
> Panel.RD
>
> HurtBox.Entity
>
> HitBox.Bullet
>
> Hitbox.Melee
>
> Hitbox.Grab
>
> Hitbox.
>
> 
>
> Hurtbox.Bullet
>
> Hurtbox.Reflector
>
> Hurtbox.Gap
>
> HurtBox.Guard
>
> Hurtbox.Parry = true;当身
>
> CollisionBox
>
> [Extra]
>
> GroundBox
>

## [Settings]

HollowLight

Hitbox.ShadeByHitstop = true

accer method = default

