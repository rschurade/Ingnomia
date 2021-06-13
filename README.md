# Ingnomia #

Just looking for the game itself?

Prebuilt binaries are available in the [release section](https://github.com/rschurade/Ingnomia/releases) or on 
[Steam](https://store.steampowered.com/app/709240/Ingnomia/).

## What is this? ##

Ingnomia started out as an independent remake of the older [Gnomoria](https://store.steampowered.com/app/224500/Gnomoria/) colony simulator, from which it was permitted to borrow some of the assets.
While the graphics look similar, all of the engine was rewritten from scratch. Compared to the reference, Ingnomias engine scales significantly better with large colonies.

In terms of features, balancing, user interface design etc. Ingnomia has since given its own spin to many of the core game elements.

While already in a playable state, it's still under heavy development and truly, by all means, "Early Access".
Not all game components have been implemented yet and some bugs are to be expected.

Ingnomia is a pure hobby project, and true free-to-play. With "free" spelled as in "free beer".

## How do I get set up for development? ##

The following steps describe how to compile the code locally and get the game running on Windows and Linux.

Note! Building on Mac is currently not possible. Certain features in the renderer require OpenGL4.3.

### Dependencies ###

#### Windows specific ####
* Microsoft Visual Studio 2019, the community edition is free
* Qt [vs addin](http://download.qt.io/official_releases/vsaddin/2.5.2/) (optional)
#### All Platforms ####
* OpenGL 4.3 - Mac is not a supported compilation platform since it has deprecated OpenGL
* Qt 5.14.1 or newer
* [Noesis Gui](https://www.noesisengine.com/developers/downloads.php) 3.0.12\
  For using Noesis in a local development build, you need to get a [trial license](https://www.noesisengine.com/trial/).
* [Steam SDK](https://partner.steamgames.com/doc/sdk)
* [SFML 2.5.1](https://www.sfml-dev.org/download/sfml/2.5.1/)
* CMake 3.16 or newer

### Build ###

```bash
cp -r "<EXISTING_INGOMIA_INSTALLATION>/content/tilesheet" content/

cmake -S . -B "<BUILD_DIR>" \
-DQt5_DIR="<QTINSTALLDIR>/<ARCH>/lib/cmake/Qt5" \
-DSTEAM_SDK_ROOT="<STEAMSDKDIR>/sdk" \
-DNOESIS_ROOT="<NOESISDIR>" \
-DNOESIS_LICENSE_NAME="<NOESIS_TRIAL_LICENSE_NAME>" \
-DNOESIS_LICENSE_KEY="<NOESIS_TRIAL_LICENSE_KEY>"
-DSFML_DIR="<SFMLDIR>/lib/cmake/SFML"
```

If no errors have occured, proceed by building the project with the chosen build system or open the generated project in an IDE of your choice.

```bash
cmake --build "<BUILD_DIR>"
```

### Building the documentation ###

Documentation building scripts are in the `docs/` directory. To rebuild documentation you will need:

* Python 3.9
* Pipenv
* Tilesheets from an existing installation (in the `content/tilesheet` directory, as when building the game)

From the `docs/` directory, run:

```bash
pipenv install
pipenv run ./generate.py
```

Output is generated in `docs/html`. You can specify a different output by passing `--output path/to/dir` to the generation script.

Generation will fail when the output directory exists. Use the `--overwrite` option in that case, but be warned that it will wipe the output directory completely.

### Forks on Github ###

When forking the project on Github, you should add `NOESIS_LICENSE_KEY` and `NOESIS_LICENSE_NAME` as secrets in your repositories configuration under Settings/Secrets. If not provided, CI builds will fail for your fork.

Ingnomia comes with automatic builds via GitHub Actions, triggered on push to repository.
As long as you have forked Ingnomia as a **public** repository, these are expected to be free-of-charge for your GitHub account.

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

If you know any of C++ or XAML you can help right away! Feel free to check for open bugs, and hop over to our [Discord](https://discord.gg/DCSmxVD) channel to get you sorted in.

Please provide your contributions in the form of a pull request, rebased onto the current head of development.

#### License ####

The contents of this repository are licensed under [GNU AFFERO GENERAL PUBLIC LICENSE Version 3](LICENSE). All contributions must adhere to the terms and conditions of this license. By submitting a pull request, you attest that you own the necessary rights on the code and you will abide to the AGPL3 license.

### Who do I talk to? ###

Hop over to our [Discord](https://discord.gg/DCSmxVD) server or open a ticket please.
