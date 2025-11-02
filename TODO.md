# Task

- [ ] 4 p None

  - [ ] playerid还没弄好

- [ ] 角色数据存档解决倒计时?

- [ ] 副窗口交互

  - [ ] 窗口跟随优化（不出屏），记录手动拖放的位置；但全屏时还是在侧（注意点选失焦）
  - [ ] 允许等比例拖动缩放+（双击标题栏）重置?
  - [ ] tooltip窗口显示说明文字
  - [ ] 可点击跳转的pa\pb
  - [ ] 样式切换！
  - [ ] 抢到主窗口焦点时记得windowsresizer黑边适配

- [ ] 重置光标选择器样式，显示滚轮进程accu/thre

- [ ] 首次F6，默认给出指导图片

  - [ ] 右键标题栏，圈圈箭头
  - [ ] 双击重置大小！
  - [ ] 左键点选，右键释放
  - [ ] 去ini逛逛以关闭此提示！

- [ ] _stacker可以抽象到更上层，默认不启用

  - [x] 或者用xmlattr控制

- [ ] 代码规则宏化

- [x] 右击标题栏复制

- [ ] 图标渲染不完美

  - [ ] 对素材差1 pixel?

- [ ] 虚标题重载？layout继承时自身标题放在最上方以覆盖

- [ ] 可视优化？：
  窗口滚轮缩放+记忆大小/
  /窗口滚轮滚动
  /多窗口
  /自由布局（layout同样stacker？）
  /甚至鼠标拖动交互；超·GUI……

- [x] 修复NCMOUSEMOVE不精确

  > `static bool tracked;`
  >
  > `if (!tracked) TrackMouseEvent(…);`

- [x] 休眠重进的DPI被改

  - [ ] 测试跨屏扩展，移动窗口情况


  > ```c++
  > CreateD3D
  > //GetWindowInfo(hwnd, &wi);
  > //d3dpp.BackBufferWidth = wi.rcClient.right - wi.rcClient.left;
  > //d3dpp.BackBufferHeight = wi.rcClient.bottom - wi.rcClient.top;
  > ```

- [x] hitstop等countdown lag?

- [x] 无敌、untech条再细化

  - [ ] 空中演出可受身？

  - [ ] 受身无敌？

- [x] 铃仙214幻视涟漪滞留弹lag有误
  817|seq 03

  > pose0（动态框，红绿共享→有绿）→pose1 (特效，未消除动态框，红绿不共享→无绿)
  > spec.SharedBox根据prevFrameData可能滞后，check_lag时不应使用

- [x] 铃仙623分身(847)的counterhit没有身代

  > unknown1ac ==0 时身代无敌

- [x] 暂停时，窗口最小化再恢复后，不dirty导致白屏

  - [x] 切换窗口莫名丢失focus

- [x] 暂停时立即显示combo info，跳过动画

- [x] 降低无限霸体圈透明度

---

## F6-Tags

- [ ] AlwaysShown框，另开窗口？
- [ ] 根据hover在固定位置显示说明
- [x] ref引用原版对象，image导入，相对切片等
- [ ] Layout继承Design
- [ ] 布局继承merge
- [x] container统一动态大小，用pad/pin（图钉）实现固定大小


> - [ ] Basic
>
>   > - [x] 位置
>   >   - [ ] center
>   >   - [ ] dir图标
>   > - [x] 速度，g
>   >   - [ ] addi
>   > - [x] actionid, seq, ps, frm
>   >   - [ ] 循环seq
>   > - [x] Ct、Cl
>   > - [x] hitstop（更新在前x）
>   > - [ ] 强制打康(蓝CH)0x1a2
>   > - [x] skillindex
>   > - [x] renderInfos(Tex)
>
> - [ ] Obj
>
>   > - [ ] isbullet
>   >   - [x] 弹幕等级/隙间等级
>   >   - [x] 相杀计数（额外）
>   >   - [x] 耐擦计数
>   > - [ ] hp(仅身代)
>   > - [ ] parentA/B: 仅icon，靠console显示指针
>   > - [ ] gpshort6, gpfloat3/custom data (3)
>   > - [ ] 不被暂停360
>   > - [ ] layer?
>
> - [ ] Player
>
>   > - [x] teamId→P3P4??
>   > - [ ] 川雾速度？？
>   > - [x] gpshort/float 6
>   >
>   > ---
>   >
>   > - [ ] untech
>   >   - [ ] stun防御硬直/受击硬直?
>   > - [ ] hp/redhp delta/maxhp
>   > - [ ] spirit & delay
>   > - [ ] 取消等级(依旧icon，N0, Al10, Ah20, B30, C40, Sk50, Sc(60+)100
>   >   - [x] 移动取消锁（上升且<=100.0）
>   >   
>   >   - [ ] 挥空取消锁（动作>=300且result为0）
>   >   
>   >     > bool resultCancellable = this->actionId < 300 || this->collisionType != 0 && this->collisionType != 3
>   > - [x] 一些天气标志；开关们
>   >   - [ ] canGrazeMelee
>   >   - [ ] 黄砂
>   >   - [ ] crush on WB
>   >   - [ ] charged atk
>   > - [ ] 乘子们
>   >   - [ ] 攻、防←棒子和人偶.2f
>   >   - [ ] speedXY; speedPower?
>   >   - [ ] power
>   >   - [ ] limit？
>   >   - [ ] 反甲
>   >   - [x] 连段伤害
>   >   - [ ] sc、skill、吸血、碎卡
>   > - [ ] 倒计时（更新在前）
>   >   - [x] 擦弹
>   >   - [x] 三无敌
>   >   - [x] SOR（封）
>   >   - [x] confusion（红眼）
>   >   - [ ] 回血符（星）
>   >   - [ ] 蓝药
>   >   - [ ] 龙星、天滴
>   > - [ ] DD锁
>   > - [ ] 
>
> - [x] 标签：
>
>   - [x] 站/蹲
>
>   - [x] 打站蹲
>
>   - [x] 空中
>
>   - [x] 体术/弹幕；
>
>   - [x] 友伤
>
>   - [x] AUB，UB
>
>   - [x] 投技
>
>   - [x] 擦弹
>
>   - [x] 无敌
>
>   - [x] 不可反射
>   - [x] Parry单独一栏（紫色上下行）f&a=|p
>   - [x] Armor与ignore Armor
>
> 
>
> - [x] 嵌套窗口系统??
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
                    "type": "dec",//hex, color, str, float
                    "hide_if": 0
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

- [x] timeGetTime精度不足，动画卡顿

- [x] appendDatFile必须在setup之前做

  随机种子遭到影响，初始化时固定54

  SokuLib::appendDatFile固定随机数种子

---



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

