# BTH Unix Assignment 2
Authors: Tomass Lacis, Gazi Samia Ahmed

How to compile and run:
- Change directory to webserver
- Run `make`
- Change directory to bin
- Run `sudo ./webserver` (with additional options if you want)
- In your browser, go to `http://localhost/index.html` to see it working, assuming you chose default port `80` and are running it on the same machine as the browser

Notes:
- Make sure you can execute `sudo` for chroot
- Code was tested on `Ubuntu 18.0.1 LTS` with GCC version `gcc (Ubuntu 7.3.0-16ubuntu3) 7.3.0`
- Server is capable of serving files other than .html (such as images and videos), see page one and page two
- We are aiming for **Grade C** (Requirements 2.1-2.10). We have also implemented chroot (Requirement 2.12), but we didn't have time for proper logging or adding fork-like request handling