DeepSeek Harness Web GUI · 游戏皮肤安装说明
============================================

本目录皮肤（浏览器侧注入，不改任何 DSH 文件，随时可卸载）：
  overwatch-skin.css   守望先锋风格（深海军蓝 + 青色 + 橙）
  valorant-skin.css    无畏契约风格（近黑深蓝 + Valorant 红 + 米白）

【官方皮肤】= 界面右下角/设置 → 外观 → 浅色 / 深色 / 跟随系统（无需安装）

安装方法（推荐 Stylus，二选一）
--------------------------------
方法 A：Stylus 扩展（皮肤管理器，可一键开关多款皮肤）
  1. Chrome/Edge 应用商店搜索安装 "Stylus"
  2. 打开本页面 (127.0.0.1:3080)
  3. 点浏览器工具栏 Stylus 图标 → 「为此站点编写新样式」
  4. 把选中的 .css 文件内容全部粘贴进去 → 命名保存
  5. 换皮肤 = 点 Stylus 图标开关对应样式；恢复官方 = 全部关闭

方法 B：Tampermonkey 油猴脚本（不想装 Stylus 时）
  1. 安装 Tampermonkey 扩展 → 新建脚本
  2. 把选中皮肤的 CSS 包在下面壳子里：
     // ==UserScript==
     // @name        DSH 守望先锋皮肤
     // @match       http://127.0.0.1:3080/*
     // @run-at      document-start
     // ==/UserScript==
     (function(){var s=document.createElement('style');
     s.textContent=document.getElementById('x')?
     '':'/*这里粘贴CSS*/';document.head.appendChild(s)})();
  3. 保存并启用即可

快速预览（不想装任何扩展）
--------------------------------
F12 打开开发者工具 → Console 粘贴：
  fetch('http://127.0.0.1:3080/皮肤.css')  // 本机文件无法直接 fetch，
                                          // 请把 CSS 内容整体复制后改用：
  (function(){var s=document.createElement('style');
  s.id='dsh-skin';s.textContent='/*粘贴CSS内容*/';
  document.head.appendChild(s)})();
刷新页面即恢复官方主题。

注意事项
--------------------------------
- 皮肤只对当前浏览器生效；换电脑/浏览器需重新安装
- 若 GUI 端口不是 3080，请改 @match 里的地址
- 深色为主打；浅色（沙漠/浅蓝/浅沙）在「跟随系统」且系统为浅色时生效
