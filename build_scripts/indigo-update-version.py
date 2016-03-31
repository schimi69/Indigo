import os
import re
import sys
import xml.etree.cElementTree as ElementTree


INDIGO_PATH = os.path.normpath(os.path.join(os.path.abspath(os.path.dirname(__file__)), os.path.pardir))


def getIndigoCMakeVersion():
    with open(os.path.join(INDIGO_PATH, 'api', 'indigo-version.cmake'), 'r') as f:
        m = re.search('SET\(INDIGO_VERSION \"(.*)\"\).*', f.read())
        return m.group(1)


def setIndigoCMakeVersion(newVersion):
    cmakeFile = os.path.join(INDIGO_PATH, 'api', 'indigo-version.cmake')
    with open(cmakeFile, 'r') as f:
        cmakeText = f.read()
        m = re.search('SET\(INDIGO_VERSION \"(.*)\"\).*', cmakeText)
        result = re.sub('SET\(INDIGO_VERSION \"(.*)\"\).*', 'SET(INDIGO_VERSION "{0}")'.format(newVersion), cmakeText)
    print('Updating Indigo version from {0} to {1} in {2}...'.format(m.group(1), newVersion, cmakeFile))
    with open(cmakeFile, 'w') as f:
        f.write(result)

def updatePomVersion(pomFile, newVersion):
    tree = ElementTree.parse(pomFile)
    ElementTree.register_namespace('', 'http://maven.apache.org/POM/4.0.0')
    root = tree.getroot()
    for child in root:
        if child.tag.endswith('version'):
            print('Updating Indigo version from {0} to {1} in {2}...'.format(child.text, newVersion, pomFile))
            child.text = newVersion
            break
    tree.write(pomFile)

def getValidAssemblyVersion(version):
    if (re.match('^\d*\.\d*\.\d*\.(\d*|\*)', version)):
        validVersion = version
    elif(re.match('^(\d*\.\d*\.\d*).*', version)):
        validVersion = re.sub('^(\d*\.\d*\.\d*).*', '\\1.*', version)
    elif(re.match('^(\d*\.\d*).*', version)):
        validVersion = re.sub('^(\d*\.\d*).*', '\\1.*.*', version)
    else:
        validVersion = "1.0.0.*"
    return validVersion
	
def updateAssemblyVersion(assemblyFile, newAssemblyVersion):
    p1 = re.compile('^([^/]*\[assembly\: AssemblyVersion\(")(.*)(\"\)\])', re.MULTILINE)
    p2 = re.compile('^([^/]*\[assembly\: AssemblyFileVersion\(")(.*)(\"\)\])', re.MULTILINE)
    with open(assemblyFile, 'r') as f:
        assemblyText = f.read()
        m1 = p1.search(assemblyText)
        r1 = p1.sub('{0}{1}{2}'.format(m1.group(1), newAssemblyVersion, m1.group(3)), assemblyText)
        newAssemblyFileVersion = re.sub('\*', '0', newAssemblyVersion) # * is not allowed for AssemblyFileVersion
        m2 = p2.search(r1)
        r2 = p2.sub('{0}{1}{2}'.format(m2.group(1), newAssemblyFileVersion, m2.group(3)), r1)
    print('Updating AssemblyVersion from {0} to {1} in {2}...'.format(m1.group(2), newAssemblyVersion, assemblyFile))
    print('Updating AssemblyFileVersion from {0} to {1} in {2}...'.format(m2.group(2), newAssemblyFileVersion, assemblyFile))
    with open(assemblyFile, 'w') as f:
        f.write(r2)


def main(newVersion=None):
    indigoVersion = newVersion if newVersion else getIndigoCMakeVersion()
    updatePomVersion(os.path.join(INDIGO_PATH, 'api', 'java', 'pom.xml'), indigoVersion)
    updatePomVersion(os.path.join(INDIGO_PATH, 'api', 'plugins', 'bingo', 'java', 'pom.xml'), indigoVersion)
    updatePomVersion(os.path.join(INDIGO_PATH, 'api', 'plugins', 'inchi', 'java', 'pom.xml'), indigoVersion)
    updatePomVersion(os.path.join(INDIGO_PATH, 'api', 'plugins', 'renderer', 'java', 'pom.xml'), indigoVersion)
    newAssemblyVersion = getValidAssemblyVersion(indigoVersion)
    updateAssemblyVersion(os.path.join(INDIGO_PATH, 'api', 'dotnet', 'Properties', 'AssemblyInfo.cs'), newAssemblyVersion)
    updateAssemblyVersion(os.path.join(INDIGO_PATH, 'api', 'plugins', 'bingo', 'dotnet', 'Properties', 'AssemblyInfo.cs'), newAssemblyVersion)
    updateAssemblyVersion(os.path.join(INDIGO_PATH, 'api', 'plugins', 'inchi', 'dotnet', 'Properties', 'AssemblyInfo.cs'), newAssemblyVersion)
    updateAssemblyVersion(os.path.join(INDIGO_PATH, 'api', 'plugins', 'renderer', 'dotnet', 'Properties', 'AssemblyInfo.cs'), newAssemblyVersion)
    if newVersion:
        setIndigoCMakeVersion(newVersion)

if __name__ == '__main__':
    main(sys.argv[1]) if len(sys.argv) == 2 else main()
