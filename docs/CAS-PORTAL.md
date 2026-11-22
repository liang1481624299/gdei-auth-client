# CAS 统一认证与个人门户对接调研 / CAS & Personal Portal Notes

> 来源：2026-11-22 对公网可访问页面、门户前端 bundle（React/axios）的静态分析。
> Source: public-facing pages and the portal frontend bundle analyzed from the public network.
>
> **环境限制 / Environment caveat**：本次调研在**校外公网**进行。CAS 登录子页 `login2022` 对校外 IP 返回“系统维护期间暂停校外访问，需连接 gdei-wifi 或 WebVPN”提示，因此**登录表单字段、口令加密方式、验证码机制，以及登录后才返回的个人信息 JSON 均未在本次实测**，需在校内网（gdei-wifi）或已登录 WebVPN 会话中补抓，见第 9 节。
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
| 学生数据门户 / data portal | `https://dsj.gdei.edu.cn/sjfx/eportal/pcportal/portal.do?id=MH_XS` | 未登录跳 CAS，回调 `https://dsj.gdei.edu.cn/sso/sso/login`，数据按账号返回 |
| 教务选课 SSO / academic SSO | `https://security.gdei.edu.cn/cas/jwxtsso.jsp` | 主站导航入口，教务系统票据跳转 |
| 图书馆 / library | `https://lib.gdei.edu.cn/` | 主站导航入口 |
| WebVPN（校外入口） | `https://webvpn.gdei.edu.cn/` | Vue SPA，校外访问校内系统的代理入口 |

## 2. CAS SSO 认证链路 / Authentication Flow

基于门户前端配置与未登录跳转行为推断（标准 CAS 3.0 + Shiro 回调）：

1. **未登录访问子系统** → 浏览器被重定向到 CAS：
   `https://security.gdei.edu.cn/cas/login?service=<子系统编码后的回调地址>`。
2. CAS 外壳页（1 KB 左右）通过 jQuery 异步加载真实表单：
   `login2022?sign=<会话十六进制签名>&service=<编码后的回调>`；
   `sign` 每次取登录外壳页时动态生成，不可复用。
3. 用户在 `login2022` 表单提交账号口令（**表单字段、加密、验证码待登录环境补抓**）。
4. 认证成功后 CAS 302 回跳 `service?ticket=ST-xxxx-xxxx`。
5. 子系统回调端点校验 ticket 并建立本地会话 cookie：
   - 融合门户：`/shiro-cas`（Apache Shiro 接 CAS）；
   - 数据门户：`/sso/sso/login`；
   - 教务：经 `/cas/jwxtsso.jsp` 转发。
6. 会话建立后，门户前端携带会话 cookie 调后端接口拉取个人信息与卡片。
7. 单点退出：`https://security.gdei.edu.cn/cas/login?service=https://iportal.gdei.edu.cn/shiro-cas`（门户 `casLogoutUrl` 配置）。

> 标准 CAS 票据 `ST` 为一次性、短时有效；客户端对接时应维护与浏览器一致的 CookieJar，跟随 302，不要自行拼装 ticket。

## 3. 融合门户后端接口（前端 bundle 静态分析）/ Portal APIs

以下路径提取自 `iportal.gdei.edu.cn/assets/js/app.3c2.js`，相对门户同源；**响应结构未鉴权实测**，登录后需复核。

### 3.1 登录用户信息 / current user

| 接口 / Endpoint | 方法 | 用途 / Purpose |
|---|---|---|
| `api/upp/userControl/getLoginInfo` | GET | 当前登录用户资料（核心个人信息来源） |
| `api/upp/userControl/guideHasUsed` | POST | 新手引导是否已使用 |
| `/api/authc/users/menus` | GET | 当前用户菜单权限 |
| `/api/authc/users/<id>/authority` | GET | 指定用户权限 |
| `/api/authc/users/selector` | GET | 人员选择器查询 |
| `api/authc/users/portraitChange` | POST | 更换头像 |
| `/api/authc/departments` | GET | 部门/学院列表 |
| `/api/authc/departments/commonDepartmentsTree` | GET | 部门树 |

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

### 4.1 融合门户侧（bundle 中已出现的 key，待登录 JSON 复核）

| 前端 key | 含义 / Meaning | 说明 / Notes |
|---|---|---|
| `userName` | 登录账号 | 学生一般为学号，教师为工号；待复核 |
| `userNickname` | 姓名 / 昵称 | 展示名 |
| `userType` | 用户类型 | 学生 / 教师等分类 |
| `email` | 邮箱 | |
| `departmentId` | 部门 / 学院 ID | |
| `departmentName` | 部门 / 学院名 | 由部门列表按 ID 关联得到 |
| `host` | 登录 IP | 登录日志用 |
| 头像 | `portrait` / `avatar` | 配合 `portraitChange` 接口 |

### 4.2 教务 / 学工侧（门户前端未出现，需另抓）/ academic & student-affairs

> 以下字段不在融合门户前端 bundle 中，预期位于教务系统（jwxt SSO）或 dsj 学生数据门户，**需登录后在对应页面抓包确认字段名与来源接口**：

| 目标字段 / Target field | 预期来源 / Likely source | 状态 / Status |
|---|---|---|
| 姓名 | 教务 / 学工 / 门户 | 门户侧见 4.1，教务侧待复核 |
| 学号 | 教务系统 | 待抓 |
| 班级 | 教务 / 学工 | 待抓 |
| 学院（规范全称） | 教务 / 部门列表 | 待抓 |
| 专业 | 教务系统 | 待抓 |
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

- 认证复用既有 `SessionCookieJar` 思路：完整保留 CAS → 子系统回调链路上的 Cookie，跟随 302，不跨域丢失票据。
- 个人信息模型按“账号登录后拉取”设计：本地不持久化他人或历史个人信息；展示字段以登录后接口返回为准，前端做缺省兜底。
- 口令不入仓库、不写日志、不进截图；敏感字段沿用既有日志脱敏策略。
- 字段名以真实接口 JSON 为准，本文 4.1 的 key 仅为前端静态推断，接入前需在登录环境逐项核对。

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

1. **CAS 登录表单**：抓 `login2022` 真实表单字段（账号/口令/验证码/隐藏域）、口令是否前端加密（Base64/RSA/自定义）、验证码接口与校验流程。
2. **门户个人信息**：登录后调 `api/upp/userControl/getLoginInfo`，记录真实 JSON 字段名与类型，校准第 4.1 节。
3. **教务 / 学工字段**：在教务系统与 dsj 学生门户（`id=MH_XS`）抓学号、班级、学院、专业、年级、学籍号、学籍状态对应的接口路径与返回结构。
4. **WebVPN**：分析其登录接口与代理 URL 改写规则，判断客户端校外可达性方案。
5. **头像接口**：确认头像字段来源 URL 与鉴权方式。

> 院徽采集已完成（见第 7、8 节）：18 个学院站均无独立院徽，头部沿用校徽变体，映射见 `colleges.json`。
