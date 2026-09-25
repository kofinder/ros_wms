from setuptools import find_packages, setup

package_name = 'wms_mobile_control'

setup(
    name=package_name,
    version='0.0.1',

    packages=find_packages(
        exclude=['test']
    ),

    data_files=[
        (
            'share/ament_index/resource_index/packages',
            ['resource/' + package_name]
        ),
        (
            'share/' + package_name,
            ['package.xml']
        ),
    ],

    install_requires=[
        'setuptools'
    ],

    zip_safe=True,

    maintainer='lynx',
    maintainer_email='thein@wmw.com',

    description='Mobile web controller for WMS AMR',

    license='MIT',

    entry_points={
        'console_scripts': [
            'mobile_control = '
            'wms_mobile_control.mobile_control:main',
        ],
    },
)