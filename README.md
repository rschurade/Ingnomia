# README #

This README would normally document whatever steps are necessary to get your application up and running.

### What is this repository for? ###

* Quick summary
* Version
* [Learn Markdown](https://bitbucket.org/tutorials/markdowndemo)

### How do I get set up? ###

* Summary of set up
The following steps describe how to compile the code and get the game running on Windows.

* Dependencies
Qt currently 5.14.1
[Noesis Gui](https://www.noesisengine.com/developers/downloads.php) Download the native SDK, open the 
Visual Studio solution and build the NoesisApp.
[Steam SDK](https://partner.steamgames.com/doc/sdk)

* Configuration	
Variables to set:
	STEAMDIR
	QTDIR
	QUAZIPDIR
	NoesisRoot
	
Compilation will fail in mainwindow.cpp because of the missing file license.h Create this file in the same folder
with the following content

    #pragma once
	const char* licenseName = "";
	const char* licenseKey = "";

The content will be either a [noesis trial license](https://www.noesisengine.com/trial/) or a full license if you have 
one. Never commit this file to the repository.

	
* Database configuration
The game uses a squlite database. It can be created from the .sql file in the /content/db folder or copied
over from the installed game from Steam.
* How to run tests
* Deployment instructions
To run the game copy over the contents of /content/tilesheet from your Steam installation.

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact