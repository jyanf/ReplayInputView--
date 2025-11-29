# Task

- [ ] untech条显示修正，面对一些演出受身

- [x] UAC/全屏崩溃

- [x] 电线杆贴图显示问题

  > 龙须debug
  >
  > ![image-20251114114031609](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20251114114031609.png)
  >
  > 蕾米debug
  >
  > ![image-20251114114247308](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20251114114247308.png)
  >
  > ---
  >
  > ```cpp
  > void __thiscall mmmSpriteExtension(SpriteEx *this, 
  >                                 float param_2, float param_3,
  >                                 float param_4, float param_5,
  >                                 float _sizex, float _sizey
  > ){//0x406d
  >  float fVar1, fVar2, fVar3, fVar4;
  >  // 如果贴图句柄无效
  >  if (this->base.dxHandle == 0) {
  >      this->size2.x = 0.0f;
  >      this->size2.y = 0.0f;
  >  }
  >  else {
  >      // 计算 UV 尺寸百分比
  >      fVar2 = param_4 / this->base.size.x;
  >      this->size2.x = fVar2;
  > 
  >      fVar1 = param_5 / this->base.size.y;
  >      this->size2.y = fVar1;
  > 
  >      // 左上角 UV
  >      fVar4 = param_2 / this->base.size.x;
  >      this->base.vertices[0].tu = fVar4;
  > 
  >      fVar3 = param_3 / this->base.size.y;
  >      this->base.vertices[0].tv = fVar3;
  > 
  >      // 右上角 UV = 左上角 + UV宽度
  >      float tu2 = fVar4 + fVar2;
  >      this->base.vertices[1].tu = tu2;
  >      this->base.vertices[1].tv = fVar3;
  > 
  >      // 左下角 UV = 左上角 + UV高度
  >      float tv2 = fVar3 + fVar1;
  >      this->base.vertices[2].tu = fVar4;
  >      this->base.vertices[2].tv = tv2;
  > 
  >      // 右下角 UV
  >      this->base.vertices[3].tu = tu2;
  >      this->base.vertices[3].tv = tv2;
  >  }
  > 
  >  // 设置顶点坐标（基于中心偏移）
  >  this->baseCoords[0].x = -_sizex;
  >  this->baseCoords[0].y = -_sizey;
  > 
  >  this->baseCoords[1].x = param_4 - _sizex;
  >  this->baseCoords[1].y = -_sizey;
  > 
  >  this->baseCoords[2].x = -_sizex;
  >  this->baseCoords[2].y = param_5 - _sizey;
  > 
  >  this->baseCoords[3].x = param_4 - _sizex;
  >  this->baseCoords[3].y = param_5 - _sizey;
  > }
  > ```

  - [x] 子A又出问题

  > 检查父A时，可边框亦有效？
  >
  > 测试可边框父A时未设置child导致的? 
  >
  > ```cpp
  > 
  > if (
  >     // ① 0x575 between rounds related
  >     attacker->gameData.owner->field_0x575 == 0 && victim->field_0x575 != 2
  >     &&
  >     // ③ FUN_0046a200
  >     !checkUntechEnded(victim)
  >     &&
  >     // ④ 受击方 realLimit < 100
  >     victim->realLimit < 100
  >     &&
  >     // ⑤ relationships
  >     ( 
  >         attacker->gameData.owner != victim
  >      || !attacker->boxData.frameData->attackFlags.friendlyfire
  >      || attacker->gameData.owner->gameData.opponent->field_0x575 == 0
  >     )
  > )
  > {
  >     if (attacker->childrenA.size() == 0) {
  >         FUN_0047c770(this, attacker, victim);
  >         return;
  >     }
  >     if (FUN_0047c770(this, attacker, victim)) {
  >         attacker->ShareCollisionStateToChildrenA(attacker);//465020
  >         return;
  >     }
  > 
  >     // 遍历子对象链表
  >     for (auto* child: attacker->childrenA) {
  >         if (FUN_0047c770(this, child, victim)) {
  >             attacker->ShareCollisionStateToChildrenA(child);
  >             return;
  >         }
  >     }
  > }
  > 
  > ```
  >
  > 

- [x] 4Psoku未适配

  - [x] 故事模式用的DataManager?

- [x] x关不掉了，auto on的锅

- [x] 心抄斩显示有误

  > 子物体需要依赖父物体至少有一红框
  >
  > 绿框呢？无需
  >
  > 此外sharedbox也对附加框生效

- [x] 全无敌时显示有误

- [x] 全屏元素覆盖description无法消除？

  - [x] 快捷键转空布局tip不消失？

- [x] vld链接有误，放入th123不识别

  > manifest文件忘了复制过去了
  >
  > 调整custom command

- [x] 暂停时F2\F3转发

  - [ ] 避免冲突

- [ ] 顶点兼容性问题与内存泄漏？

- [ ] hitstop讨论

  - [ ] 蕾米C弹321321321
                 210210210
  - [ ] 两个数？

- [x] 快捷键快速切换

  - [x] [为1P，]为2P

- [x] 选择器动画

  - [ ] 和数字

- [x] 研究下满级龙眼

  - [x] 反射盾凭什么知道发弹？

  > ```cpp
  > //0x465100
  > void __thiscall HandleReflection(GameObjectBase *shield) {
  >     GameObjectBase *parent;
  > 
  >     parent = this;
  >     while (parent->parentA) {
  >         parent = parent->parentA;
  >     }
  >     parent->direction *= -1;
  >     parent->ally = shield->ally;
  >     parent->field_0x1a0 += 1;
  >     parent->opponent = shield->opponent;
  >     
  >     for (auto& childA : parent->childrenA) {
  >         childA.direction *= -1;
  >         childA.field_0x1a0 += 1;
  >         childA.ally = shield->ally;
  >         childA.opponent = shield->opponent;
  >     }
  > }
  > ```
  >
  > 0x47b6f0; maybe BattleManager member
  > ```cpp
  > bool __thiscall HandleReflectCollision(GameObjectBase *reflector,GameObjectBase *other) {
  >     if (other->ally == reflector->ally) {
  >         return true;
  >     }
  >     if (reflector->frameData->frameFlags.reflector) {
  >         if (FUN_0047a740(reflector,other)) {//check if box collided
  >             HandleReflection(other,reflector);
  >             reflector->field_0x1a0 += 1;
  >             mResetCollisionBuffer(this);
  >             return true;
  >         }
  >     }
  >     return false;
  > }
  > ```
  >
  > 0x57db7d jg→nop；移除反射弹生成上限

- [ ] 布局

  - [x] 无风4cd, short 4ce

  - [ ] 伊吹瓢

  - [ ] 黄砂56f（自己必被康）；梅雨80c（自己弹地）

  - [ ] 撰写readme

  - [x] 说明填写

  - [x] 图标们

  - [ ] 身代无敌

  - [x] 相杀/hp

  - [ ] 天气剑生效中

  - [x] 霸体已吸收伤害0x188

  - [x] 灵力书等回复速率？天狗扇子加移速？
    棒子

  - [x] 强制破防、可擦体术、

  - [x] 时停

  - [x] limit乘子

  - [x] delta位移乘子需要保存

    > ![image-20251107041803222](C:\Users\Lenovo\AppData\Roaming\Typora\typora-user-images\image-20251107041803222.png)
    >
    > 1. input
    >
    > 2. counters
    >    update players
    >    charm
    >
    > 3. collision
    >
    > 4. speed&rivermist??
    >
    > 5. movement←hook
    >
    >    proceed timers
    >    	timestop; hitstop; 4c0; 4bc
    >    	invul timers
    >    	add card if gauge is full
    >
    >    set redhp, weather, skillLevelA
    >    reset rivermist, speedXY
    >
    >    update positions by mul
    >
    >    
    >
    > 6. camera & onprocess

- [x] layout针对common 1000+

- [x] d3d9STENCIL并集绘制框

  - [x] 边框也？

- [ ] sprite可自定义基点/白框

- [x] NC问题

  - [x] 启用厚边后，指针锁定为箭头

    > 莫名奇妙，窗口样式问题？
    > 予以SET_CURSOR代劳

- [x] verify2

- [x] 进行坐标裁剪防止锚点出场地？

  > 0,1280;840,c.bottom

- [x] 同串不更新防闪动?

  - [x] 更新两步走，trylock失败时不清除

- [x] MOUSE_MOVE从同级区域切入时，虽然垫在下方但是缓存仍然命中——使用缓存对重叠情况无法兼容

  > （保证下方物体无描述，即描述互不重叠时可以接受）
  >
  > 缓存父区域解决

- [ ] 梅雨伪连显示？无伤显示？

- [x] 拖尾hover时一并变色？

  - [x] tail gui


  > - `setTail(actId, width, length, subdivision, blendMode)`
  >
  > - `inline int vertexBufferSize() const { return length * subdivision * 2 + 2; }`
  >
  > - ```cpp
  >   struct TailObject {
  >       GameObjectBase* parent; int segments, subdivision, unknown0C;//0C always -1?
  >       float width; int blendMode; Vector2f texCoord;
  >       int texId = 0; bool tracking;
  >       //
  >   }
  >   ```
  >   
  > - 

  - [ ] mode为paramsD ?

- [x] 气泡样式

  > TTS_BALLOON
  >
  > 配合TTF_TRACK，就不需要TTF_ABSOLUTE
  >
  > 坑爹的抢焦点；WS_EX_TRANSPARENT

- [x] Color也能description

  - [x] 换行符……`&#10;`

- [x] 编码适配

  > fileCP→wchar→gameCP
  >
  > - [x] xml根据游戏当前intl转换，或系统ACP

- [ ] 4 p None

  - [x] layout应用playerid还没弄好

- [ ] 角色数据存档解决倒计时?

- [ ] 副窗口交互

  - [x] 窗口跟随优化（不出屏），记录手动拖放的位置；但全屏时还是在侧（注意点选失焦？）
  - [ ] 允许（等比例）拖动缩放
  - [x] （双击标题栏）重置?
  - [x] tooltip窗口显示说明文字

    > 坑爹：cbSize 48 44 40
    >
    > 坑爹2：manifest加载获得新样式——actctx从资源创建时的dwFlags和参数
  - [x] 可点击跳转的pa\pb

    - [ ] 悬浮时有反馈？
  - [x] 样式切换！
  - [ ] > 抢到主窗口焦点时记得windowsresizer黑边适配、另，resizer全屏模式下应禁用拖动（防止bug）
    >
    > 考虑失焦时直接退出全屏？但是切换桌面的功能……

- [ ] 重置光标选择器样式，显示滚轮进程accu/thre

- [ ] 首次F6，默认给出指导图片

  - [ ] 右键标题栏，圈圈箭头
  - [ ] 拖动；双击或切换全屏重置大小！
  - [ ] 左键点选，右键释放
  - [ ] 去ini逛逛以关闭此提示！

- [ ] _stacker可以抽象到更上层，默认不启用

  - [x] 或者用xmlattr控制

- [ ] 代码规则宏化

- [x] 右击标题栏复制

  - [ ] 复制的提示图片

- [x] 图标渲染不完美

  - [ ] 对素材差1 pixel?
  - [x] 不管了，使用公用Sprite变量

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
>   >   - [ ] center(小字隐藏)
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
>   > - [ ] owner/ally/opponent
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
>   > - [x] spirit & delay
>   > - [ ] 取消等级(依旧icon，N0, Al10, Ah20, B30, C40, Sk50, Sc(60+)100
>   >   - [x] 移动取消锁（上升且<=100.0）
>   >   
>   >   - [x] 挥空取消锁（动作>=300且result为0）
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

