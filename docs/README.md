# ReplayInputView++

This mod is a renewed edition of **ReplayInputView** and **ReplayInputView+** through customized development, specifically designed to display inner game info for *Hisoutensoku*, which helps players build a insight for general gameplay or mods developing.

To make it simple, we use **RIV** in the following to represent the term ***ReplayInputView***.

## Ⅰ: Basic Functions

### 1. BoxDisplaying(F4)

![Hitbox(R) and Hurtbox(G) displayed](box1.png)

Displays game <span style="color:#ff8080">hitboxes</span> and <span style="color:#80ff80">hurtboxes</span> in replay and local duel, as a convenient and clear approach to visualize and find out what's going on.

Also displays <span style="color:#f0f080">collision box</span> and an <u>position anchor</u>, which is firstly implemented by **RIV+**.

### 2. Debug Info(F6)

The original debug info panel is deprecated out of memory leak when using along with giuroll.

View its reworked and superior edition in the following "What's New" part.

### 3. Input Display(F7)

![](input1.png)

This panel splits into 2 part, the upper record of history inputs, the lower panel for real-time directional key and buttons by frames.

### 4. Decelerate(F9)/ Accelerate(F10)/ Pause(F11)/ Frame Step(F12)

Allow key control on battle process of original game.

The delay-adjusting method has becomes invalid due to giuroll's locking framerate to 60f+, so only discrete integer rate steps can be used now, eg. $\frac{1}{3}\rightarrow\frac{1}{2}\rightarrow1\rightarrow\frac{2}{1}\rightarrow\frac{3}{1}$



### 5. Modified combo counter to show at 1 hit

This is to simply edit assembly value and breaks the hidden-until-2-hits limitation.





## Ⅱ: What's New

### 1. About Boxes

This is where this mod began. The idea of re-developing RIV came from inaccurate and missing box display on objects, which sets me off on this long developing journey. Here are the new features that I believe introduce real improvement.

#### Add missing hurtbox for objects (bullet collision)

![](box0.png)

Now you can see them.

The color profile is adjustable in config file, with different colors and styles to tag special box states like invulnerable, parry bullet/melee, yukari gap and so on.

By the way, I also adds boxes for ground heights.



#### Remove fill color for inactive boxes

This really marks fake boxes and make box logic more accurate, especially for bullets.

![](box4.png)

More works are done to filter some other fake box cases.



#### Make use of d3d9 stencil test to avoid box overlapping together

![](box3.png)

As shown in the picture, which one is clearer?

Also, you have a choice to only keep the outer most outline, which is considered as "box merging", to make them even more distinguishable between objects and players.

<img src="box2.png" style="zoom:50%;" />

### 2. Supported all the local play mode

By **RIV++**, all the functions are now available also in Arcade mode and **Story mode**.

We can now finally view boxes in story! 



### 3. Added Armor Meter

Dragon-star armor mechanism now has been fully analyzed. Turn on boxes to view the circles and find out how it works.

![](armor.png)



### 4. Input panel optimizations

![image-20251110190649147](input0.png)

#### Displays buffer by highlighted stroke

Input buffers are now visible if you are in a cancel chain.

#### Fade out history recorded buttons

The record displayed now has a fade out effect to make later inputs distinguished. 



### 5. Brand new debug panel(F6)

![](debug0.png)

Or we should call it a **window** now. To enable panels to displays more info as possible, we developed it to be a individual window using *D3DAdditionalSwapChain* technique.

We also designed a gui system to manage expanded layouts functions through xml files, to make it more portable and flexible. Which means, it is even able to be customized if you like.

To also enable object debug, mouse interface is introduced. You can select/deselect anchors in game screen using L/R buttons of mouse and then view its info through the debug window, just like players.

![](anchor-gui.png)

Hint: gui object anchors are rendered as a diamond.

The window has been implemented with extra interfaces of:

1. right click on caption, copy a hex string of current selected object address
2. double click on caption, reset the position and size of window, and follow game's window again
3. mouse hovering on elements would display doc defined descriptions, some as formatted string to display its value
4. hotkeys to quickly select/deselect players without using mouse



### 6. Break hotkey range limits

The original RIV checks input from FKey array, which limits hotkey choice to function keys. And RIV++ turns to direct x input, so any key on keyboard is now usable.



### 7. Takeover combo meter updating

![](paused.gif)

It's annoying that, when game is paused, combo meter doesn’t show the first hit in real time — blame the pop-up animation.

So RIV++ takes over combo meter updating when paused to make another QoL.



## Ⅲ: How do I customize it?

#### This mod uses default assets as shown below:

<details open>  <summary>🗃️ modules/ReplayInputView++/</summary> <ul>
    <li>💾 <b>ReplayInputView++.dll</b></li>
    <li>⚙️ <b>ReplayInputView++.ini</b></li>
    <details open> <summary>📁 fonts/</summary>
      <ul>
        <li>🔠 CascadiaCode-Mod.ttf</li>
        <li>🔠 SmileySans-Mod.ttf</li>
        <li>📄 font-expand-codepage.py</li>
        <li>📄 <b>Drag n Drop to expand.cmd</b></li>
      </ul>
	</details>
    <details open> <summary>📄 <b>ReplayInputView.dat</b></summary> <ul>
        <details open> <summary>📁 rivpp/</summary> <ul>
            <li>🧱 ArmorBar.png</li>
            <li>🧱 ArmorLifebar.png</li>
            <li>🧱 back.png</li>
            <li>🧱 hint.png</li>
            <li>🧱 cursor.png</li>
            <li>📦 * titlebox and frame images</li>
            <li>🔢 * number font images</li>
            <li> * icons images</li>
            <li><b>🧩 layout.xml</b></li>
            <li><b>🧩 layout_plus.txt</b></li>
        </ul> </details>
      </ul> </details>
  </ul> </details>

### ReplayInputView++.ini

Lets's start from taking a look at the content of `.ini` config file.

#### [Keys]

Hotkeys are editable, it's ok to change to other keys you like.

#### [ColorProfile]

You can change colors for different box states.

For more details, view descriptions in .ini instead.



### Replacing files in .dat

Feel free to unpack `ReplayInputView++.dat` file with [**shady-cli**](https://hisouten.koumakan.jp/wiki/Shady_Packer/Shady_Cli), and replace files to customize it.

```bash
::Unpack Commands For example
REM Unpacking ReplayInputView++.dat to folder 
shady-cli -pack -m dir -o "" "ReplayInputView++.dat"
```

After finishing replacement, make a `data -> rivpp -> *Files` structured folder and pack it back to `.dat` file using:

```bash
::Re-pack Commands For example
REM Packing folder files back to ReplayInputView++.dat
shady-cli -pack -m data -o "ReplayInputView++_custom1.dat" "data"
```

Then you can change `[Assets] File=` option in `ReplayInputView++.ini` to switch between the original and customized `.dat` files.



## Editing layout+ for debug window

The layout+ system are consist of 2 layout file: 

1. **layout.xml**; this file is a to make use of vanilla layout system. Multiple assets are loaded through this, and then referenced and managed by object `id` defined inside.
2. **layout_plus.txt**; this file is the real one that manages to put everything in place, and then enables all customization features.
   The mod reads xml structure by raw text to support new tags and unlimited style, so its extension name must be `.txt` to avoid being converted in packing process. 

The following are detailed introduction for layout+ nodes.

> This xml has expanded features out of original soku layout system, implemented by ReplayInputView++
> 本布局文件含有非想天则原有布局系统之外的扩展特性（由RIV++负责实现）
>
> 继承关系如图：

#### Introductions: 

#### 介绍如下：

  - `<window width="240" height="600"><window/>`
    Class window: controls the size of debug window by w/h properties.
    窗口标签：宽高属性控制窗口大小
    
    One version `<textbox>` is allowed to put inside to display a generic info.
    出于调试考虑添加了版本字符串属性。
    
  - `<fonts></fonts>`
    Class fonts: place a font collection for later reference.
    字体组标签：用于放置所有字体标签。
    
  - `<font/>`
    Class font: describes a font with multiple attributes.
    字体标签：用多种属性描述字体实例。
    
    Available attr:
    可用属性：
    
    1. `id`, a integer with which text element can get a fontdesc by `font_id` and render.
      ID数字，后续文本元素可以使用`font_id`属性引用对应的字体进行渲染。
    2. `name`, internal name of a font file; should have been installed, or as a file existed in mod's font folder.
      *(the font file must support current system codepage to be successfully loaded)*
      名称字符串，目标字体的内部名称；需要字体已被安装，或者放置在font文件夹中。
      字体文件必须支持当前的[代码页/字符集](#about-fonts)。
    3. `stroke`, use game's black stroke/shadow
       是否增加黑色描边
    4. `bold`, bold font
       加粗与否
    5. `wrap`, would wrap with a given textbox, need `textbox` to have given `width` attribute.
       换行；内容达到文本框宽度后将自动换至下一行，因此要求显示指定文本框宽度。
    6. `height`, font size (in pixel)
       字形大小(像素)
    7. `color_top`/`color_bottom`, enables vertical gradient color
       *(if these are set and not equal to each other, then it's possible to colorize texts later using color tag in strings)*
       控制纵向渐变颜色；若二者颜色不同，则在文本中使用color标签时，着色模式由相乘改为叠加。
    8. `spacing`/`xspacing`, horizontal character spacing
       字间距修正值
    9. `yspacing`, spacing between lines
       行间距修正值
    
  - `<sublayout></sublayout>`

    Class sublayout: handles inheriting and stacking of grouped elements.
    子布局标签；

    ………………………………………………wip………………………………………………
#### Customization: 
以下为自定义部分：

2 types of customization is recommended:

  - `<sublayout class="suwako" inherit="Player"><sublayout/>`
    The system will check if there's compatible name for current character, so it's possible to override default layout of a character.
    Normally you should use `inherit` attribute to derive the default Player contents.
    It's case sensitive and make sure you use the internal english name. (the same ones in `th123a.dat> data/character`).
  - `<sublayout class="common.1006" inherit="Object"></sublayout>`
    `<sublayout class="alice.805" inherit="Object"></sublayout>`
    Also, object can also be derived by `character name + actId` . It's sometimes useful to tag out their remained time.



#### Others:

Preserved `id` range in layout.xml
**保留id范围(0~99)及用途**: 

    1. `0`: unindexable always-display id;
        原版的默认显示id，不可索引；
    2. `1`: 
        鼠标复制的提示图片；
    3. `2`: 光标图片.

#### About Fonts

To keep compatibility for users, we put used fonts in folder and loads them dynamically.

If you want to use your own font, make sure that:

1. **Be aware of the real inner name of font file**: font loading uses inner font-family name or full name with styles, use advanced tools to check the real name of a font file.
2. **Make sure the font supports the charset of your game or PC codepage (usually ANSI, related to th123intl setting)**: even if you only uses ASCII part, codepages not supported still fail font loading and fallback the font to default. 
   We provided a **font-forge** script in python to allow you to quickly expands your font file to full codepage even without available glyphs. The default "CascadiaCode-Mod.ttf" is literally a example.



---

## Special Thanks to:

Testers: 三回転Tstar@[bilibili](https://space.bilibili.com/357511007), monte@discord, miko@QQ; 

Technical support: @enebe-nb, @PinkySmile, @Hagb, @S-len; 

Community support: @soku-cn community players



Honorable credit to the original authors of **RIV**(anonymous one) and **RIV+**(@Shinki, @PCvolt and @delthas), for their pioneering work.



Hates: @ichirin. it tooks us a whole day to find out and fix the problem that your *ScoreTracker* and *PracticeEx* has polluted and ruined `pd3dDev->EndScene()` function by hasty hooks.
just kidding lmao, we love your CIF and other magical mods
