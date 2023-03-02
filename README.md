# CSH v1.0.0 <img src="http://icons.iconarchive.com/icons/blackvariant/button-ui-system-apps/1024/Terminal-icon.png" alt="terminal-icon" width="55" height="55" align="left"/>
#### *A simplified version of bash using C!* <br>*If you need a graphical tutorial for installation, click [here!](https://i.imgur.com/Ifnnse5.mp4)*

<p align="justify">The purpose of this project is to provide a basic understanding of how a shell works, how processes and subprocesses comunicate with the OS System and how commands are executed by a shell. Some commands are still emulating the behaviour of UNIX commands such as <b><i>cd</b></i> and <i><b>umask</b></i>. Probably they will be changed in a future release.</p>

## DOWNLOAD <img src="https://cdn3.iconfinder.com/data/icons/social-media-2169/24/social_media_social_media_logo_git-1024.png" alt="git-icon" width="40" height="40" align="left"/>

<p align="justify">To download this project, you can either clone the repository using git clone command to get a copy of the project:</p>

```bash
git clone https://github.com/gu4re/CSH.git "your-folder-destination"
```` 

<img src="https://imgur.com/5lNegb4.png" align="right" alt="download-zip-button" width="280" height="230"/>
<p align="justify"><i>Note: after cloning the repo make sure to evently use 'git pull' command to get future versions of the project in your local folder:</i></p>

```bash
git pull
````

<p align="justify">Or, you can download the repository as a ZIP file by clicking on the green "Code" button at the top of the repository and selecting <a href="https://github.com/gu4re/CSH/archive/refs/heads/main.zip">Download ZIP</a>.</p>

## COMPILE & RUN <img src="https://cdn3.iconfinder.com/data/icons/luchesa-vol-9/128/Html-512.png" alt="code-icon" width="40" height="40" align="left"/>

<p align="justify">It is important to note that when compiling the code using gcc or any other compatible C compiler, the appropriate library for your system architecture must be included. This library can be found in the _libraries directory of this repo (either "libparser_x86.a" or "libparser_x64.a"). Note that there is currently <b>no library available for ARM architecture.</b></p>
<img src="https://imgur.com/LVn28Hu.png" align="right" alt="download-zip-button" width="340" height="160"/>
<p align="justify">Example with gcc compiler using csh_en.c file in a x64 architecture. Make sure you execute this command at main-branch level i.e as the side photo:</p>

```bash
cd "your-folder-destination"
gcc "_code/csh_en.c" "_libraries/libparser_x64.a" -o "name-of-executable" -Wall -Wextra -Wshadow
# Comment: "name-of-executable" can be anything, usually used the same name as the source file e.g "csh_en"
````
<p align="justify">After compiling the code, gcc will create an executable file in the current directory. To run and test CSH, just use in terminal the run "./" expression:</p>


```bash
./csh_en
# Comment: Following the sample above
````

## ISSUES <img src="https://www.clker.com/cliparts/9/1/4/0/11954322131712176739question_mark_naught101_02.svg.hi.png" alt="issues-icon" width="40" height="40" align="left"/>

<p align="justify">Any issue can be reported <a href="https://github.com/gu4re/gu4re/issues">here<a> including as header <b>"CSH ISSUE"</b> and I will try to solve it as soon as posible!</p>

## LICENSE <img src="https://www.logomed-gabinet.pl/wp-content/uploads/2021/06/license-icon-2793454.png" alt="license-icon" width="40" height="40" align="left"/>

<p align="justify">This project is licensed under the open source Apache License 2.0. Please make sure that you comply with <a href="https://github.com/gu4re/CSH/blob/main/LICENSE">the terms of this license<a> when using the code in this repository.</p>
