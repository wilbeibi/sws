#sws

------------
  
  
  

 Name: sws -- HTTP/1.0 web server
  
  Team Name: 404NotFound
  
### What's included
  

 ```
 .
 ├── LICENSE       
 ├── Make.on.Darwin
 ├── Make.on.Linux
 ├── Make.on.NetBSD
 ├── Make.on.SunOS
 ├── Makefile
 ├── README.md
 ├── cgi-bin
 │   ├── get.cgi
 │   ├── post.cgi
 │   └── test.cgi
 ├── index.html
 ├── install.sh
 ├── main.c
 ├── net.c
 ├── net.h
 ├── parse.c
 ├── parse.h
 ├── requests.c
 ├── requests.h
 ├── response.c
 ├── response.h
 ├── test.html
 ├── uninstall.sh
 └── util.c
 ```
 
### Setup
 
 The automatic installer/uninstaller  
 `chmod x ./install.sh`  
 `./install.sh`  
 and   
 `chmod x ./uninstall.sh`  
 `./uninstall.sh`
 
#### Problems?   
 There are some problems when `make` directly.
 
### Usage
 
 `./sws -h` to get usage info.
 More, check sws.1.pdf
 
### Contributors
 MinhuiGu  
 Kaikaiw  
 Wilbeibi  