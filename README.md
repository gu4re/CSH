# CSH 
#### *A simplified version of bash using C!*

The purpose of this project is to provide a basic understanding of how a shell works, how processes and subprocesses comunicate with the OS System and how commands are executed by a shell. Some commands are still emulating the behaviour of UNIX commands such as *"cd"* and *"umask"*. Probably they will be changed in a future release.

## Installation 🔌

To install this project, you can either clone the repository using git clone command:

```bash
git clone https://github.com/gu4re/CSH.git "your-folder-destination"
````

Or, you can download the repository as a ZIP file by clicking on the green "Code" button at the top of the repository and selecting "Download ZIP".

It is important to note that when compiling the code using gcc or any other compatible C compiler, the appropriate library for your system architecture must be included. This library can be found in the _libraries directory of this repo (either libparser_x86 or libparser_x64). Note that there is currently **no library available for ARM architecture.**

## License 📃

This project is licensed under the Apache License 2.0. Please make sure that you comply with the terms of this license when using the code in this repository.
