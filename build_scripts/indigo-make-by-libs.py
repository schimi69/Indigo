# This module assumes that you have installed all the
# libs files in the <source root>/dist directory

import os
import shutil
import sys
import re
import subprocess
import inspect
from optparse import OptionParser

def make_doc():
    curdir = abspath(os.curdir)
    script_dir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    root_dir = os.path.join(script_dir, "..")

    os.chdir(os.path.join(root_dir, 'api/python'))
    subprocess.check_call('"%s" copy-libs.py' % sys.executable, shell=True)
    os.chdir('../../doc')
    subprocess.check_call('"%s" builder.py' % sys.executable, shell=True)
    os.chdir(curdir)

def copy_doc(destname):
    shutil.copytree('../doc/build/html', join(curdir, destname, 'doc'))

def flatten_directory(dir):
    todelete = []
    for f in os.listdir(dir):
        if f.find("python") != -1 or f.find("java") != -1 or f.find("dotnet") != -1:
            continue
        dir2 = os.path.join(dir, f)
        if os.path.isdir(dir2):
            for f2 in os.listdir(dir2):
                f2full = os.path.join(dir2, f2)
                shutil.move(f2full, dir)
            todelete.append(dir2)
            os.rmdir(dir2)


def move_dir_content(src_dir, dest_dir):
    for f in os.listdir(src_dir):
        f2 = os.path.join(src_dir, f)
        destf2 = os.path.join(dest_dir, f)
        if os.path.isdir(destf2):
            move_dir_content(f2, destf2)
        elif not os.path.exists(destf2):
            shutil.move(f2, destf2)


def join_archives(names, destname):
    for name in names:
        if not os.path.exists(name + ".zip"):
            return
    for name in names:
        subprocess.check_call("unzip %s.zip -d %s" % (name, name), shell=True)
    os.mkdir(destname)
    for name in names:
        move_dir_content(name, destname)
    if os.path.exists(destname + ".zip"):
        os.remove(destname + ".zip")
    if args.doc:
        copy_doc(destname)
    subprocess.check_call("zip -r -9 -m %s.zip %s" % (destname, destname), shell=True)
    for name in names:
        shutil.rmtree(name)
        os.remove("%s.zip" % name)


def join_archives_by_pattern(pattern, destname):
    archives = []
    for f in os.listdir("."):
        if re.match(pattern, f):
            archives.append(os.path.splitext(f)[0])
    if len(archives) == 0:
        return
    print(archives)
    join_archives(archives, destname)


def clearLibs():
    for f in os.listdir(libs_dir):
        if f == "readme.txt":
            continue
        ffull = os.path.join(libs_dir, f)
        if os.path.isdir(ffull):
            shutil.rmtree(ffull)
        else:
            os.remove(ffull)


def unpackToLibs(name):
    if os.path.exists("tmp"):
        shutil.rmtree("tmp")
    subprocess.check_call("unzip %s.zip -d tmp" % (name), shell=True)
    move_dir_content(os.path.join("tmp", name), libs_dir)
    shutil.rmtree("tmp")


parser = OptionParser(description='Indigo libraries repacking')
parser.add_option('--libonlyname', help='extract only the library into api/lib')
parser.add_option('--config', default="Release", help='project configuration')
parser.add_option('--type', default='python,java,dotnet', help='wrapper (dotnet, java, python)')
parser.add_option('--doc', default=False, action='store_true', help='Build documentation')

(args, left_args) = parser.parse_args()

if not args.type:
    args.type = 'python,java,dotnet'

if args.doc:
    make_doc()

if len(left_args) > 0:
    print("Unexpected arguments: %s" % (str(left_args)))
    exit()

suffix = ""
if args.config.lower() != "release":
    suffix = "-" + args.config.lower()

need_join_archieves = (args.libonlyname == None)
need_gen_wrappers = (args.libonlyname == None)


cur_dir = os.path.split(__file__)[0]

# Find indigo version
sys.path.append(os.path.join(os.path.dirname(__file__), "..", "api"))
from get_indigo_version import getIndigoVersion

version = getIndigoVersion()
os.chdir(os.path.join(cur_dir, "../dist"))
if need_join_archieves:
    flatten_directory(".")

arc_joins = [
    ("indigo-libs-%ver%-linux-shared", "indigo-libs-%ver%-linux.+-shared" ),
    ("indigo-libs-%ver%-win-shared", "indigo-libs-%ver%-win.+-shared" ),
    ("indigo-libs-%ver%-mac-shared", "indigo-libs-%ver%-mac.+-shared" ),
    ("indigo-libs-%ver%-linux-static", "indigo-libs-%ver%-linux.+-static" ),
    ("indigo-libs-%ver%-win-static", "indigo-libs-%ver%-win.+-static" ),
    ("indigo-libs-%ver%-mac-static", "indigo-libs-%ver%-mac.+-static" ),
]

if need_join_archieves:
    for dest, pattern in arc_joins:
        p = pattern.replace("%ver%", version) + "\.zip"
        d = dest.replace("%ver%", version) + suffix
        join_archives_by_pattern(p, d)

print("*** Making wrappers *** ")

api_dir = os.path.abspath("../api")
libs_dir = os.path.join(api_dir, "libs")

wrappers = [
    ("win", ["win"]),
    ("linux", ["linux"]),
    ("mac", ["mac"]),
    ("universal", ["win", "linux", "mac"]),
]

wrappers_gen = ["make-java-wrappers.py", "make-python-wrappers.py", 'make-dotnet-wrappers.py']

for w, libs in wrappers:
    clearLibs()
    if args.libonlyname and w != args.libonlyname:
        continue
    any_exists = True
    for lib in libs:
        name = "indigo-libs-%s-%s-shared%s" % (version, lib, suffix)
        if os.path.exists(name + ".zip"):
            any_exists = any_exists and True
            unpackToLibs(name)
        else:
            any_exists = any_exists and False
    if not any_exists:
        continue
    if need_gen_wrappers:
        for gen in wrappers_gen:
            if args.type is not None:
                for g in args.type.split(','):
                    if gen.find(g) != -1:
                        subprocess.check_call('"%s" %s -s "-%s" %s' % (sys.executable, os.path.join(api_dir, gen), w, '--doc' if args.doc else ''), shell=True)
            
