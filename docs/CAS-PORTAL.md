# CAS 统一认证与个人门户对接调研 / CAS & Personal Portal Notes

> 来源：2026-11-22 对公网可访问页面、门户前端 bundle（React/axios）的静态分析；2026-11-26 在可达统一认证中心的网络下**实测匿名链路**（外壳页、login2022 表单、校验端点、子系统 302 跳转、前端 bundle）；同日在**本人已登录门户会话**内复核了票据跳转链、会话 Cookie 与 `sessionStorage.userLogin` 全字段。
> Source: public pages + portal bundle static analysis (2026-11-22); anonymous live probing (2026-11-26); logged-in session verification of ticket flow, session cookies and the merged `userLogin` object (2026-11-26).
> 最后核对 / Last reviewed：本地 CST(UTC+8) `2026-11-26 23:00:00`。
>
> **登录后实测口径 / Logged-in scope**：CAS 登录链、会话 Cookie、门户 `userLogin` 资料字段已在本人账号登录会话中实测核对（字段名/类型/枚举见第 4.1 节，文档不落真实姓名学号等值）；jwxt / dsj 学籍字段已在早先成功进入子系统的会话中抓到（见 4.2、4.3）。jwxt 网关鉴权头目前为前端 bundle 静态推断，原始成功 XHR 头待可进入学生教务的会话复核（见 4.2、§9-1）。
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
| 学生数据门户 / data portal | `https://dsj.gdei.edu.cn/sjfx/eportal/pcportal/portal.do?id=MH_XS` | 已登录会话实测免密进入「个人画像」；报表型 `.do` 端点（4.3） |
| 教务系统 jwxt | `https://jwxt.gdei.edu.cn/gly/api/cas/login?pattern=teacher-login` | gly 后端经 `ly-api-gateway` 网关；学籍字段见 4.2 |
| 教务选课 SSO 入口 / academic SSO entry | `https://security.gdei.edu.cn/cas/jwxtsso.jsp` | 已实测：CAS 有 TGC 时凭单点票据免密跳转 jwxt |
| 图书馆 / library | `https://lib.gdei.edu.cn/` | 主站导航入口 |
| WebVPN（校外入口） | `https://webvpn.gdei.edu.cn/` | Vue SPA，校外访问校内系统的代理入口 |

## 2. CAS SSO 认证链路 / Authentication Flow

认证根为 `https://security.gdei.edu.cn/cas/`（实测响应头 `Server: GDEI`，容器 Apache Tomcat 7.0.75；域名与 `iportal.gdei.edu.cn` 同解析到 `210.38.64.57`）。

### 2.1 子系统回调（实测 302）/ Subsystem callbacks

未登录访问各子系统，均被 302 回统一 CAS，`service` 带回各自回调：

| 子系统 | CAS service 回调 | 后端会话端点 / 备注 |
|---|---|---|
| 融合门户 iportal | `https://iportal.gdei.edu.cn/shiro-cas` | Apache Shiro 接 CAS；实测 ticket 回调先落到 `http://iportal.gdei.edu.cn?ticket=ST-...`，nginx 301 到 `https://` 后由 SPA 落地 `#/StudentHomepage`，会话靠 Cookie / 令牌 |
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
   | `service` | 回调 URL（登录页 radio 实测共七项，见下「登录目标选项」；门户、教务、数据门户三套回调已实测见 2.1，邮件 / VPN / CMS 回调未逐一探测） |
   | `renew` | 隐藏固定 `true` |
   | `txtUserName` / `txtPasswd` | 隐藏域，仅 VPN 分支使用 |

   - **无图形验证码**（表单与 `login.js` 均无验证码字段；`login.js` 只做焦点/尺寸适配）。
   - 登录页静态提示（2026-11-26 浏览器复测可见）：「请使用校园网账号登录，不详请查账号」「新用户初始密码为身份证号码后 8 位数字」「若遗失密码，可通过手机短信自助修改密码」；页面标题「统一身份认证系统 / 校园网账号登录」，横幅为广东第二师范学院校名。
4. **认证成功跳转（浏览器实测）**：POST `/cas/login` 成功返回 **200**（不是 302），响应体是一段 JS：`window.location.href="<cas根>/cas/login?service=<回调>&ticket=ST-..."`；**成功跳转 Location 在 body 的 JS 里，不在 302 响应头**。随后二次跳转：GET 该带 ticket 的 CAS 地址 → body JS 再跳到 `http://iportal.gdei.edu.cn?ticket=ST-...`（CAS 在此换发第二个 ST）；`http://` 经 nginx 301 到 `https://`，门户 SPA 落地 `https://iportal.gdei.edu.cn/#/StudentHomepage`。
   - ticket 形态：`ST-<序号>-<随机串>`，一次性、短时有效；实测同一会话 CAS 换发两张 ST（`ST-21xx-...` 序号递增）。
   - 复测：本机 Chrome profile 无有效门户会话，匿名访问 `https://iportal.gdei.edu.cn/` 直接被 302 到 `https://security.gdei.edu.cn/cas/login?service=https://iportal.gdei.edu.cn/shiro-cas`（地址栏 `service` 为明文未编码），再次印证未登录即回 CAS。
5. 会话建立后，门户前端携带会话 Cookie / 令牌调后端接口拉取个人信息与卡片（见第 3 节）。

**登录目标选项（radio value 实测七项）**：

| 选项 | radio value / service |
|---|---|
| 门户 | `https://security.gdei.edu.cn/cas/login?service=http://iportal.gdei.edu.cn` |
| 旧教务 | `http://jwgl.gdei.edu.cn/login_cas.aspx` |
| 教务 SSO | `https://security.gdei.edu.cn/cas/jwxtsso.jsp` |
| 邮件 | `https://security.gdei.edu.cn/cas/mailsso.jsp` |
| VPN | `https://webvpn.gdei.edu.cn` |
| CMS | `https://cms.webvpn.gdei.edu.cn` |
| CAS 自身 | 空（无 service） |

### 2.3 票据校验端点（实测 404）/ Ticket validation endpoints

以下标准 CAS 服务端校验端点在本机匿名探测**全部 404（Tomcat 错误页）**，未对外暴露：

- `/cas/serviceValidate`、`/cas/p3/serviceValidate`
- `/cas/proxyValidate`、`/cas/p3/proxyValidate`
- `/cas/validate`、`/cas/p3/validate`

> **关键结论**：本套部署**不开放标准 CAS 服务端票据校验接口**，桌面客户端**不能**像标准 CAS 客户端那样拿 ticket 调 `serviceValidate` 换取 XML 用户属性。要拿到门户会话与个人信息，必须模拟浏览器走完整登录链（外壳 → 表单 → POST → 跟随 302 回调建会话 → 调接口），并用 CookieJar 全程保留 Cookie、跟随 302，不要自行拼装 ticket。

### 2.4 RSA 公钥与辅助接口 / RSA keys & aux endpoints

- **表单内嵌公钥（主登录已确认）**：`isSubmit()` 主分支使用登录表单内嵌的一把 1024 位公钥（`MIGfMA0...QIDAQAB`，完整值见登录表单 HTML），JSEncrypt RSA / PKCS#1 v1.5 → Base64。
- **动态公钥接口（仅弱口令分支）**：`POST/GET https://security.gdei.edu.cn/user/inc/api?opt=getkey`，返回另一把 1024 位 Base64 公钥；`getRSAKey()` 仅在弱口令校验分支 `goModWeak()` 使用，**主登录提交不走此接口**（已确认）。
- **校验/弱口令接口**：`https://security.gdei.edu.cn/user/inc/api`，`opt=auth&uid=<账号>&pwd=<RSA 密文>`，返回：`1` 账号存在弱口令 / `10` 账密错误 / `5` 多次错误锁定 5 分钟 / `99` 操作超时。
- **账号自助页**：`/user/query`（查用户名）、`/user/reset`（短信重置）、`/user/modify`（改密）。
- 弱口令：纯数字或纯字母会弹窗提醒并跳转改密。

> 标准 CAS 票据 `ST` 为一次性、短时有效；客户端对接时应维护与浏览器一致的 CookieJar，跟随 302，不要自行拼装 ticket。

## 3. 融合门户后端接口（前端 bundle 静态分析）/ Portal APIs

以下路径提取自 `iportal.gdei.edu.cn/assets/js/app.3c2.js`，相对门户同源（无独立 baseURL）；**响应结构未鉴权实测**，登录后需复核。

**鉴权方式（bundle + 登录会话实测）**：axios 实例 `withCredentials=true`，默认请求头 `X-Requested-With: XMLHttpRequest`（GET/POST/DELETE），POST 默认 `application/x-www-form-urlencoded`。请求拦截器：`r = Date.now()`（毫秒时间戳），`csrfToken = 哈希("timestamp="+r+",key="+GATEWAY_KEY)`（GET/POST 均带 `csrfToken`，另带 `csrfTimestamp=r` 头；`GATEWAY_KEY` 为前端配置内签名密钥，文档不落其值，摘要哈希算法待复核）；登录后另带 `loginUserId=<userId>` 头。登录会话实测：当前域 JS 可见 Cookie 仅 `Authorization`（值为门户内部令牌），后端另有 Shiro HttpOnly 会话 Cookie（`document.cookie` 读不到）；主资料接口 `tryLoginUserInfo` 在请求头带 `Authorization: <令牌>` 与 `gatewayAppId: ly-upp`。响应拦截器把 `data.meta.status == 302` / 401 判为未登录。匿名调用被 SPA 兜底回首页 `index.html`，拿不到任何数据。

### 3.0 部门/头像等关联接口 / Lookup endpoints

| 接口 / Endpoint | 方法 | 用途 / Purpose |
|---|---|---|
| `/api/authc/departments` | GET | 部门/学院列表，按 `departmentId` 关联部门名。**学生账号实测无权限**：缺 `pageSize` 返 400 `MissingServletRequestParameterException`；带 `pageSize=200&pageNum=1` 返 500「超出数据权限的操作」，普通学生拉不到部门全量，学院全称无法靠此接口关联 |
| `/api/authc/departments/commonDepartmentsTree` | GET | 部门树（预期同受数据权限限制，待复核） |
| `api/authc/users/portraitChange` | POST | 更换头像（头像字段 `portrait` / `avatar`） |

### 3.1 登录用户信息 / current user

| 接口 / Endpoint | 方法 | 用途 / Purpose |
|---|---|---|
| `tryLoginUserInfo` | POST（或由 `window.config.tryLoginMethod` 指定的 GET） | **主资料接口**：CAS 回调建立会话后前端首个用户接口，返回完整资料对象（`userId`/`userName`/`departmentName`/`departmentId`/`userType`/`sex` 等 46 字段，见 4.1），整体写入 `sessionStorage.userLogin`；请求头带 `Authorization` 令牌与 `gatewayAppId: ly-upp` |
| `api/upp/userControl/getLoginInfo` | GET | 补充接口：实测响应 `data` 仅 `userId`、`firLogin`、`guideUsed`、`headPortrait`、`strategy`；其中 `strategy`、`headPortrait` 合并回 `userLogin`，`guideUsed`/`firLogin` 驱动新手引导与首次登录分支 |
| `api/upp/userControl/guideHasUsed` | POST | 新手引导是否已使用 |
| `/api/authc/users/menus` | GET | 当前用户菜单权限 |
| `/api/authc/users/<id>/authority` | GET | 指定用户权限 |
| `/api/authc/users/selector` | GET | 人员选择器查询 |

> 调用顺序（bundle 实测）：会话有效时先读本地 `userLogin`；无会话则先调 `tryLoginUserInfo` 落主资料，再调 `getLoginInfo` 合并 `strategy`/`headPortrait`/`guideUsed`/`firLogin`，最后调 `onlineUsers/add` 上报当前浏览环境（`userName`/`userType`/`nickName`/`host`/`departmentId`/浏览器/屏幕/登录类别等）。

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

### 4.1 融合门户侧（登录会话实测，`sessionStorage.userLogin` 合并对象）

登录后资料对象整体写入 `sessionStorage.userLogin`（实测共 46 个 key）。前端上报/展示从该对象读取：`userName`、`userType`、`host`、`email`、`departmentId`、`departmentName`、`headPortrait` 等；`strategy`、`headPortrait` 由 `getLoginInfo` 补充合并。

**实测有值字段（本人学生账号样本；文档只记含义与枚举，不落真实值）**：

| 前端 key | 含义 / Meaning | 实测口径 / Notes |
|---|---|---|
| `userId` | **学号** | 学生侧 `userId` 即学号（注意：不是 `userName`）；SSE 心跳头 `LoginUserId` 也取此字段 |
| `userName` | **姓名** | 真实姓名展示名 |
| `departmentName` | **班级** | 学生侧部门字段被复用为班级，文本已拼接「年级 + 专业方向 + 班」；门户不另拆独立年级/专业字段 |
| `departmentId` | 班级 / 部门 ID | 字符串 ID；学院全称无法靠 `departments` 接口关联（学生无权限，见 3.0） |
| `userType` | 用户类型 | 实测 `student`（教师预期 `teacher`）；上报映射为 `operaCategory` |
| `sex` | 性别 | 实测 `"1"` 男，其余值按女处理；仅用于无头像时男/女默认头像兜底 |
| `host` | 登录来源标记 | 实测 `cas-host`；上报映射为 `ip`（并非真实 IP） |
| `strategy` | 登录策略标记 | 实测 `"0"`；驱动消息提醒分支 |
| `sortNum` | 排序号 | 实测 `"0"` |
| `headPortrait` | 头像 | **`data:image/jpeg;base64,...` 内嵌 data URI**（非 URL 字段）；由 `getLoginInfo` 合并 |
| `attribute` | 扩展属性 | 实测空对象 `{}` |
| `tokenId` | 门户内部令牌 | JWT 样式长串，前端保存；另有 `accessToken`/`refreshToken`/`authorization` 字段，用途与是否需客户端复用待复核 |

**实测为 `null` / 空串的字段（学生账号未填充）**：

| 前端 key | 含义 / Meaning | 实测值 |
|---|---|---|
| `userNickname` | 昵称 | `null`（昵称未启用，展示名用 `userName`） |
| `email` / `qq` | 邮箱 / QQ | `null` |
| `kindCode` / `studentUrl` / `teacherUrl` | 类别码 / 学生·教师主页链接 | `null` |
| `orgId` | 机构 ID | `-1`（关联名字段未启用） |
| `typeName` / `sessionId` / `status` / `onlineTicket` / `loginId` / `id` | 类型名 / 会话 / 状态 / 在线票据 / 登录 ID / 主键 | `null` |
| `studentJobNumber` / `candidateNumber` / `examinationCardNumber` / `admissionNumber` / `idcard` / `phone` | 工号 / 考生号 / 准考证号 / 录取号 / 身份证 / 手机 | 空串 `""` |
| `runAs` | 模拟身份 | `null` |

**其余保留键（未逐一核对取值）**：`firstLogin`、`skin`、`startTimestamp`、`lastAccessTime`、`lastLoginTime`、`menus`、`apps`、`userSelectionList`、`dataAuthRelation`、`languageType`、`accessToken`、`refreshToken`、`authorization`、`fromUserId`。

> 字段缺口结论：门户 `userLogin` **未出现**独立的 `className`、`grade`、`major`、`collegeName`（学院全称）、`realName`、`jobNumber`、`mobile`、`idCard`、`birthday` 等学籍/身份字段；班级文本已含年级/专业/班拼接，但学院规范全称、专业、年级、学籍号、学籍状态需走教务 jwxt 或 dsj（见 4.2）。

### 4.2 教务系统 jwxt（登录会话实测）/ academic system

CAS 已有 TGC 单点会话时，访问教务 SSO 入口 `https://security.gdei.edu.cn/cas/jwxtsso.jsp` 可**免密**跳到教务 SPA `https://jwxt.gdei.edu.cn/#/jwxt/js/student/...`。教务后端经网关 `X-Application-Context: ly-api-gateway:10008`，响应统一 `{code:200, data:{...}}`。

**主资料接口**：`GET /api/studentInfo/selectMyjwByid/<学生内部ID>`（`xsid`），页面路由 `#/jwxt/js/student/childServiceQuery/myStudentInfo`。
> 鉴权注意：裸 `fetch` 重放缺网关鉴权头会返 `code:5100000「系统异常」`。

**网关鉴权 / 请求头（前端 bundle `jwxt.gdei.edu.cn/assets/js/app.3f76b.js` 静态分析）**：

| 项 | 口径 / Notes |
|---|---|
| 会话凭证 | **后端 CAS 回调后 Set-Cookie 下发的会话 Cookie** 为主要鉴权凭证；axios 实例 `withCredentials=true` 自动携同域 Cookie。前端拦截器**未**注入 `Authorization` 令牌头 |
| `X-Requested-With` | 默认 `XMLHttpRequest`（GET/POST/PUT/DELETE 均带） |
| `csrfToken` 头 | 请求拦截器生成：`_t = Date.now()/1000`（秒级时间戳，同时作为 query 参数 `_t`）；`csrfToken = MD5(_t + tokenKey)`，`tokenKey="lyedu"`（配置 `tokenKey:"lyedu"`）；MD5 为标准十六进制摘要（bundle 内联 MD5 实现，初始向量 `0x67452301` 等可辨） |
| 本地登录态 | 会话信息存 `sessionStorage` 的 `user` 对象；拦截器读 `user.userName` 判断是否已登录，为空且 URL 不在免鉴权白名单则判会话过期、拦截并跳登录。`user.userType`：`1` 教师（应用 appId `002`），其余为学生（appId `003`） |
| `X-Auth-Token` | 仅出现在 CORS `Access-Control-Allow-Headers` 白名单（`Origin, Content-Type, X-Auth-Token`），前端默认**不发送**；是否存在备用令牌头需在成功会话 Network 复核 |
| 不出现的头 | 本端**无**门户式 `Authorization` 头、无 `gatewayAppId`（那是 iportal `ly-upp` 专属，见 §3） |
| 过期/异常码 | 响应拦截器：`401` + `code 53000010`、`510` + `code 51000010`、`401` + `code 53000012` 判会话过期跳登录；裸 fetch 缺头实测返 `5100000「系统异常」` |
| 登录/回调配置 | `loginPattern="manager-login"`；`loginApi=/api/login`；`casLoginApi=/cas/login?pattern=teacher-login&returnUrl=<当前页>`；`casLogoutApi=/api/cas/logout?pattern=teacher-login`；`checkCas=/api/checkCas`。账号密码登录提交 `userName` / `token`(密码 Base64) / `target` / `vrifyCode` / `pattern` / `timestamp` |

> **学生回调卡点（实测）**：本 bundle 为 gly 管理/教师端壳，CAS 回调 `pattern=teacher-login`；学生账号经该回调被 gly 后端拒绝，返 `{"code":50000500,"message":"单点登录失败"}`。前端 bundle 内未出现 `student-login` 等学生模式取值（0 命中），学生端正确回调 `pattern` 由后端应用库下发、未在静态资源中暴露。下表字段映射来自早先一次成功进入学生教务 SPA 的会话；本轮未能复现进入学生 SPA，故上述头清单为**前端静态推断**，原始成功 XHR 的实际 Request Headers（Cookie 名、是否带 `X-Auth-Token`、`csrfToken` 实测值）仍需在可进入学生教务的会话里从 Network 复核（见 §9-1）。

**字段映射（拼音缩写 key，本人学生账号样本；文档只记字段名/含义/敏感级，不落真实值）**：

| jwxt key | 含义 / Meaning | 备注 / Notes |
|---|---|---|
| `xsid` | 学生内部 ID | 同时用于接口路径段 |
| `xh` | 学号（教务学号） | |
| `xm` | 姓名 | |
| `xbmc` | 性别名称 | |
| `mzmc` | 民族名称 | |
| `zzmmm` | 政治面貌名称 | |
| `csrq` | 出生日期 | `YYYYMMDD`；敏感 |
| `zjzlmc` | 证件种类名称 | 如居民身份证 |
| `zjhm` | 证件号码（身份证号） | **高敏感，客户端不得采集/上屏/落盘** |
| `ksh` | 考生号 | 敏感，默认不采集 |
| `lxdh` | 联系电话（手机） | 敏感，默认不采集 |
| `nj` | 年级 | 独立年级字段 |
| `xymc` | 学院名称（规范全称） | 门户拿不到的学院全称在此 |
| `zyfmc` | 年级专业方向名称 | 专业 + 方向拼接文本 |
| `bjmc` | 班级名称 | |
| `xqmc` | 校区名称 | |
| `xz` | 学制 | 数字（年） |
| `pyccmc` | 培养层次名称 | 如本科 |
| `rxrq` | 入学日期 | `YYYY-MM-DD` |
| `xjztmc` | 学籍状态名称 | 如在校；对应在读/休学等 |
| `sfzj` | 是否在校 | 标志 `1`/`0` |
| `zxzt` | 在校状态 | 标志 `1`/`0` |
| `xs1bmc` | 培养类别 | 如师范 |
| `xxnx` | 在校年级/学期标志 | 实测 `-8`，确切含义待复核 |
| `zxcpbase` | 在校照 | base64 data URI |
| `byzbase` | 毕业照 | base64 data URI |
| `bz` | 备注 | 多为数据导入时间戳 |

**其余教务 Tab（接口未逐一展开）**：家庭信息 `GET selectFamListByxsid?pageNum=&xsid=`（列表，实测 0 条；表头含称谓/姓名等）；另有学生成绩信息、学生异动信息、注册信息三个 Tab，对应独立 `.do`/`/api/...` 接口，与账号资料展示关系不大，留待按需抓包。

### 4.3 数据门户 dsj（学情画像，登录会话实测）/ data portal

入口 `https://dsj.gdei.edu.cn/sjfx/eportal/pcportal/portal.do?id=MH_XS`，凭 CAS 会话免密进入「综合学情分析 · 个人画像」。后端为报表型 `.do` 端点（`portal.do`、`queryserver.do`、`showreport.do`、`reqmgr.do`），前端 echarts 渲染。展示字段与教务 jwxt 同源（姓名/学号/院系/专业/班级/年级），画像页另含：

| 画像字段 | 含义 / Meaning | 备注 / Notes |
|---|---|---|
| 专业代码 / 班级代码 | 专业、班级的内部编码 | 教务基本信息接口未直接返回 |
| 生源类型 / 生源地 | 招生来源 | 敏感个人背景，默认不采集 |
| 宿舍房间号 | 在校宿舍 | **隐私敏感，客户端不得采集/上屏** |
| 电子邮箱 | 邮箱 | 多为空 |
| GPA、修读/优秀/及格/挂科课程数 | 课程成绩分析 | 学情统计，非身份字段 |

> dsj 不提供独立于教务的身份学籍主数据；客户端若需学院/专业/年级/学籍状态，以 jwxt `selectMyjwByid` 为准。

### 4.4 字段来源矩阵与多源降级 / Field source matrix & fallback

**三类数据源接口**：

| 源 | 系统 | 接口 | 字段厚度 |
|---|---|---|---|
| A | 融合门户 iportal | `tryLoginUserInfo`（主资料）+ `api/upp/userControl/getLoginInfo`（补充 `headPortrait`/`strategy`） | 薄：仅姓名/学号/班级文本/性别/头像等基础身份 |
| B | 教务 jwxt | `GET /api/studentInfo/selectMyjwByid/<xsid>`（主学籍）；`selectFamListByxsid`（家庭列表，实测空） | 最全：学院/专业/年级/班级/学籍状态/民族/政治面貌/校区/学制/培养层次等 |
| C | 数据门户 dsj | `portal.do?id=MH_XS` → `queryserver.do` / `showreport.do` / `reqmgr.do`（画像，与 B 同源） | 中：身份字段与 B 同源，另含专业/班级代码、邮箱、成绩统计 |

**展示身份字段 × 来源矩阵**（✓ 有规范字段；△ 拼接/派生/样本为空；— 无）：

| 展示字段 | A 门户 | B 教务 | C 数据门户 | 降级顺序 | 备注 |
|---|---|---|---|---|---|
| 姓名 | ✓ `userName` | ✓ `xm` | ✓ 画像姓名 | A → B → C | 三源互为冗余 |
| 学号 | ✓ `userId` | ✓ `xh` | ✓ 画像学号 | A → B → C | 两源学号是否同值待复核 |
| 班级 | △ `departmentName`（年级+方向+班拼接文本） | ✓ `bjmc` | ✓ 画像班级 | B → A → C | 规范名以 B 为准，A 文本仅兜底 |
| 学院全称 | —（`departments` 学生无权限） | ✓ `xymc` | ✓ 画像院系 | B → C | A 给不了 |
| 专业方向 | — | ✓ `zyfmc` | ✓ 画像专业 | B → C | |
| 年级 | —（拼在班级文本里） | ✓ `nj` | ✓ 画像年级 | B → C | |
| 性别 | ✓ `sex`（`1` 男/余女） | ✓ `xbmc`（名称） | — | A → B | A 为标志位，B 为名称 |
| 学籍状态 | — | ✓ `xjztmc` | — | **B 独占** | |
| 头像 | ✓ `headPortrait`（data URI） | ✓ `zxcpbase` / `byzbase` | — | A → B | |
| 民族 | — | ✓ `mzmc` | — | **B 独占** | |
| 政治面貌 | — | ✓ `zzmmm` | ✓ 画像政治面貌 | B → C | |
| 校区 | — | ✓ `xqmc` | — | **B 独占** | |
| 学制 | — | ✓ `xz` | ✓ 画像学制 | B → C | |
| 培养层次 | — | ✓ `pyccmc` | — | **B 独占** | |
| 入学日期 | — | ✓ `rxrq` | ✓ 画像入学日期 | B → C | 按需上屏 |
| 培养类别 | — | ✓ `xs1bmc` | — | **B 独占** | |
| 在校标志 | — | ✓ `sfzj` / `zxzt` | — | **B 独占** | |
| 邮箱 | △ `email`（学生样本 null） | — | ✓ 画像邮箱 | C → A | |
| 专业/班级代码 | — | — | ✓ 画像代码 | **C 独占** | 一般不上屏 |

**不参与降级采集的高敏感字段**（任一源返回也不采集、不上屏、不落库）：身份证 `zjhm`、手机 `lxdh`、考生号 `ksh`、出生日期 `csrq`、宿舍房间号、生源地 / 生源类型。

**降级策略（字段级多源兜底）**：

1. **分级拉取，不阻塞主流程**：会话建立后先拉 A（门户，CAS 回调第一个落地、最易进）拿基础身份（姓名/学号/性别/头像/班级文本）；再异步尝试 B（教务，字段最全）补齐学院/专业/年级/学籍状态等；B 不可达再尝试 C（数据门户）兜底同源字段。连接 / 保活等 NAC 主流程不等资料接口。
2. **字段级合并、规范来源优先**：班级 / 学院 / 专业 / 年级 / 学籍状态以 B 为准，B 缺则用 C，再缺才用 A 的拼接文本；姓名 / 学号 / 性别 / 头像三源互为冗余。合并时记录每个字段实际来源，便于排障。
3. **缺省兜底**：某字段所有源都拿不到，上屏占位「—」，不报错、不阻塞；不凭缓存的他人或历史信息对外展示。
4. **源独立健康判断**：A / B / C 各自独立超时与失败计数；某源当次会话连续失败即标记不可用、跳过其拉取，不拖慢整体。
5. **一致性校验**：若 A 学号与 B `xh`、A 姓名与 B `xm` 不一致，以最近成功源为准并记日志，不静默互相覆盖。

**已知限制 / 待补**：

- B 教务接口路径段 `xsid`（学生内部 ID）≠ 门户学号 `userId`；进入教务后需先由教务「我的信息」入口解析当前 `xsid`，该解析接口本轮未抓。
- 学生账号经 gly `pattern=teacher-login` 回调被拒（50000500），B 在学生端可达性不稳定；C 与 B 同源，是 B 字段的主要兜底。
- A 的 `departmentName` 是拼接文本，不可拆出独立年级 / 专业；规范拆分只能靠 B / C。
- 各源字段编码、空值口径（`null` vs 空串）需在真实会话对齐。

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
- **最小化采集**：客户端上屏/落盘仅取展示所需身份字段（姓名、学号、班级、学院、专业方向、年级、性别、学籍状态、头像）。身份证号（`zjhm`）、手机号（`lxdh`）、考生号（`ksh`）、出生日期（`csrq`）、宿舍房间号、生源地等属高敏感个人信息，默认不采集、不上屏、不落本地库。
- **信任锚边界**：NAC 门户仍是校园网入网认证与设备在线状态的信任根；CAS/iportal 仅用于在用户已入网、本人登录后**补全展示用账号资料**（姓名/部门等），不替代 NAC 入网认证，也不在离线/未入网时凭缓存资料对外鉴权。
- 字段名以真实接口 JSON 为准；4.1 已在本人学生账号登录会话核对。教师账号（`userType=teacher`）字段填充情况本项目不自行复核（本人为学生，见 §9-5），接入教师侧前需另用教师账号验证。

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

**2026-11-26 已实测闭环（无需再抓）**：

- CAS 外壳 / `sign` / `JSESSIONID` 链路、`login2022` 表单字段、RSA 加密与隐藏域、无图形验证码、标准校验端点 404、三套子系统回调。
- 登录成功跳转：POST `/cas/login` 返 200、body JS 跳转、CAS 两次换发 `ST-<序号>-<随机串>`、门户 SPA 落地 `#/StudentHomepage`。
- 门户鉴权：`withCredentials` + `X-Requested-With` + `csrfToken` 头；会话含 JS 可见 `Authorization` Cookie 与 Shiro HttpOnly 会话。
- 主资料接口 `tryLoginUserInfo` 与补充接口 `getLoginInfo` 的分工；`userLogin` 全字段名与取值口径（4.1）；`departments` 学生账号 500 无权限。
- 主登录 RSA 用表单内嵌公钥，`getkey` 动态公钥仅弱口令分支。
- jwxt 教务 SSO 免密进入；主资料接口 `/api/studentInfo/selectMyjwByid/<xsid>` 全量学籍字段映射（4.2）；dsj 学情画像入口与补充字段（4.3）。学院全称、专业方向、年级、学籍状态、培养层次、校区、学制、入学日期等身份字段均已在 jwxt 取到。

**仍需补抓 / 复核**：

1. **jwxt 网关鉴权头（静态已推断，原始 Network 待复核）**：前端 bundle 已确认鉴权体系为「后端会话 Cookie（`withCredentials`）+ `X-Requested-With: XMLHttpRequest` + `csrfToken=MD5(秒级时间戳+"lyedu")`（时间戳同时放 query `_t`）」，无 `Authorization`/`gatewayAppId` 头；`X-Auth-Token` 仅在 CORS 白名单。待能进入学生教务 SPA 的会话里，从 Network 点开成功的 `selectMyjwByid` XHR 复核：实际会话 Cookie 名/属性、是否真的携带 `X-Auth-Token`、`csrfToken` 实测格式与 `_t` 口径。本轮学生账号经 gly `pattern=teacher-login` 回调被拒（50000500），未能进入学生 SPA 抓原始头。
2. **教务其余 Tab 接口**：学生异动信息、注册信息、成绩信息三个 Tab 的接口路径与字段（与账号资料展示关系不大，按需再抓）；家庭信息 `selectFamListByxsid` 列表实测为空。
3. **会话 Cookie 名细节**：Shiro HttpOnly 会话 Cookie 的具体名称/属性；jwxt、dsj 回调各自建立的会话 Cookie。命令行 CookieJar 复现完整登录链尚未跑通（`Invoke-WebRequest` 重定向拦截、`HttpWebRequest` TLS 握手、`HttpClient` 空 Content 均失败），客户端实现前需用 Qt 网络栈重新验证 Cookie 保留与 302 跟随。
4. **门户令牌用途**：`tokenId` / `accessToken` / `refreshToken` / `authorization` 各自用途、有效期与客户端是否需要复用/刷新；`tryLoginUserInfo` 的 `Authorization` 头与 `Authorization` Cookie 的关系。
5. **教师账号字段复核（已关闭，不再推进）**：本人为学生账号，无法登录教师端；4.1 / 4.2 口径均来自学生样本。教师（`userType=teacher`）的工号、部门、职称等字段填充情况留待有教师账号时另抓，本项目不自行尝试获取。
6. **头像落地策略**：`headPortrait` / `zxcpbase` 为 base64 data URI，客户端展示时的解码、缓存与缺省兜底方案。
7. **WebVPN**：分析其登录接口与代理 URL 改写规则，判断客户端校外可达性方案。
8. **教务 `xsid` 解析与跨源学号一致性**：`selectMyjwByid` 路径段 `xsid` 为学生内部 ID，不等于门户 `userId`；需抓教务进入首页后解析当前 `xsid` 的入口接口。并在真实会话比对门户 `userId` 与教务 `xh`、画像学号是否同值，确定多源降级合并时以哪个为准（矩阵见 4.4）。

> 院徽采集已完成（见第 7、8 节）：18 个学院站均无独立院徽，头部沿用校徽变体，映射见 `colleges.json`。
