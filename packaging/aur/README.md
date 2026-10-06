# AUR

包页：https://aur.archlinux.org/packages/fiilctl

## 首次发布（已完成 2026-10-06）

```bash
cd /tmp && rm -rf aur-push && mkdir aur-push && cd aur-push
git clone ssh://aur@aur.archlinux.org/fiilctl.git && cd fiilctl
cp ~/Dev/fiilctl/packaging/aur/{PKGBUILD,.SRCINFO} .
git add PKGBUILD .SRCINFO
git commit -m "Initial import: fiilctl 1.0.0-1"
git push origin HEAD:master
```

## 发新版本

```bash
# 1) 打 tag 推到 GitHub（CI 会顺手把 .deb 附到 Release）
cd ~/Dev/fiilctl && git tag -a v1.0.1 -m "fiilctl 1.0.1" && git push origin v1.0.1

# 2) 更新 PKGBUILD 的 pkgver + sha256
cd packaging/aur
v=1.0.1
sed -i "s/^pkgver=.*/pkgver=$v/" PKGBUILD
url="https://github.com/xzl01/fiilctl/archive/refs/tags/v$v.tar.gz"
sha=$(curl -sL "$url" | sha256sum | cut -d' ' -f1)
sed -i "s/^sha256sums=.*/sha256sums=('$sha')/" PKGBUILD
makepkg --printsrcinfo > .SRCINFO          # 需要在本目录执行

# 3) 推到 AUR（首次 commit 用 "upgpkg: <新版本>: <一句话>"）
git -C <本地 AUR 克隆> ... # 见上面流程
```

## ⚠️ 前提：`aur.archlinux.org` 必须走直连

AUR 的 SSH 会**拒绝代理出口 IP**，症状是 TCP 连上但立刻被对端关闭、连 banner 都不发：

```
$ ssh -T aur@aur.archlinux.org
Connection closed by 198.18.0.29 port 22      # 198.18.x = mihomo fake-ip
```

把代理切到 DIRECT（或给 `aur.archlinux.org` 加 DIRECT 规则）后即可正常推送，推送完记得切回来。
`https://aur.archlinux.org`（RPC/网页/HTTPS clone）走代理没问题，只有 SSH 推送有这个问题。
