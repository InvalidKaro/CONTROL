from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
errors = []

required = [
    'platformio.ini', 'include/app.h', 'include/board_pins.h',
    'src/main.cpp', 'src/core/ControlOS.cpp', 'src/core/WebUi.cpp',
    '.github/workflows/build.yaml'
]
for item in required:
    if not (root / item).exists():
        errors.append(f'missing: {item}')

control_h = (root / 'src/core/ControlOS.h').read_text()
control_cpp = (root / 'src/core/ControlOS.cpp').read_text()
m = re.search(r'App\* apps_\[(\d+)\]', control_h)
n = re.search(r'AppCount\s*=\s*(\d+)', control_h)
if not m or not n or m.group(1) != n.group(1):
    errors.append('App array size and AppCount do not match')

for local in re.findall(r'#include\s+"([^"]+)"', '\n'.join(p.read_text(errors='ignore') for p in (root/'src').rglob('*.[ch]pp'))):
    matches = list(root.rglob(local))
    if not matches:
        errors.append(f'unresolved local include: {local}')

web = (root / 'src/core/WebUi.cpp').read_text()
for endpoint in ['/api/status','/api/apps','/api/control','/api/led','/api/files','/api/ota']:
    if endpoint not in web:
        errors.append(f'missing WebUI endpoint: {endpoint}')

if '172, 0, 0, 1' not in web:
    errors.append('172.0.0.1 AP configuration not found')
if 'webPassword_ = "control"' not in web:
    errors.append('WebUI password is not control')

if errors:
    print('\n'.join(errors))
    sys.exit(1)
print('ControlOS static project validation: OK')
