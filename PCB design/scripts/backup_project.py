# WRO boards — consistent snapshot of a live EasyEDA project file (.eprj2 is SQLite) while EasyEDA has it open: Python's
# sqlite3 backup API from a READ-ONLY connection, then PRAGMA integrity_check. Never overwrites an existing backup.
# From the pcb-pipeline template production/backup_project.py, with the two WRO projects. Save first (the writing
# scripts end with a save; otherwise save in the GUI).
# Usage: python backup_project.py robot|hub|station <tag>   ->  PCB design/Robot/WRO_Robot_<tag>.eprj2 | PCB design/hub/WRO_HUB_<tag>.eprj2
import os
import sqlite3
import sys
import time

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
PROJECTS = {'robot': ('Robot', 'WRO_Robot'), 'hub': ('hub', 'WRO_HUB'), 'station': ('station', 'station_pcb')}


def main():
    board, tag = sys.argv[1], sys.argv[2]
    folder, name = PROJECTS[board]
    src_path = os.path.join(ROOT, folder, name + '.eprj2')
    dst_path = os.path.join(ROOT, folder, '%s_%s.eprj2' % (name, tag))
    if os.path.exists(dst_path):
        sys.exit('backup already exists: ' + dst_path)
    print('source', os.path.getsize(src_path), 'bytes, saved', time.ctime(os.path.getmtime(src_path)))
    src = sqlite3.connect('file:' + src_path.replace('\\', '/') + '?mode=ro', uri=True)
    dst = sqlite3.connect(dst_path)
    with dst:
        src.backup(dst)
    src.close()
    print('integrity', dst.execute('PRAGMA integrity_check').fetchone()[0])
    dst.close()
    print('backup', os.path.getsize(dst_path), 'bytes ->', dst_path)


if __name__ == '__main__':
    main()
