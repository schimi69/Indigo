import os
import re
import shutil
import subprocess
from os.path import *

from optparse import OptionParser
from zipfile import ZipFile

import sys


def get_cpu_count():
    cpu_count = 1
    if os.name == 'java':
        from java.lang import Runtime
        runtime = Runtime.getRuntime()
        cpu_count = runtime.availableProcessors()
    else:
        import multiprocessing
        cpu_count = multiprocessing.cpu_count()
    return cpu_count


def shortenDBMS(dbms):
    if dbms == 'postgres':
        return 'pg'
    elif dbms == 'sqlserver':
        return 'mssql'
    elif dbms == 'oracle':
        return 'ora'


def shortenGenerator(generator):
    result = generator
    if generator.startswith('Visual Studio '):
        result = generator.replace('Visual Studio ', 'VS')
    elif generator.startswith('Unix MakeFiles'):
        result = generator.replace('Unix Makefiles', 'umake')
    elif generator.startswith('Unix MakeFiles'):
        result = generator.replace('MinGW Makefiles', 'mmake')
    return result.replace(' ', '')

def getBingoVersion():
    version = "unknown"

    for line in open(os.path.join(os.path.dirname(__file__), "..","bingo","bingo-core","src","core", "bingo_version.h")):
        m = re.search('BINGO_VERSION "(.*)-.*"', line)
        if m:
            version = m.group(1)
    return version

presets = {
    "win32-2013": ("Visual Studio 12", ""),
    "win32-2015": ("Visual Studio 14", ""),
    "win64-2013": ("Visual Studio 12 Win64", ""),
    "win64-2015": ("Visual Studio 14 Win64", ""),
    "win32-mingw": ("MinGW Makefiles", ""),
    "linux32": ("Unix Makefiles", "-DSUBSYSTEM_NAME=x86"),
    "linux32-universal": ("Unix Makefiles", "-DSUBSYSTEM_NAME=x86"),
    "linux64": ("Unix Makefiles", "-DSUBSYSTEM_NAME=x64"),
    "linux64-universal": ("Unix Makefiles", "-DSUBSYSTEM_NAME=x64"),
    "mac10.7": ("Xcode", "-DSUBSYSTEM_NAME=10.7"),
    "mac10.8": ("Xcode", "-DSUBSYSTEM_NAME=10.8"),
    "mac10.9": ("Xcode", "-DSUBSYSTEM_NAME=10.9"),
    "mac10.10": ("Xcode", "-DSUBSYSTEM_NAME=10.10"),
    "mac10.11": ("Xcode", "-DSUBSYSTEM_NAME=10.11"),
    "mac10.12": ("Xcode", "-DSUBSYSTEM_NAME=10.12"),
    "mac10.13": ("Xcode", "-DSUBSYSTEM_NAME=10.13"),
    "mac-universal": ("Unix Makefiles", "-DSUBSYSTEM_NAME=10.7"),
}

parser = OptionParser(description='Bingo build script')
parser.add_option('--generator', help='this option is passed as -G option for cmake')
parser.add_option('--params', default="", help='additional build parameters')
parser.add_option('--config', default="Release", help='project configuration')
parser.add_option('--dbms', help='DMBS (oracle, postgres or sqlserver)')
parser.add_option('--nobuild', default=False, action="store_true", help='configure without building', dest="nobuild")
parser.add_option('--clean', default=False, action="store_true", help='delete all the build data', dest="clean")
parser.add_option('--preset', type="choice", dest="preset", choices=list(presets.keys()), help='build preset %s' % (str(presets.keys())))
parser.add_option('--no-multithreaded-build', dest='mtbuild', default=True, action='store_false', help='Use only 1 core to build')

(args, left_args) = parser.parse_args()
if len(left_args) > 0:
    print("Unexpected arguments: %s" % (str(left_args)))
    exit()

if args.preset:
    args.generator, args.params = presets[args.preset]
if not args.generator and args.dbms != 'sqlserver':
    print("Generator must be specified")
    exit()

if args.preset and 'universal' in args.preset:
    args.params += ' -DUNIVERSAL_BUILD=TRUE'

cur_dir = abspath(dirname(__file__))
root = os.path.normpath(join(cur_dir, ".."))
project_dir = join(cur_dir, "bingo-%s" % args.dbms)

if args.dbms == 'oracle':
    print('dbms is oracle. Detecting oci...')

    print('Detecting local oci...')
    local_oci_include_dir = os.path.join(root, 'third_party/oci/include')
    print('Local oci include directory: ' + local_oci_include_dir)
    if os.path.isdir(local_oci_include_dir):
        print('The local oci include directory exists. Use local oci. Don\'t attach command line argument -DUSE_SYSTEM_OCI=ON.')
    else:
        print('The local oci include directory doesn\'t exist.')
        print('Detecting system oci...')
        if not 'ORACLE_HOME' in os.environ:
            print('ORACLE_HOME evn var wasn\'t found. It is needed to build with system oci.')
            raise Exception('Failed to detect neither local nor system oci.')
        print('ORACLE_HOME evn var was found. Use system oci. Attach command line argument -DUSE_SYSTEM_OCI=ON.')
        args.params += ' -DUSE_SYSTEM_OCI=ON'

if args.dbms != 'sqlserver':
    build_dir = (shortenDBMS(args.dbms) + " " + shortenGenerator(args.generator) + " " + args.config + args.params.replace('-D', ''))
    build_dir = build_dir.replace(" ", "_").replace("=", "_").replace("-", "_")
    full_build_dir = os.path.join(root, "build", build_dir)
    if os.path.exists(full_build_dir) and args.clean:
        print("Removing previous project files")
        shutil.rmtree(full_build_dir)
    if not os.path.exists(full_build_dir):
        os.makedirs(full_build_dir)

    if args.generator.find("Unix Makefiles") != -1:
        args.params += " -DCMAKE_BUILD_TYPE=" + args.config

    os.chdir(full_build_dir)
    command = "%s cmake -G \"%s\" %s %s" % ('CC=gcc CXX=g++' if (args.preset.find('linux') != -1 and args.preset.find('universal') != -1) else '', args.generator, args.params, project_dir)
    print(command)
    subprocess.check_call(command, shell=True)

    if args.nobuild:
        exit(0)

    for f in os.listdir(full_build_dir):
        path, ext = os.path.splitext(f)
        if ext == ".zip":
            os.remove(join(full_build_dir, f))

    if args.generator.find("Unix Makefiles") != -1:
        make_args = ''
        if args.mtbuild:
            make_args += ' -j{} '.format(get_cpu_count())

        subprocess.check_call("cmake --build . --config %s -- %s" % (args.config, make_args), shell=True)
        subprocess.check_call("make package", shell=True)
        subprocess.check_call("make install", shell=True)
    elif args.generator.find("Xcode") != -1:
        subprocess.check_call("cmake --build . --config %s" % args.config, shell=True)
        subprocess.check_call("cmake --build . --target package --config %s" % args.config, shell=True)
        subprocess.check_call("cmake --build . --target install --config %s" % args.config, shell=True)
    elif args.generator.find("Visual Studio") != -1:
        vsenv = os.environ
        if args.mtbuild:
            vsenv = dict(os.environ, CL='/MP')

        subprocess.check_call("cmake --build . --config %s" % args.config, env=vsenv, shell=True)
        subprocess.check_call("cmake --build . --target PACKAGE --config %s" % args.config,  shell=True)
        subprocess.check_call("cmake --build . --target INSTALL --config %s" % args.config, shell=True)
    else:
        print("Do not know how to run package and install target")

    os.chdir(root)
    if not os.path.exists("dist"):
        os.mkdir("dist")
    dist_dir = join(root, "dist")

    for f in os.listdir(full_build_dir):
        path, ext = os.path.splitext(f)
        if ext == ".zip":
            shutil.rmtree(join(full_build_dir, f.replace('-shared.zip', ''), f.replace('-shared.zip', '')), ignore_errors=True)
            zf = ZipFile(join(full_build_dir, f))
            zf.extractall(join(full_build_dir, f.replace('-shared.zip', ''), f.replace('-shared.zip', '')))
            zf = ZipFile(join(dist_dir, f.replace('-shared.zip', '.zip')), 'w')
            os.chdir(join(full_build_dir, f.replace('-shared.zip', '')))
            for root, dirs, files in os.walk('.'):
                for file in files:
                    if file.endswith('.sh') or file.endswith('.bat'):
                        os.chmod(join(root, file), 0o755)
                    zf.write(join(root, file))
            os.chdir(root)
else:
    dllPath = {}

    vsversion = 'Visual Studio'
    if args.preset.find("2012") != -1:
        vsversion += ' 11'
    elif args.preset.find("2013") != -1:
        vsversion += ' 12'
    elif args.preset.find("2015") != -1:
        vsversion += ' 14'
    else:
        vsversion += ' 10'

    for arch, generator in (('x86', vsversion), ('x64', vsversion + ' Win64')):
        build_dir = (shortenDBMS(args.dbms) + " " + shortenGenerator(generator) + " " + args.params)
        build_dir = build_dir.replace(" ", "_").replace("=", "_").replace("-", "_")
        full_build_dir = os.path.join(root, "build", build_dir)
        dllPath[arch] = os.path.normpath(os.path.join(full_build_dir, 'dist', 'Win', arch, 'lib', args.config))
        if os.path.exists(full_build_dir) and args.clean:
            print("Removing previous project files")
            shutil.rmtree(full_build_dir)
        if not os.path.exists(full_build_dir):
            os.makedirs(full_build_dir)
        os.chdir(full_build_dir)
        command = "cmake -G \"%s\" %s %s" % (generator, args.params, project_dir)
        print(command)
        subprocess.check_call(command, shell=True)

        if args.nobuild:
            exit(0)

        for f in os.listdir(full_build_dir):
            path, ext = os.path.splitext(f)
            if ext == ".zip":
                os.remove(join(full_build_dir, f))

        subprocess.check_call("cmake --build . --config %s" % args.config, shell=True)

    os.chdir(join(root, 'bingo', 'sqlserver'))
    command = 'msbuild /t:Rebuild /p:Configuration=%s /property:DllPath32=%s /property:DllPath64=%s' % (args.config, dllPath['x86'], dllPath['x64'])
    print(os.path.abspath(os.curdir), command)
    subprocess.check_call(command)

    os.chdir(root)
    if not os.path.exists("dist"):
        os.mkdir("dist")
    dist_dir = join(root, "dist")

    # Get version
    version = getBingoVersion()

    if not os.path.exists(join(root, 'dist', 'bingo-sqlserver-%s' % version)):
        os.makedirs(join(root, 'dist', 'bingo-sqlserver-%s' % version, 'bingo-sqlserver-%s' % version))
    else:
        shutil.rmtree(join(root, 'dist', 'bingo-sqlserver-%s' % version))
        os.makedirs(join(root, 'dist', 'bingo-sqlserver-%s' % version, 'bingo-sqlserver%s' % version))

    os.makedirs(join(root, 'dist', 'bingo-sqlserver-%s' % version, 'bingo-sqlserver-%s' % version, 'assembly'))
    for item in os.listdir(join(root, 'bingo', 'sqlserver', 'sql')):
        if item.endswith('.sql') or item.endswith('.bat'):
            shutil.copyfile(join(root, 'bingo', 'sqlserver', 'sql', item), join(root, 'dist', 'bingo-sqlserver-%s' % version, 'bingo-sqlserver-%s' % version, item))
    if not os.path.exists(join(root, 'bingo', 'sqlserver', 'bin', args.config, 'bingo-sqlserver.dll')):
        print('Warning: File %s does not exist, going to use empty stub instead' % join(root, 'dist', 'bingo-sqlserver-%s' % version, 'bingo-sqlserver-%s' % version, 'assembly', 'bingo-sqlserver.dll'))
        open(join(root, 'dist', 'bingo-sqlserver-%s' % version, 'bingo-sqlserver-%s' % version, 'assembly', 'bingo-sqlserver.dll', 'w')).close()
    else:
        shutil.copyfile(join(root, 'bingo', 'sqlserver', 'bin', args.config, 'bingo-sqlserver.dll'), join(root, 'dist', 'bingo-sqlserver-%s' % version, 'bingo-sqlserver-%s' % version, 'assembly', 'bingo-sqlserver.dll'))

    os.chdir('dist')
    if os.path.exists('bingo-sqlserver-%s.zip' % version):
        os.remove('bingo-sqlserver-%s.zip' % version)
    shutil.make_archive('bingo-sqlserver-%s' % version, format='zip', root_dir=join(root, 'dist', 'bingo-sqlserver-%s' % version))
    shutil.rmtree('bingo-sqlserver-%s' % version)
    os.chdir(root)
