# Changelog
All notable changes to this project will be documented in this file.

## [unreleased]

### Fixed
- message window not disappearing after clicking an answer button if game is unpaused, 
  game not possible to unpause now when message window is open
### Added

### Changed

### Removed



## [0.8.0]
### Fixed
- Isolated simulation thread from UI thread, so they don't share data.
  UI components are now split into a UI controller living in t he UI thread and a data aggregation component living in the simulation thread.
  Both parts are connected via the Qt message system, but are not allowed to access state owned by the other thread.
- Prevent errors in sprite mapping from cascading. The errors themselves are not resolved yet, but will no longer corrupt an entire save game.
- Always resolve all assets relative to the main executable.
### Added
- Added Noesis based UI, replacing the existing UI from scratch. (Work in progress.)
- Added "Hunting" behavior, in addition to legacy "defend" behavior.
  Squads can actively seek out enemies all over the map as they become visible, without user involvement.
- Added Linux support.
### Changed
- Relicense code under [GNU AFFERO GENERAL PUBLIC LICENSE Version 3](LICENSE) and publish as open source.
- Switch to CMake build system.
- Switch from C++11 with MSVC legacy extensions to pure C++17.
- Update Qt to 5.14.1.
- Refactored rendering.
	- Cull empty tiles in vertex rather than fragment shader.
	- Split opaque and transparent tiles into distinct render passes.
	- Upload data to GPU via compute shader rahter than direct mapping.
	- Render dark areas desaturated.
	- Improve visibility of designations.
	- Cull tiles down to minimal screen space size in order to minimize overdraw.
	- Increase required OpenGL version to 4.3.
	- Add depth buffer to rendering.
- Replace explicit squad target control by behavior settings for Squad.
- Move Uniform assignment from squad position to individual gnome.
  Gnomes can now have a uniform without being assigned to a squad, but they still need a squad to control their combat behavior.
- Allow arbitrarily sized squads, up from only 5 slots.
- Speed up high level pathability tests.
- Speed up A* pathfinding.
- Speed up liquid simulation.
- Speed up spatial item lookup.
- Switch from file based SQLite DB to pure in-memory SQLite DB.
- Load SQLite DB from human readable SQL file instead of binary representation.
- Run as "native Windows 10 application" under Windows 10, rather than running in compatibility mode.

### Removed
- Removed old Qt based UI.
- Removed RapidJSON from JSON parsing and dependencies. Comments in JSON files are no longer supported.
- Removed Quazip from dependencies.
- Removed zLib from dependencies.

## [0.7.5]
- fix bee hive now craftable at carpenter
- add wall torch
- add plank stairs
- fix furniture not buildable in rooms
- fix phase shifting gnomes
- fix deconstructing floors moves items to the worker's position
- fix item search abandoning too soon
- added categories to the workshop construction window to make it easier to find a specific workshop
- add adjustable threshold to item overlay, mark item red if amount is lower than threshold
- add big torch and brazier, both crafted in the blacksmith workshop
- add config option to control the render order of layers: "renderMode": "down|up" up looks better with water transparency but adds more load to the gpu
- fix rotating dresser
- fix military widget multiple issues
- fix gnomes in get job loop when trees not reachable for pick fruit or fell
- add damage reduction when wearing armor
- add glass workshops
- make renderDown the default mode

## [0.7.4]
- sprite handling rework to allow more sprites in the future
- added adjustable limit to hay harvesting
- added water evaporation
- fix save/load auqifiers and deaquifiers
- fix building a wall removes water on that tile
- added setting to set target fps when the game is paused, ( config.json: "pausedRenderTimer": # ) higher number = lower fps
- added setting to adjust the light level at night time
- added check connection between items and jobs before give the job to a gnome, fixes gnomes getting stuck because of unreachable items
- fixed workshop tasks showing different materials when a new material appeared after creating the task
- prospecting now has a 50% success chance
- prospecting outcome now depends on source material
- fix constructed wallfloor disappearing when building a wall on top
- added alarm bell and gnomes retreating to its location when the alarm is raised

## [0.7.3]
- fix enemy kingdoms not attacking
- fix goblin aggression being so high it overcomes the peaceful setting
- fix gnomes changing haircolor depending on the direction they face
- fix bug with fill trough that would stop gnomes from working
- fix some weapon crafting recipes
- added popup and center camera on gnome death
- when sending several explorers at the same time, they shouldn't find the same kingdom anymore
- fix endless get job loop when required items aren't reachable
- added checkbox to pasture widget to toggle harvest jobs (milk, shear), that setting will be off for existing games so please check your pastures
- improved auto crafting behavior
- fix saving of embark presets
- fix prospector making stone slivers
- added fill hole command
- fix crafting of metal bars when source material "any" is selected
- fix reset mission list wehn starting a new game
- fix crash with removing stockpile filter items
- added checkbox to world size slider to enable map sizes greater than 300
- added harvesting grass for hay from pastures
- building a structure like fence that removes the WALKABLE flag will also remove the designation (farm, stockpile...) on that tile
- improved (hopefully) stockpiling performance
- goblins should no longer target gnomes that are on a mission
- workshop and stockpile windows now remember their size and position
- added new sprites for rough stone walls
- split rendering into two passes for floor and everything above, sprites overlapping tile borders won't get cut off by neighboring floor sprite anymore

## [0.7.2]
- fix building workshops over the edge
- fix automaton removing cores
- fix automaton list reset on load
- added quick save
- fixed crash with non existant mission
- added check if gnome can reach workshop when gettings jobs
- some animals now drop hides on butcher
- item info now shows tool level if item is a tool
- job info in the tile info widget now shows required tool level if the job requires a tool
- added flintstone pickaxe head
- fix wood bed frame 
- fix wall floor construction
- grain plants (wheat, oat, millet, barley) no longer produce seeds, the seed item is grain, they now produce 2 grain per harvest with a 50% chance of a third
- fix giving out jobs when required items not exist, this will reduce many loops of getJob-dropJob
- fix when building a wall it sets the embedded material and item spriteID to zero
- fix when building a wall it moves items and creatures out of that tile.
- fix unreachable material preventing some crafting
- removed the constraint that all items have to be the same material when selecting "any" material for crafting
- stopped eggs from walking around
- added beehives and honey
- added flowers
- added waste disposal workshop

## [0.7.1]
- fixed some definitions, flour, bone table
- fix saving/loading for larger data size
- fixed crash with removing stockpiles under constructed containers
- fixed workshops creating auto craft jobs even when they shouldn't
- fixed crash with deleting stockpiles
- fixed crash with too many gnomes on the profession overview list
- fixed crafting of automaton parts
- fix auto harvester for vegetables
- it's now possible to toggle the debug overlay, default key O
- added on screen item counts
- quadrupled move speed of gnomes

## [0.7.0]
- added death from drowning
- added automatons
- fixed floor disappearing when deconstructing workshops
- fixed replacing fancy floors and walls
- added windmill
- add bone and skull walls
- adjusted all item prices (wip)
- fixed mod support
- prepared main menu and the new game widget for translations
- added neighbor kingdoms and interactions with them
- made the ocean front uneven
- added rivers

## [0.6.9]
- added profession selection to profession overview
- lowered required tool level for felling trees so wooden felling axes work now
- added sheep dyeing
- add sort by profession to profession overview
- checking for default profession on start and add it if it's missing
- improved random movement
- added hard limit of 1000 creatures per type
- fix creature breeding when limit is reached
- fix deconstructing doors
- fix a crash with pastures
- improved grass spreading
- added birch trees
- improved water handling
- added engineer and machine workshops
- fixed a bug with digging ramps down

## [0.6.8]
- added metal pickaxe heads
- added peaceful mode
- added octree to item management
- fixed floor sprite rotation for stone stairs
- added toggle for stockpile limit modes
- assign workshop, sort gnomes by name
- fixed super fast cows
- fix crash with deleting a stockpile
- added deleting of save games
- add priorities to farms, groves and pastures
- build containers with multi selection
- fix traders having no inventory after load
- fix beds sometimes not released properly
- fix crash with dyers workshop and auto generating craft plank jobs
- fix dyer workshop, save/load created materials and names

## [0.6.7]
- possible fix for wrong material crafts
- fix double tile definition for tailor workshops
- add saving of monsters
- add saving of event manager state
- fix saving time and day with first auto save on new game
- added occupied tile flag to prevent building into trees
- fix clear reserved items on hauling job abortion
- stockpile limits only uses item type now
- fix replace wall being able to be applied to workshops
- improved replacing of walls and floors
- improved deconstructing
- prevent digging ramps under fishery
- add workshop craft to by material

## [0.6.6]
- add target assignment for squads
- fixed freeze with auto craft
- fixed crash when pressing keys while in the menu
- fix deconstructing workshops
- fix lockup with butcher workshop
- fix releasing carried items when a wheelbarrow job gets interrupted. 
- fix traders having no items
- fix job selection test tile when clicking different job while one is already selected
- switched sacks and bags, sacks are nor buildable and bags the carry container
- added container info to item info widget
- fix remove items from containers too when taking them from a stockpile
- fix building sacks outside of stockpiles
- fix building walls on doors
- fix item count in trade widgte for items with quality

## [0.6.5]
- fixed crash when changing workhop priority
- fixed setting uniform and squad names
- fixed missing entry in material to tool level table for platinum
- added auto save
- fixed tools not getting release properly when interupted (this time for real hopefully)
- reworked light handling, allow gnomes to carry torches

## [0.6.4]
- fixed build palisade, not possible anymore with floor above
- fixed loading of installed crates on stockpiles
- reduced DB calls for farms and groves
- fixed stockpiles generating hauling jobs for fields with containers that don't accept that item
- fix workshop crafting now costs time accoring to the crafting recipe
- fix designating a grove also set farm flag
- added armor and weapon crafting
- added armor equipping
- added military settings window
- added training grounds
- fixed crash when clicking the tailor workshop, error in mattress sprite
- fixed crash when clicking an A button in the skill overview window
- fixed soil corner ramp orientation
- fixed crash when restarting the game
- fix stone stair rotation
- fix undefined job state when a gnome was working a job and a need kicked in
- fix grass-soil uramps showing the winter sprite all season
- fix another instance of tools not releasing properly when sleep hits

## [0.6.3]
- added scheduling
- cached some more db values to reduce reoccurring db access
- added ingame bug report
- fixed fences not showing up
- fixed animals procreating
- fixed stockpile stacksize when installing container
- first goblins won't appear before summer year two
- increased gnome move speed
- clear log on start now clears log on start
- fixed oak tree rotation - nobody noticed wtf
- reduced grow time for most plants significantly

## [0.6.2]
- added prospecting recipes
- fix building fisheries
- fix crash with dirt floor sprites after loading
- fix materials not registering for crafts
- fix tech gain for crafting at workshops
- fix tool pick up for dig jobs
- fix planks and blocks not recognized for floor construction

## [0.6.1]
- added default material to sprite definition, no more bone table as default sprite when building workshops
- fixed material2 for combined items at startup
- redesigned the stockpile filter tab
- added workshop priority
- added assign gnome to workshops

## [0.6.0]
- added indicator to butcher list if animal is young
- added job sprite to upper wall tile for DigStairs
- fix renaming groves
- fix adding tiles to existing groves
- added oak trees
- added combat - this is ongoing development
- added auto butchering of corpses
- added mushroom biome
- added keybinding options for actions
- added oaks for oak tree planting
- replaced the external data storage with a SQLite database

## [0.5.9]
- talked to some unreliable onions and straightened them out
- fix build thatch outer ramp works again
- mine stairs up now removes the wall in the tile above
- added more walls with floor on top
- fix pine cones defs for stockpiles and containers
- add buildable soil and stone outer corner ramps
- clicking the gnome list to open a gnome window that is already open now brings that window to front
- fixed crash with opening item info when the maker has died
- dead gnomes disappear after two days
- fix animal state change tick doesn't reset on load
- fix animals grow up again
- disallow setting designations on tiles occupied by tree leaves
- reinstated groves
- added some dyes
- added hair dyeing
- improved lighting

## [0.5.8]
- creatures will now evade trees suddenly growing up below them
- fixed priorities for different job types with same skill requirement
- newly built workshops now accept generated jobs by default
- reinstated death from hunger and thirst
- gnomes now eat and drink up to full once they started
- auto craft items for build jobs
- plant tree now checks for other trees in range
- plant tree now checks for other plant tree jobs in range
- adjusted sunrise and sunset times for seasons
- fix missing fish bones in stockpile filter
- fix claim items for workshop craft jobs, when a linked stockpile exists but the item is not in that stockpile
- fix job priorities when there are farming jobs around
- fix deconstructing workhops only creates on deconstruct job even if more tiles are selected
- fix deconstructing scaffolds deconstructs all scaffolds above
- felling pine trees now produces 1-3 pine cones that can be used to plant new trees
- fix plants growing in seasons they shouldn't when planted in that season
- now also expel items from the trunk tile when a tree grows up
- digging stairs down should create raw materials at "0, 0, 0" anymore
- added indirect sunlight to neighboring tiles
- fixed trader no appearing

## [0.5.7]
New save game required
- fix some settings not saving their changes
- harvesting trees is now a Horticulture job
- dig hole now creates ramps at the outer tiles of the hole
- improved get best drink, get best food
- improved constructing ramps
- fixed crash with removing a farm while paused
- fix adding tiles to existing designations open the wrong window
- added job priorities
- fixed crash when removing linked stockpiles
- raised drink value of wine to 40
- carpenters can now craft fishing rods
- changed saving items so that adding new items won't break saves anymore (breaks save games one last time)

## [0.5.6]
- fix reset the behavior tree when aborting a job through skill deactivation
- fix render depth in settings is an INT not a BOOL
- fix harvesting trees
- reactivated wheelbarrows to work with behavior tree
- fix workshop widget checkbox showing the correct state for accept jobs
- fix BT for aborting jobs
- fix deconstructing stuff 
- fix deconstructing workshops returns items now

## [0.5.5]
New save game required
- fix deconstructing built ramps
- fixed checkboxes not visible
- added behavior trees for creature AI
- added sheds, troughs and beehives

## [0.5.4]
- added seed and animal trader
- added migration, 1-5 gnomes will arrive each season
- tile info window doesn't reveal undiscovered tiles anymore
- fix crash with population overview clicking skill button after sorting
- added bone furniture
- added wall paintings

## [0.5.3]
- fixed animals not following gnomes
- added support for stylesheet mods
- improved job sprite rendering
- improved job sprites for wall and floor constructions
- seperated stone and brick blocks

## [0.5.2]
- fix duplicate sprite creations
- debug mode disable fow
- started implementing basic magic features
- added job info to tile info widget
- UI overhaul
- fix not possible to build walls on ramp tiles anymore
- fix floor replacement on grass tiles turning out as raw soil floor

## [0.5.1]
New save game required
- fixed workshop crafting queue behaving strangely
- abort jobs when required skill gets deactivated
- can activate stockpile filter categories even with no items in that category present so that newly created items go to that stockpile
- added trading

## [0.5.0]
- add buckets and sacks as carry containers
- added fishery workshop
- added item history

## [0.4.9]
- fixed missing check for pasture when designating areas
- added wheelbarrows
- added gui for game settings
- added auto generate jobs

## [0.4.8]
- added checkbox to gnome window to let them ignore no pass zones
- removed seperate render passes for floor and wall tiles

## [0.4.7]
- added no pass designation 
- fix falling gnomes discover their new location
- remove floor - improve job selection to avoid getting trapped
- build wall - improve job selection to avoid getting trapped
- add renaming of gnomes

## [0.4.6]
New save game required
- added dig ramps down
- water flows down now
- wild animals no longer spawn in the starting zone
- animals now getting hungry
- some animals on pastures eat grass
- fix dig hole
- fixed gravity for dropped tools
- fixed path finding/move order not correctly aborting in in some cases
- remove dead gnomes from lists

## [0.4.5]
- tiles with non wall neighbors are discovered at game start
- grass spreads now to dirt tiles
- fixed beer and bread recipes taking grain
- reinstated water
- added fish

## [0.4.4]
- improved check if item is allowed in a container
- fixed items in containers being rendered after load
- built soil and rough stone walls and floors will be mined rather than deconstructed
- added cabinet, dresser, statues to funiture options
- fixed a workshop crafting related crash
- animals no longer randomly wander through doors that are blocked for them
- fix: pasture onle show collect eggs checkbox for egg laying animals
- separated selection rendering from main render

## [0.4.3]
- update lights in range when mining a wall
- open stockpile window at stockpile creation
- goats give milk now
- eggs can now be collected, omelette!
- removed glFinish in the main render function, this might improve things on some systems
- fixed a stockpile related crash

## [0.4.2]
- fix Gnome list showing the right now after sorting
- another fix for initial gnome speed
- fixed brewing recipes to use brewing skill instead of cooking
- building/replacing floor tiles with ramp tops is no longer possible
- prevent logs created by felling a tree at map border to float in limbo

## [0.4.1]
- shift leftclick now forces the tile info widget to open even if only one thing is on the tile
- made gnomes less blinking
- added cheese
- added move top/bottom buttons to stockpile widgets
- fixed bug that caused craft jobs with materials set to "any" not being started
- add "fow" option to configs when missing

## [0.4.0]
- stockpiling, tiles with containers are handled first
- job queue: craft# the number to craft will count down now
- job queue: craft to, jobs will not disappear anymore when the number is reached but suspend
- slowed down fast runners
- sun loving plants no longer grow underground
- fixed crash when removing a pasture
- fixed selection checking for zLevel to only stop at walls and floors

## [0.3.9]
- fix: update selection z level when changing level
- added chisel to stone mason
- added 3 pixel to mouse y for better tile selection
- skill system overhaul (first part) - needs new game
- gnome move speed now depends on hauling skill
- added save button to profession management to prevent unwanted changes to professions

## [0.3.8]
- fix: saving keeps stockpile order now
- no more duplicate gnome names, Ulf Ulfson son of Ulf disagrees
- gnomes on gnome list and profession overview are now sorted by name
- population overview can now be sorted by skill
- fixed keyboard zoom not working
- population overview and gnome widgets now update each other in real time

## [0.3.7]
- improved world rotation
- added keybinding ctrl + / ctrl - for changing z levels
- made key bindings configurable

## [0.3.6]
- added missing translation strings for some item groups
- fix replace floor to work under designations
- fixed a crash with multiple butchers butchering the same animals
- fix missing item definition for sausage and sandwich causing pink boxes
- fixed messed up sprites for constructed items on load (double beds)
- fix missing definitions ammo pouch
- fix workshop craft# and craft to

## [0.3.5]
- fixed canceling workshop constructions leaving job sprites behind
- show stair orientation when digging stairs down
- fixed creatures and gnomes flashing when on the same tile
- reworked the workshop gui
- stockpiles: initial suspend threshold per item is now 0, if it's 0 the suspend threshold is ignored
- fixed some items not possible to stockpile - please report if an item doesn't appear in the stockpile filter

## [0.3.4]
- fixed some crashes with butchering animals
- added short walls, hotkey H - will need new game
- added bones to all butchering results
- added animals: duck, goose, dog, cat, fox, wolf, deer, goat, squirrel
- fixed sand short wall

## [0.3.3]
- if embark items is empty medium preset will be selected
- fixed crash when clicking on certain tiles of a rotated crude workbench

## [0.3.2]
- replacing walls now return materials
- moved the log file to C:\Users\<User>\Documents\My Games\Ingnomia
- tweaked cursor positioning if there is a wall structure in the tile in front of it
- fix space bar for toggle pause
- added keyboard plus and minus for zoom

## [0.3.1]
- abort jobs now correctly unclaims items
- hunger and thirst now only abort jobs if food/drink items are available
- fixed a crash with aborting jobs
- made the new game screen more friendly for lower resolutions
- added button to create a random gnome
- fixed saving of job sprites
- added status messages to loading screen
- fixed gnomes freezing after load

## [0.3.0]
- added check if required items exist before handing out jobs, should reduce green/yellow flashing of jobs
- fixed memory leak with jobs
- added message box when opengl init fails
- reduced sleep, hunger, thirst
- renamed the build door widget to build utility so people stop asking why torches are doors
- fix stockpile filter settings for loading/saving copy&paste
- big windows like Population Overview, Stocks and Kingdom now have a max size of the game window

## [0.2.9]
- fixed crash with butcher when pressing cancel on empty list
- fixed crash with deconstructing a torch
- fixed removing rooms
- fixed deconstructing furniture in rooms
- fixed ghosts of felling axes making it to the next game

## [0.2.8]
- fixed creation of ghost items at position (0,0,0)
- added tooltip to construct workshop button listing crafts
- fix crash with deconstructing fences
- added butcher excess lifestock
- lowered food intake and doubled the time before gnomes die from thirst and starvation
- added mine stairs up
- fixed deconstruction stairs

## [0.2.7]
- fixed crashes with removing stockpiles and pasture
- fixed domr stuff carrying over after new game
- improved eating and drinking
- added highlight to selected item in right click menu

## [0.2.6]
- fences
- added cows, moo
- fixed rotating workhop job sprites
- fixed crash when loading constructions without sprite
- add remove floor, replace wall/floor commands
- added remove plant command

## [0.2.5]
- require felling axe for tree cutting
- added baby animals
- fixed crash with workshops with empty recipe lists
- gnomes now die from hunger and thirst
- updated to Qt5.11.1

## [0.2.4]
- menu option continue loads last save game now
- when using a container to build a workshop it now drops its items
- added metals and gems to underground
- added config option for fow
- reordered farm job priorities
- fix gnome widget professions
- added embark item presets

## [0.2.3]
- added random kingdom name generation
- added support for multiple save games
- added status screen for messages during world generation
- screenshots
- added keyboard support ( 1 - 0 ) for commands
- hide undiscovered tiles

## [0.2.2] 
- added butchering animals from pastures

## [0.2.1]
- added pasture
- rework of animal actions

## [0.2.0]
- part two of stockpile rework, per item suspends
- added copy&paste for stockpile settings
- fix work sprites disappearing on escape
- added right click menu

## [0.1.9]
- when only one entity is present on a tile, clicking it skips the tile info window and opens the respective window
- part one of stockpile rework

## [0.1.8]
- added wall build mode that auto builds floor tiles below
- added outer ramp corners
- added checkbox to farmwidget to shown only types with available seed item
- improved renderer

## [0.1.7]
- added digHole command
- added building of most wall and floor types
- added scaffold
- added gravity to gnomes and items when removing floor tiles

## [0.1.6]
- fixed placement of starting items
- added dyers workshop

## [0.1.5]
- fixed cursor pos for rotated views
- paint job sprite green after required tool is picked up
- seasonal sprites for slopes

## [0.1.4]
- added new game screen
- fix #81 changing profession for gnome created additional need bars and attribs

## [0.1.3]
- added water processing and rendering
- added many workshops and craftable items
- added distillery workshop
- containers which require the same item and material type now do so

## [0.1.2]
- added item info widget
- added light handling to renderer
- added day/night mode
- saving/loading of lights
- animated torches
- added personal room assignment
- added beds to rooms and dorms, gnomes need sleep now
- added eating and drinking

## [0.1.1]
- added exprtk for math formula parsing
- fixed seasonal sprites for opengl renderer
- added clearLogOnStart option to config
- added enableScaling option to config
- added value bars for needs to gnome widget
- added value decay per minute for needs
- added activity log for gnomes
- possible fix for black background in some windows

## [0.1.0]
- updated to Qt 5.10.0
- added lots of plants
- added "enableLog" option to config.cfg
- added changelog.txt

## [0.0.9]
- new OpenGL based renderer
- reworked stockpile item filtering

## [0.0.8]
- reworked sprite handling
- gnome sprites generated from parts
- reworked the workshop construction window
- added high dpi scaling and font size adjustment
