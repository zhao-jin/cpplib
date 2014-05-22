import os, sys
        
from setuptools import setup,find_packages

pkgName='auto';
pkgList=[];

desc='';
long_desc = """ """;
pkgUrl=''

trunk = None
def main():
    setup(
        name=pkgName,
        description=desc,
        long_description = long_desc, 
        version= trunk or '1.0.0.0', 
        url=pkgUrl, 
        license='license',
        platforms=['unix', 'linux'],
        author='raymondxie',
        author_email='',
        entry_points={'console_scripts': ['']},
        classifiers=['Programming Language :: Python'],
	packages = find_packages('.'),
	include_package_data = True,
	package_data={pkgName: pkgList},
        zip_safe=False,
    )

if __name__ == '__main__':
    main()
        
