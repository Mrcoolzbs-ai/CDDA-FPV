#!/usr/bin/env python3
"""Package a freshly built Windows executable and its recursive PE dependencies.
Run from the repository root under MSYS2; missing non-system imports are fatal.
"""
import argparse, hashlib, json, re, shutil, subprocess, tempfile, zipfile
from pathlib import Path
p=argparse.ArgumentParser()
p.add_argument('--exe',required=True,type=Path)
p.add_argument('--dll-dir',required=True,type=Path)
p.add_argument('--output',required=True,type=Path)
a=p.parse_args()
root=Path(__file__).resolve().parents[2]
# These DLLs are provided by supported Windows installations, not the SDK.
system=set('kernel32.dll user32.dll gdi32.dll advapi32.dll shell32.dll ole32.dll oleaut32.dll uuid.dll comdlg32.dll comctl32.dll msvcrt.dll ucrtbase.dll ws2_32.dll winmm.dll imm32.dll version.dll setupapi.dll cfgmgr32.dll shlwapi.dll bcrypt.dll crypt32.dll secur32.dll ntdll.dll rpcrt4.dll powrprof.dll psapi.dll iphlpapi.dll dbghelp.dll dwmapi.dll hid.dll winspool.drv dinput8.dll dxgi.dll d3d11.dll opengl32.dll glu32.dll avrt.dll propsys.dll mfplat.dll mf.dll mfreadwrite.dll strmiids.dll ksuser.dll msacm32.dll msimg32.dll winhttp.dll normaliz.dll dnsapi.dll wldap32.dll authz.dll netapi32.dll userenv.dll'.split())
dlls={f.name.lower():f for f in a.dll_dir.glob('*.dll')}
if not a.exe.is_file():raise SystemExit('Fresh EXE missing')
mo=root/'lang/mo/ru/LC_MESSAGES/cataclysm-dda.mo'
if not mo.is_file():raise SystemExit('Russian MO catalog missing')
with tempfile.TemporaryDirectory(prefix='fpv-package-') as tmp:
    dest=Path(tmp)/'CDDA-FPV'
    dest.mkdir()
    shutil.copy2(a.exe,dest/'cataclysm-tiles.exe')
    queue=[dest/'cataclysm-tiles.exe'];copied=set();missing=set()
    while queue:
        binary=queue.pop()
        output=subprocess.check_output(['objdump','-p',str(binary)],text=True,errors='replace')
        for name in re.findall(r'DLL Name:\s*(\S+)',output):
            key=name.lower()
            if key in system or key.startswith(('api-ms-win-','ext-ms-win-')):continue
            if key in copied:continue
            if key not in dlls:missing.add(name);continue
            copied.add(key)
            target=dest/dlls[key].name
            shutil.copy2(dlls[key],target);queue.append(target)
    if missing:raise SystemExit('Unresolved PE imports: '+', '.join(sorted(missing)))
    for folder in ['data','gfx']:
        shutil.copytree(root/folder,dest/folder,ignore=shutil.ignore_patterns('.git','__pycache__'))
    (dest/'lang/mo/ru/LC_MESSAGES').mkdir(parents=True)
    shutil.copy2(mo,dest/'lang/mo/ru/LC_MESSAGES'/mo.name)
    shutil.copytree(root/'fpv/config-ru',dest/'config')
    for name in ['LICENSE.txt','README.md','fpv/BUILD_RU.md','fpv/TEST_REPORT.md']:
        shutil.copy2(root/name,dest/Path(name).name)
    licenses=dest/'licenses';licenses.mkdir()
    for share in [a.dll_dir.parent/'share/licenses',a.dll_dir.parent/'share/doc']:
        if share.exists():
            for d in share.iterdir():
                if d.is_dir() and ('license' in str(share) or d.name.lower() in ('sdl2','sdl2_image','sdl2_mixer','sdl2_ttf')):
                    shutil.copytree(d,licenses/d.name,dirs_exist_ok=True)
    (dest/'START-CDDA-FPV.bat').write_bytes(b'@echo off\r\ncd /d "%~dp0"\r\nstart "" cataclysm-tiles.exe\r\n')
    commit=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
    manifest={'version':'0.6.0-ru','commit':commit,'exe_sha256':hashlib.sha256((dest/'cataclysm-tiles.exe').read_bytes()).hexdigest(),'dlls':sorted(copied)}
    (dest/'BUILD.json').write_text(json.dumps(manifest,indent=2)+'\n')
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(a.output,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        for f in sorted(dest.rglob('*')):
            if f.is_file():z.write(f,f.relative_to(dest.parent))
print(f'{a.output.name}: {a.output.stat().st_size} bytes; {len(copied)} DLLs; commit {commit}')
