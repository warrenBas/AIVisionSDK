from setuptools import setup, find_packages

setup(
    name="ai_sensor",
    version="0.1",
    packages=find_packages(),
    install_requires=[
        "pyserial", 
    ],
)