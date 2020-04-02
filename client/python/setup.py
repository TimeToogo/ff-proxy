from setuptools import setup, find_packages

with open("README.md", "r") as fh:
    long_description = fh.read()

setup(
    name="ff-client",
    version="__VERSION__",
    author="Elliot Levin",
    author_email="elliotlevin@hotmail.com",
    description="Client library for ff-proxy",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/TimeToogo/ff-proxy",
    packages=find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.6",
)
