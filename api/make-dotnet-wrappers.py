from optparse import OptionParser
import os
from os.path import join, abspath, dirname
import shutil
import subprocess

from get_indigo_version import getIndigoVersion


if os.name == 'nt':
    msbuildcommand = 'msbuild /t:Rebuild /p:Configuration=Release'
else:
    # Mono
    msbuildcommand = 'xbuild /t:Rebuild /p:Configuration=Release'

parser = OptionParser(description='Indigo .NET libraries build script')
parser.add_option('--suffix', '-s', help='archive suffix', default="")
parser.add_option('--doc', default=False, action='store_true', help='Put documentation into the archive')
(args, left_args) = parser.parse_args()

wrappers = (args.suffix[1:], )
print(wrappers)
if 'universal' in wrappers:
    wrappers = ('win', 'linux', 'mac')

api_dir = abspath(dirname(__file__))
root = join(api_dir, "..")
dist_dir = join(root, "dist")
if not os.path.exists(dist_dir):
    os.mkdir(dist_dir)

cur_dir = os.path.abspath(os.curdir)

# Find indigo version
version = getIndigoVersion()

os.chdir(dist_dir)
if os.path.exists("dotnet"):
    shutil.rmtree("dotnet")
os.mkdir('dotnet')

libraryPath = join(api_dir, 'libs', 'shared')

# Build Indigo-dotnet
indigoDotNetPath = join(api_dir, "dotnet")
if os.path.exists(join(indigoDotNetPath, "Resource")):
    shutil.rmtree(join(indigoDotNetPath, "Resource"))


if 'win' in wrappers:
    os.makedirs(join(indigoDotNetPath, "Resource", 'Win', 'x64'))
    os.makedirs(join(indigoDotNetPath, "Resource", 'Win', 'x86'))
    if os.path.exists(join(libraryPath, 'Win', 'x64', 'msvcr120.dll')):
        win2013 = 1
        win2015 = 0
    elif os.path.exists(join(libraryPath, 'Win', 'x64', 'vcruntime140.dll')):
        win2013 = 0
        win2015 = 1
    else:
        win2013 = 0
        win2015 = 0
else:
    win2013 = 0
    win2015 = 0

if 'linux' in wrappers:
    os.makedirs(join(indigoDotNetPath, "Resource", 'Linux', 'x64'))
    os.makedirs(join(indigoDotNetPath, "Resource", 'Linux', 'x86'))
    linux = 1
else:
    linux = 0

if 'mac' in wrappers:
    os.makedirs(join(indigoDotNetPath, "Resource", 'Mac', '10.7'))
    mac = 1
else:
    mac = 0

os.chdir(indigoDotNetPath)
command = '%s /property:LibraryPath=%s /property:Win2013=%s /property:Win2015=%s /property:Linux=%s /property:Mac=%s /property:Copy=%s' % (
    msbuildcommand, libraryPath, win2013, win2015, linux, mac, 'copy' if os.name == 'nt' else 'cp')
print(command)
subprocess.check_call(command, shell=True)

# Build IndigoRenderer-dotnet
indigoRendererDotNetPath = join(api_dir, "plugins", "renderer", "dotnet")
if os.path.exists(join(indigoRendererDotNetPath, "Resource")):
    shutil.rmtree(join(indigoRendererDotNetPath, "Resource"))
if 'win' in wrappers:
    os.makedirs(join(indigoRendererDotNetPath, "Resource", 'Win', 'x64'))
    os.makedirs(join(indigoRendererDotNetPath, "Resource", 'Win', 'x86'))
    win = 1
else:
    win = 0

if 'linux' in wrappers:
    os.makedirs(join(indigoRendererDotNetPath, "Resource", 'Linux', 'x64'))
    os.makedirs(join(indigoRendererDotNetPath, "Resource", 'Linux', 'x86'))
    linux = 1
else:
    linux = 0

if 'mac' in wrappers:
    os.makedirs(join(indigoRendererDotNetPath, "Resource", 'Mac', '10.7'))
    mac = 1
else:
    mac = 0

os.chdir(indigoRendererDotNetPath)
command = '%s /property:LibraryPath=%s /property:Win=%s /property:Linux=%s /property:Mac=%s /property:Copy=%s' % (
    msbuildcommand, join(api_dir, 'libs', 'shared'), win, linux, mac, 'copy' if os.name == 'nt' else 'cp')
print(command)
subprocess.check_call(command, shell=True)

# Build IndigoInchi-dotnet
indigoInchiDotNetPath = join(api_dir, "plugins", "inchi", "dotnet")
if os.path.exists(join(indigoInchiDotNetPath, "Resource")):
    shutil.rmtree(join(indigoInchiDotNetPath, "Resource"))
if 'win' in wrappers:
    os.makedirs(join(indigoInchiDotNetPath, "Resource", 'Win', 'x64'))
    os.makedirs(join(indigoInchiDotNetPath, "Resource", 'Win', 'x86'))
    win = 1
else:
    win = 0

if 'linux' in wrappers:
    os.makedirs(join(indigoInchiDotNetPath, "Resource", 'Linux', 'x64'))
    os.makedirs(join(indigoInchiDotNetPath, "Resource", 'Linux', 'x86'))
    linux = 1
else:
    linux = 0

if 'mac' in wrappers:
    os.makedirs(join(indigoInchiDotNetPath, "Resource", 'Mac', '10.7'))
    mac = 1
else:
    mac = 0


os.chdir(indigoInchiDotNetPath)
command = '%s /property:LibraryPath=%s /property:Win=%s /property:Linux=%s /property:Mac=%s /property:Copy=%s' % (
    msbuildcommand, join(api_dir, 'libs', 'shared'), win, linux, mac, 'copy' if os.name == 'nt' else 'cp')
print(command)
subprocess.check_call(command, shell=True)

# Build Bingo-dotnet
bingoDotNetPath = join(api_dir, "plugins", "bingo", "dotnet")
if os.path.exists(join(bingoDotNetPath, "Resource")):
    shutil.rmtree(join(bingoDotNetPath, "Resource"))
if 'win' in wrappers:
    os.makedirs(join(bingoDotNetPath, "Resource", 'Win', 'x64'))
    os.makedirs(join(bingoDotNetPath, "Resource", 'Win', 'x86'))
    win = 1
else:
    win = 0

if 'linux' in wrappers:
    os.makedirs(join(bingoDotNetPath, "Resource", 'Linux', 'x64'))
    os.makedirs(join(bingoDotNetPath, "Resource", 'Linux', 'x86'))
    linux = 1
else:
    linux = 0

if 'mac' in wrappers:
    os.makedirs(join(bingoDotNetPath, "Resource", 'Mac', '10.7'))
    mac = 1
else:
    mac = 0

os.chdir(bingoDotNetPath)
command = '%s /property:LibraryPath=%s /property:Win=%s /property:Linux=%s /property:Mac=%s /property:Copy=%s' % (
    msbuildcommand, join(api_dir, 'libs', 'shared'), win, linux, mac, 'copy' if os.name == 'nt' else 'cp')
print(command)
subprocess.check_call(command, shell=True)

# Zip results
doc_dir = join(api_dir, '..', 'doc')
os.chdir(dist_dir)
shutil.copy(os.path.join(api_dir, "LICENSE.GPL"), "dotnet")
shutil.copy(join(indigoDotNetPath, 'bin', 'Release', 'indigo-dotnet.dll'), "dotnet")
shutil.copy(join(indigoRendererDotNetPath, 'bin', 'Release', 'indigo-renderer-dotnet.dll'), "dotnet")
shutil.copy(join(indigoInchiDotNetPath, 'bin', 'Release', 'indigo-inchi-dotnet.dll'), "dotnet")
shutil.copy(join(bingoDotNetPath, 'bin', 'Release', 'bingo-dotnet.dll'), "dotnet")
shutil.copy(join(bingoDotNetPath, 'bin', 'Release', 'bingo-dotnet.XML'), "dotnet")
if args.doc:
    shutil.copytree(os.path.join(doc_dir, 'build', 'html'), os.path.join('dotnet', 'doc'))

archive_name = "indigo-dotnet-%s" % (version + args.suffix)
os.rename("dotnet", archive_name)
os.system("zip -r -9 -m %s.zip %s" % (archive_name, archive_name))
