import os
import shutil
from os.path import *

from optparse import OptionParser

parser = OptionParser(description='Indigo Java libraries build script')
parser.add_option('--suffix', '-s', help='archive suffix', default="")

(args, left_args) = parser.parse_args()

# Find indigo version
from get_indigo_version import getIndigoVersion
version = getIndigoVersion()

api_dir = abspath(dirname(__file__))
root = join(api_dir, "..")
dist_dir = join(root, "dist")
if not os.path.exists(dist_dir):
    os.mkdir(dist_dir)

archive_name = "./indigo-python-" + version + args.suffix

dest = os.path.join(dist_dir, archive_name)
if os.path.exists(dest):
    shutil.rmtree(dest)
os.mkdir(dest)
shutil.copy(os.path.join(api_dir, "python", "indigo.py"), dest)
shutil.copy(os.path.join(api_dir, "plugins", "renderer", "python", "indigo_renderer.py"), dest)
shutil.copy(os.path.join(api_dir, "plugins", "inchi", "python", "indigo_inchi.py"), dest)
shutil.copy(os.path.join(api_dir, "plugins", "bingo", "python", "bingo.py"), dest)
shutil.copytree(os.path.join(api_dir, "libs", "shared"),
                os.path.join(dest, "lib"),
                ignore=shutil.ignore_patterns("*.lib"))

shutil.copy(os.path.join(api_dir, "LICENSE.GPL"), dest)
os.chdir(dist_dir)
if os.path.exists(archive_name + ".zip"):
    os.remove(archive_name + ".zip")
shutil.make_archive(archive_name, 'zip', os.path.dirname(archive_name), archive_name)
shutil.rmtree(archive_name)
full_archive_name = os.path.normpath(os.path.join(dist_dir, archive_name))
print('Archive {}.zip created'.format(full_archive_name))
