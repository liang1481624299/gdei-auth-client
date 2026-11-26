# CAS 统一认证与个人门户对接调研 / CAS & Personal Portal Notes

> 来源：2026-11-22 对公网可访问页面、门户前端 bundle（React/axios）的静态分析；2026-11-26 在可达统一认证中心的网络下**实测匿名链路**（外壳页、login2022 表单、校验端点、子系统 302 跳转、前端 bundle）。
> Source: public pages + portal bundle static analysis (2026-11-22); anonymous live probing of shell page, login2022 form, validation endpoints and subsystem redirects (2026-11-26).
> 最后核对 / Last reviewed：本地 CST(UTC+8) `2026-11-26 21:26:02`。
>
> **环境限制 / Environment caveat**：2026-11-26 实测可匿名访问 CAS 外壳与表单、可探测未登录跳转链；浏览器复测确认本机 Chrome 无现存门户会话，匿名访问门户直接回 CAS 登录页。**登录后才返回的个人信息 JSON、教务/学工学籍字段、票据成功后的 302 与会话 Cookie 名，均需在已登录会话中补抓**（本轮无账号凭证，登录步骤需用户本人完成），见第 9 节。
>
> **合规红线 / Compliance**：个人信息一律由客户端在用户本人登录后按账号运行时拉取，**不得把学号、姓名、班级、学院、学籍号等真实个人信息硬编码进仓库或文档**；口令仅经系统密钥存储读取，不明文落盘。校徽/院徽等素材版权归学校，仅作客户端界面展示用途。

## 1. 站点体系 / Site Landscape

| 用途 / Purpose | 地址 / URL | 备注 / Notes |
|---|---|---|
| 学校主站 / main site | `https://www.gdei.edu.cn/` | 公开，校徽与机构列表在此 |
| 机构设置 / organization list | `https://www.gdei.edu.cn/1314/list.htm` | 教学机构、党政部门、群团、教辅、附属、其他六类 |
| 教学机构 / colleges | `https://www.gdei.edu.cn/jxkyjg/list.htm` | 18 个二级学院，列表项 `img-src` 为空，院徽需进各子站 |
| 统一认证 / CAS SSO | `https://security.gdei.edu.cn/cas/login?service=<回调>` | 各子系统共用同一认证中心 |
| 融合门户 / integrated portal | `https://iportal.gdei.edu.cn/` | 雷神（ly-sky）React + antd + axios 门户，CAS 回调 `/shiro-cas` |
| 学生数据门户 / data portal | `https://dsj.gdei.edu.cn/sjfx/eportal/pcportal/portal.do?id=MH_XS` | 未登录经 `/sso/ecas/login.do` → `/sso/sso/login` → 回 CAS，回调 `https://dsj.gdei.edu.cn/sso/sso/login`，数据按账号返回 |
| 教务系统 / academic system | `https://jwxt.gdei.edu.cn/gly/api/cas/login?pattern=teacher-login` | 新教务 gly 后端 API；CAS 回调即此地址，`pattern` 区分师生 |
| 教务选课 SSO 入口 / academic SSO entry | `https://security.gdei.edu.cn/cas/jwxtsso.jsp` | 主站导航入口，302 到 CAS 再回 jwxt 回调 |
| 图书馆 / library | `https://lib.gdei.edu.cn/` | 主站导航入口 |
| WebVPN（校外入口） | `https://webvpn.gdei.edu.cn/` | Vue SPA，校外访问校内系统的代理入口 |

## 2. CAS SSO 认证链路 / Authentication Flow

认证根为 `https://security.gdei.edu.cn/cas/`（实测响应头 `Server: GDEI`，容器 Apache Tomcat 7.0.75；域名与 `iportal.gdei.edu.cn` 同解析到 `210.38.64.57`）。

### 2.1 子系统回调（实测 302）/ Subsystem callbacks

未登录访问各子系统，均被 302 回统一 CAS，`service` 带回各自回调：

| 子系统 | CAS service 回调 | 后端会话端点 / 备注 |
|---|---|---|
| 融合门户 iportal | `https://iportal.gdei.edu.cn/shiro-cas` | Apache Shiro 接 CAS，会话靠 Cookie |
| 教务 jwxt | `https://jwxt.gdei.edu.cn/gly/api/cas/login?pattern=teacher-login` | gly 后端 API；`pattern` 区分师生 |
| 数据门户 dsj | `https://dsj.gdei.edu.cn/sso/sso/login` | 先经 `/sso/ecas/login.do` 二次封装，再回 CAS |

单点退出：`https://security.gdei.edu.cn/cas/login?service=https://iportal.gdei.edu.cn/shiro-cas`（门户 index.html 注入的 `window.casLogoutUrl`）。

### 2.2 登录表单链路（实测）/ Login form flow

1. **GET 外壳页** `https://security.gdei.edu.cn/cas/login?service=<回调编码>`：
   - 种 `JSESSIONID`（`Path=/cas`，HttpOnly）；
   - 返回约 1 KB 外壳 HTML，内含本次会话动态 `sign=<32 位十六进制>`（绑定会话，不可复用）。
2. **GET 真实表单** `https://security.gdei.edu.cn/cas/login2022?sign=<sign>&service=<编码回调>`：
   - **必须带** `X-Requested-With: XMLHttpRequest` 与 `Referer: <外壳页>`，否则返回 200 空体（jQuery `.load()` 默认行为）；
   - 返回 `login2022` 表单 HTML（约 13 KB）。
3. **POST 表单** `https://security.gdei.edu.cn/cas/login`，字段：

   | 字段 | 说明 |
   |---|---|
   | `username` | 用户名，前端 `toLocaleLowerCase()` 转小写 |
   | `password` | **JSEncrypt RSA / PKCS#1 v1.5 加密**后的 Base64 密文（非明文） |
   | `tokens` | 表单隐藏域，32 位十六进制，当次表单生成（CSRF/会话令牌） |
   | `stamp` | 秒级时间戳隐藏域，前端每秒自增 |
   | `service` | 回调 URL（页面单选共六项：门户 / 教务 / 邮件 / VPN / CMS / CAS；门户、教务、数据门户三套回调已实测见 2.1，邮件 / VPN / CMS 回调未逐一探测） |
   | `renew` | 隐藏固定 `true` |
   | `txtUserName` / `txtPasswd` | 隐藏域，仅 VPN 分支使用 |

   - **无图形验证码**（表单与 `login.js` 均无验证码字段；`login.js` 只做焦点/尺寸适配）。
   - 登录页静态提示（2026-11-26 浏览器复测可见）：「请使用校园网账号登录，不详请查账号」「新用户初始密码为身份证号码后 8 位数字」「若遗失密码，可通过手机短信自助修改密码」；页面标题「统一身份认证系统 / 校园网账号登录」，横幅为广东第二师范学院校名。
4. 认证成功后 CAS 302 回跳 `service?ticket=ST-xxxx`；子系统回调端点内部校验 ticket 并建立本地会话 Cookie（**成功后的 Location、ticket 形态、Shiro Cookie 名待登录环境抓包**）。
   - 复测：本机 Chrome profile 无有效门户会话，匿名访问 `https://iportal.gdei.edu.cn/` 直接被 302 到 `https://security.gdei.edu.cn/cas/login?service=https://iportal.gdei.edu.cn/shiro-cas`（地址栏 `service` 为明文未编码），再次印证未登录即回 CAS。
5. 会话建立后，门户前端携带会话 Cookie 调后端接口拉取个人信息与卡片。

### 2.3 票据校验端点（实测 404）/ Ticket validation endpoints

以下标准 CAS 服务端校验端点在本机匿名探测**全部 404（Tomcat 错误页）**，未对外暴露：

- `/cas/serviceValidate`、`/cas/p3/serviceValidate`
- `/cas/proxyValidate`、`/cas/p3/proxyValidate`
- `/cas/validate`、`/cas/p3/validate`

> **关键结论**：本套部署**不开放标准 CAS 服务端票据校验接口**，桌面客户端**不能**像标准 CAS 客户端那样拿 ticket 调 `serviceValidate` 换取 XML 用户属性。要拿到门户会话与个人信息，必须模拟浏览器走完整登录链（外壳 → 表单 → POST → 跟随 302 回调建会话 → 调接口），并用 CookieJar 全程保留 Cookie、跟随 302，不要自行拼装 ticket。

### 2.4 RSA 公钥与辅助接口 / RSA keys & aux endpoints

- **表单内嵌公钥**：`isSubmit()` 主分支使用一把内嵌 1024 位公钥（`MIGfMA0...QIDAQAB`，完整值见登录表单 HTML）。
- **动态公钥接口**：`POST/GET https://security.gdei.edu.cn/user/inc/api?opt=getkey`，返回另一把 1024 位 Base64 公钥（实测每次会话可能不同）；`getRSAKey()` 仅在弱口令校验分支 `goModWeak()` 使用。**主登录提交到底用内嵌公钥还是动态公钥，待登录抓包复核。**
- **校验/弱口令接口**：`https://security.gdei.edu.cn/user/inc/api`，`opt=auth&uid=<账号>&pwd=<RSA 密文>`，返回：`1` 账号存在弱口令 / `10` 账密错误 / `5` 多次错误锁定 5 分钟 / `99` 操作超时。
- **账号自助页**：`/user/query`（查用户名）、`/user/reset`（短信重置）、`/user/modify`（改密）。
- 弱口令：纯数字或纯字母会弹窗提醒并跳转改密。

> 标准 CAS 票据 `ST` 为一次性、短时有效；客户端对接时应维护与浏览器一致的 CookieJar，跟随 302，不要自行拼装 ticket。

## 3. 融合门户后端接口（前端 bundle 静态分析）/ Portal APIs

以下路径提取自 `iportal.gdei.edu.cn/assets/js/app.3c2.js`，相对门户同源（无独立 baseURL）；**响应结构未鉴权实测**，登录后需复核。

**鉴权方式（bundle 实测）**：axios 实例 `withCredentials=true`，默认请求头 `X-Requested-With: XMLHttpRequest`（GET/POST/DELETE），POST 为表单编码；**会话靠 Shiro 会话 Cookie，不发 `Authorization` Bearer**。响应拦截器把 `data.meta.status == 302` 判为未登录。匿名调用被 SPA 兜底回首页 `index.html`，拿不到任何数据。

### 3.0 部门/头像等关联接口 / Lookup endpoints

| 接口 / Endpoint | 方法 | 用途 / Purpose |
|---|---|---|
| `/api/authc/departments` | GET | 部门/学院列表，按 `departmentId` 关联出 `departmentName` |
| `/api/authc/departments/commonDepartmentsTree` | GET | 部门树 |
| `api/authc/users/portraitChange` | POST | 更换头像（头像字段 `portrait` / `avatar`） |

### 3.1 登录用户信息 / current user

| 接口 / Endpoint | 方法 | 用途 / Purpose |
|---|---|---|
| `api/upp/userControl/getLoginInfo` | GET | 当前登录用户资料（核心个人信息来源） |
| `api/upp/userControl/guideHasUsed` | POST | 新手引导是否已使用 |
| `/api/authc/users/menus` | GET | 当前用户菜单权限 |
| `/api/authc/users/<id>/authority` | GET | 指定用户权限 |
| `/api/authc/users/selector` | GET | 人员选择器查询 |

### 3.2 其他已识别模块（供后续对接参考）/ other modules

| 前缀 / Prefix | 模块 / Module |
|---|---|
| `api/upp/appStore/` | 个人应用收藏 |
| `api/upp/theme/` | 门户主题/布局 |
| `api/upp/attachmentManagement` | 附件上传/删除 |
| `api/uppinfo/infoCenter/` | 资讯中心 |
| `api/uppmessage/message/` | 消息中心 |
| `api/uppflow/process/` | 待办流程 |
| `api/uppcard/affairCenter/` | 办事中心 |
| `api/bs-schedule/` | 日程 |
| `api/uppemail/` | 邮箱卡片 |

## 4. 个人信息字段分层 / Personal Fields by Source

### 4.1 融合门户侧（bundle 消费 `getLoginInfo` 响应对象读取的 key，待登录 JSON 复核）

bundle 中资料上报/展示对象从响应 `d` 读取：`d.userName`、`d.userNickname`、`d.userType`、`d.host`、`d.email`、`d.departmentId`。

| 前端 key | 含义 / Meaning | 说明 / Notes |
|---|---|---|
| `userName` | 登录账号 | 学生一般为学号，教师为工号；待登录 JSON 复核 |
| `userNickname` | 姓名 / 昵称 | 展示名 |
| `userType` | 用户类型 | 学生 / 教师等分类（上报映射为 `operaCategory`） |
| `email` | 邮箱 | |
| `departmentId` | 部门 / 学院 ID | 响应对象只带 ID；部门名需调 `/api/authc/departments` 按 ID 关联 |
| `host` | 登录 IP | 登录日志用 |
| 头像 | `portrait` / `avatar` | 配合 `portraitChange` 接口；头像 URL 与鉴权待复核 |

> bundle 中**未出现** `className`、`grade`、`major`、`studentNo`、`collegeName`、`realName`、`jobNumber`、`loginName` 等学籍字段。

### 4.2 教务 / 学工侧（门户前端未出现，需另抓）/ academic & student-affairs

> 以下字段不在融合门户前端 bundle 中，预期位于教务系统（jwxt SSO）或 dsj 学生数据门户，**需登录后在对应页面抓包确认字段名与来源接口**：

| 目标字段 / Target field | 预期来源 / Likely source | 状态 / Status |
|---|---|---|
| 姓名 | 门户 `userNickname` 已可取；教务侧待复核 | 门户侧见 4.1 |
| 学号 | 门户 `userName` 疑为学号，待复核；规范学号以教务为准 | 待抓 |
| 班级 | 教务 / 学工（jwxt 或 dsj） | 待抓 |
| 学院（规范全称） | 门户部门 ID → 部门列表关联；教务侧待复核 | 部分可由门户补 |
| 专业 | 教务系统 jwxt | 待抓 |
| 年级 / 年级专业班 | 教务 / 学工 | 待抓 |
| 学籍号 | 学工 / 教务 | 待抓 |
| 学籍状态（在读/休学等） | 学工 | 待抓 |

## 5. 访问环境要求 / Access Environment

- **校内**：连接 `gdei-wifi` 或校内有线网，CAS 与各子系统可直连。
- **校外**：先登录 `https://webvpn.gdei.edu.cn/`，由 WebVPN 代理访问校内系统；客户端若要支持校外，需先研究 WebVPN 的认证与代理转发机制。
  - 前端为 Vue SPA，已识别路由：`/auth/login`、`/auth/local`（本地账号）、`/auth/tfa`（双因素）、`/auth/bind-email`、`/auth/bind-mobile`、`/auth/bind-otp`、`/auth/bind-wechat-official`、`/auth/change-password`、`/auth/reset-password`。
  - 认证请求封装在共享 axios chunk，后端真实端点、字段与代理 URL 改写规则待登录环境抓包确认。
- 未登录状态下，门户与数据门户接口返回 302 跳 CAS；匿名调用拿不到任何个人数据。

## 6. 实现约束（对接本项目时）/ Implementation Constraints

- **不能复刻标准 CAS 客户端**：`serviceValidate`/`proxyValidate` 等校验端点 404 不开放，客户端无法用 ticket 换 XML 用户属性；必须走浏览器式完整登录链（外壳 → login2022 表单 → POST `/cas/login` → 跟随 302 回调建会话 → 调资料接口）。
- 认证复用既有 `SessionCookieJar` 思路：完整保留 CAS → 子系统回调链路上的 Cookie，跟随 302，不跨域丢失票据；门户接口请求带 `X-Requested-With: XMLHttpRequest`、`withCredentials` 携 Cookie。
- 口令需 RSA（JSEncrypt / PKCS#1 v1.5）加密后提交；`tokens`、`stamp` 必须从当次登录表单提取，不可跨会话复用；`username` 转小写。
- 个人信息模型按“账号登录后拉取”设计：本地不持久化他人或历史个人信息；展示字段以登录后接口返回为准，前端做缺省兜底。
- 口令不入仓库、不写日志、不进截图；敏感字段沿用既有日志脱敏策略。
- **信任锚边界**：NAC 门户仍是校园网入网认证与设备在线状态的信任根；CAS/iportal 仅用于在用户已入网、本人登录后**补全展示用账号资料**（姓名/部门等），不替代 NAC 入网认证，也不在离线/未入网时凭缓存资料对外鉴权。
- 字段名以真实接口 JSON 为准，本文 4.1 的 key 为前端消费侧静态推断，接入前需在登录环境逐项核对。

## 7. 素材清单 / Assets

| 素材 / Asset | 本地路径 / Local path | 来源 / Source |
|---|---|---|
| 横版校徽 + 校名（SVG，内嵌位图） | `docs/assets/school/gdei-logo.svg` | `https://www.gdei.edu.cn/_upload/tpl/01/3a/314/template314/images/logo.svg` |
| 站点图标（ICO） | `docs/assets/school/favicon.ico` | `https://www.gdei.edu.cn/_upload/tpl/01/3a/314/template314/favicon.ico` |
| 校徽绿色横版（位图） | `docs/assets/school/colleges/logo-green.png` | 各学院站 `/_upload/site/.../logo.png`（组 A） |
| 校徽白色横版（位图，深底用） | `docs/assets/school/colleges/logo-white.png` | 各学院站 `/_upload/site/.../logo.png`（组 B） |
| 校徽横版矢量模板 | `docs/assets/school/colleges/chemistry.svg` | `/_upload/tpl/01/4f/335/template335/images/logo.svg` |
| 学院—子站—logo 映射清单 | `docs/assets/school/colleges/colleges.json` | 逐站首页 `Site_Logo` 块解析 |

**院徽 / 机构 logo 结论**：18 个二级学院站、抽样的党政部门站（教务部 `jwc`、财务部 `cwc`、学生工作部 `xsc`）头部均沿用主站校徽，**未发现独立院徽或机构标识**；差异仅为绿版/白版/缩略裁切变体，位图按哈希去重，未在仓库内重复存放同一张图。各学院原始 logo URL 与变体类型见 `colleges.json`；其余群团/教辅/附属/其他机构为同一套 CMS 模板，预期同为校徽变体，如需子站 URL 清单可再批量解析。

## 8. 二级学院清单（教学机构）/ College List

来自 `jxkyjg/list.htm`；`/_redirect?siteId=4&columnId=1389&articleId=<id>` 已逐一解析为真实子站路径，完整子站 URL、原始 logo URL 与变体类型见 `docs/assets/school/colleges/colleges.json`。

广东学前教育学院、计算机与人工智能学院、教师教育学院、历史与社会发展学院、教育学院、美术学院、文学院、音乐学院、数学学院、教师发展学院、外国语言文化学院、继续教育学院、物理与信息工程学院、化学与材料科学学院、马克思主义学院、生物与食品工程学院、管理学院、体育学院。

## 9. 待登录环境补全清单 / TODO After Login

**2026-11-26 匿名已实测（无需再抓）**：CAS 外壳/sign/JSESSIONID 链路、`login2022` 表单字段、RSA 加密与隐藏域、无图形验证码、标准校验端点 404、三套子系统 302 回调、门户 axios 鉴权方式（Cookie + AJAX 头）、bundle 资料字段 key 与缺失字段。

**仍需已登录会话补抓**：

1. **登录成功后的票据与会话**：POST `/cas/login` 成功 302 的完整 Location、`ticket` 形态与有效期；回调 `/shiro-cas` 建立的会话 Cookie 名/属性；jwxt、dsj 回调各自的会话 Cookie。
2. **门户个人信息 JSON**：登录后调 `api/upp/userControl/getLoginInfo`，记录真实 JSON 全字段名与类型，校准第 4.1 节；确认 `departmentName` 是否直返、`userName` 是否即学号、`userType` 取值枚举、头像 URL 拼装与鉴权。
3. **主登录 RSA 公钥选择**：复核主提交用表单内嵌公钥还是 `/user/inc/api?opt=getkey` 动态公钥；记录密文编码（Base64）与是否需 `stamp`/`tokens` 服务端时效校验。
4. **教务 / 学工字段**：在 jwxt（`/gly/api/cas/login` 回调）与 dsj 学生门户（`id=MH_XS`）抓学号、班级、学院、专业、年级、学籍号、学籍状态对应的接口路径与返回结构。
5. **头像接口**：确认头像字段来源 URL 与鉴权方式。
6. **WebVPN**：分析其登录接口与代理 URL 改写规则，判断客户端校外可达性方案。

> 院徽采集已完成（见第 7、8 节）：18 个学院站均无独立院徽，头部沿用校徽变体，映射见 `colleges.json`。
