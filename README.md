# README #

## How do I get set up? ##

The following steps describe how to compile the code and get the game running on Windows.

### Dependencies ###

* Microsoft Visual Studio 2019, the community edition is free
* Qt 5.14.1
* Qt [vs addin](http://download.qt.io/official_releases/vsaddin/2.5.2/) (recommended)
* [Noesis Gui](https://www.noesisengine.com/developers/downloads.php) 3.0.4 \
  Download the native SDK, open the Visual Studio solution and build the NoesisApp.
* [Steam SDK](https://partner.steamgames.com/doc/sdk)

### Configuration ###
You will need to provide the paths to the following dependencies in environment variables:

	STEAMDIR = steamworks_sdk_xxx\sdk
	QTDIR = 5.14.1\msvc2017_64\
	NoesisRoot = noesis-3.0.4
	
Compilation will initially fail in `license.h` due to a missing Noesis license. \
The content can be either a [noesis trial license](https://www.noesisengine.com/trial/) or a full license if you have 
one. \
*Never commit this file to the repository.* (Use `git update-index --skip-worktree src/gui/license.h` to exclude from tracking.)

	
### Database configuration ###

The game uses a squlite database. It can be created from the .sql file in the `/content/db` folder or copied
over from the installed game from Steam.

### Deployment ###

To run the game copy over the contents of `/content/tilesheet` from your Steam installation.

## Contribution guidelines ##

Help with Ignomia is always welcome.

### Making suggestions ###

We love to hear about your ideas on this game! Please head straight to our [Discord](https://discord.gg/DCSmxVD) server and discuss them in the #suggestions channel. You might see them realized at some point.

Please don't open tickets for suggestions on your own. That is reserved for already planed features.

### Reporting bugs ###

Even if you can't contribute in code, testing and reporting bugs is just as important to us. If you find something which doesn't behave right, or even crashes, feel free to open a ticket in the bugtracker.

Please include only one bug per ticket, with a precise step-by-step instruction how to trigger, and search for open tickets on the same subject first.

If you prefer, you may also try to reproduce bugs reported by other users. The more precise informations on a bug are, the higher the chance it can be fixed.

### Code contributions ###

If you know any of C++ or WPF, you can help right away! Feel free to check for open bugs, and hop over to our [Discord](https://discord.gg/DCSmxVD) channel to get you sorted in.

Please provide your contributions in the form of a pull request, rebased onto the current head of development.

#### License ####

The contents of this repository are licensed under [GNU AFFERO GENERAL PUBLIC LICENSE Version 3](LICENSE). All contributions must adhere to the terms and conditions of this license. By submitting a pull request, you attest that you own the necessary rights on the code and you will abide to the AGPL3 license.

### Who do I talk to? ###

Hop over to our [Discord](https://discord.gg/DCSmxVD) server or open a ticket please.